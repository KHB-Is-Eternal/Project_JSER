// Fill out your copyright notice in the Description page of Project Settings.


#include "SkillSystem/GameplayCueNotify/Particle/GCN_SpawnNiagaraBySpawnConfig.h"
#include "SkillSystem/GameplayCueNotify/Particle/SkillNiagaraSpawnConfig.h"
#include "SkillSystem/GameplayCueNotify/Particle/SkillNiagaraSpawnHelper.h"
#include "SkillSystem/GameplayCueNotify/Particle/SkillVfxCullingHelper.h"
#include "SkillSystem/GameplayCueNotify/Particle/VisionParticleManagerSubsystem.h"
#include "CharacterSystem/GameplayTags/GameplayTags.h"

#include "Engine/Blueprint.h"
#include "AbilitySystemStats.h"
#include "AbilitySystemGlobals.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystemLog.h"
#include "GameplayCueManager.h"
#include "NiagaraComponent.h"
#include "NiagaraSystem.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/RootMotionSource.h"
#include "GameFramework/ProjectileMovementComponent.h"
#include "CharacterSystem/Interface/TargetableInterface.h"

//#include UE_INLINE_GENERATED_CPP_BY_NAME(GameplayCueNotify_Static)

namespace
{
	/**
	 * SourceObject에서 USkillNiagaraSpawnConfig를 직접 가져옵니다.
	 * 기존 ResolveSettingsFromConfig를 완전히 대체합니다.
	 */
	const USkillNiagaraSpawnConfig* GetSpawnConfigFromParameters(const FGameplayCueParameters& Parameters)
	{
		return Cast<USkillNiagaraSpawnConfig>(Parameters.SourceObject.Get());
	}

	/** 공통 서버 체크 */
	bool ShouldSkipOnServer(const AActor* MyTarget)
	{
		if (!IsValid(MyTarget))
		{
			return true;
		}
		return MyTarget->GetNetMode() == NM_DedicatedServer;
	}
}

bool UGCN_SpawnNiagaraBySpawnConfig::OnExecute_Implementation(AActor* MyTarget, const FGameplayCueParameters& Parameters) const
{
	if (ShouldSkipOnServer(MyTarget))
	{
		return false;
	}

	const USkillNiagaraSpawnConfig* const SpawnConfig = GetSpawnConfigFromParameters(Parameters);
	if (!IsValid(SpawnConfig))
	{
		return false;
	}

	// [Optimization] 전역 시야 및 거리 최적화 판별
	// 이 GCN은 파티클이 부착될 수도, 단발성일 수도 있습니다. 보통 bAttachToSource 등에 따라 지속형 여부를 판별합니다.
	const bool bIsPersistent = SpawnConfig->bAttachToSource;
	const EVfxCullState CullState = USkillVfxCullingHelper::CheckVfxCulling(MyTarget, Parameters, bIsPersistent);
	if (CullState == EVfxCullState::SkipSpawn)
	{
		return false;
	}

	UWorld* const World = MyTarget->GetWorld();
	if (!IsValid(World))
	{
		return false;
	}
	const FSkillNiagaraSpawnSettings SpawnSettings = SpawnConfig->ToSettings();
	if (SpawnSettings.NiagaraSystem.IsNull())
	{
		return false;
	}

	const AActor* const EffectCauser = Cast<AActor>(Parameters.EffectCauser.Get());
	const AActor* const Instigator = Cast<AActor>(Parameters.Instigator.Get());

	// 네이티브 태그 참조
	const FGameplayTag& TagSummoner = ProjectER::GameplayCue::Skill::Summoner;
	const FGameplayTag& TagHitTarget = ProjectER::GameplayCue::Skill::HitTarget;
	
	const AActor* SourceActor = nullptr;
	if (Parameters.OriginalTag.MatchesTag(TagSummoner))
	{
	    SourceActor = IsValid(Instigator) ? Instigator : MyTarget;
	}
	else if (Parameters.OriginalTag.MatchesTag(TagHitTarget))
	{
	    SourceActor = MyTarget;
	}
	else // 기본값 (기존 Range 로직 통합)
	{
	    SourceActor = EffectCauser;
		// 발사체 또는 범위 액터가 충돌 즉시 파괴된 경우(SourceActor 무효), 시전자에게 붙지 않고 재생을 취소함.
		if (!IsValid(SourceActor))
		{
			return false;
		}
	}

	// 3. Transform 설정: SourceActor가 유효하면 그 트랜스폼을 기본으로 하되, Parameters.Location이 있으면 위치를 덮어씀
	FTransform SourceTransform;
	if (IsValid(SourceActor))
	{
		SourceTransform = SourceActor->GetActorTransform();
		
		// 소켓 설정이 없는 기본 상태이면서 외부에서 전달된 Location이 있다면 해당 위치를 우선함 (예: 장판 생성 위치)
		if (SpawnConfig->SocketOrBoneName == NAME_None && !Parameters.Location.IsNearlyZero())
		{
			SourceTransform.SetLocation(Parameters.Location);
		}
	}
	else
	{
		// [Fix] SourceActor가 없는 경우 MyTarget(적용 대상)의 회전이라도 사용하도록 변경 (Identity 방지)
		FRotator FallbackRotation = IsValid(MyTarget) ? MyTarget->GetActorRotation() : FRotator::ZeroRotator;
		SourceTransform = FTransform(FallbackRotation, Parameters.Location);
	}

	// 3. 중복 부착 방지: 동일한 SpawnConfig를 가진 컴포넌트가 이미 부착되어 있다면 선택적으로 제거
	if (IsValid(SpawnConfig) && SpawnConfig->bOverrideDuplicate)
	{
		const FName ConfigTagName = FName(*SpawnConfig->GetPathName());
		auto CleanupExistingNC = [ConfigTagName](AActor* Actor)
		{
			if (IsValid(Actor))
			{
				TArray<UNiagaraComponent*> ExistingNCs;
				Actor->GetComponents<UNiagaraComponent>(ExistingNCs);
				for (UNiagaraComponent* NC : ExistingNCs)
				{
					if (IsValid(NC) && NC->ComponentTags.Contains(ConfigTagName))
					{
						NC->DestroyComponent();
					}
				}
			}
		};

		CleanupExistingNC(const_cast<AActor*>(SourceActor));
		CleanupExistingNC(MyTarget);
	}

	UNiagaraComponent* const SpawnedComponent = SkillNiagaraSpawnHelper::SpawnNiagaraBySettings(World, SpawnSettings, SourceTransform, SourceActor, nullptr, Parameters.TargetAttachComponent.Get());
	if (IsValid(SpawnedComponent))
	{
		const AActor* VisionTarget = IsValid(MyTarget) ? MyTarget : SourceActor;

		// 초기 숨김 처리는 RegisterParticle이 등록 시점에 시야 상태로 확정함 (006 합-1/합-2)
		if (CullState == EVfxCullState::SpawnAndTrackVision ||
			CullState == EVfxCullState::SpawnHidden || 
			CullState == EVfxCullState::SpawnAndTrackVisionUntilSeen)
		{
			if (UVisionParticleManagerSubsystem* VisionSubsystem = World->GetSubsystem<UVisionParticleManagerSubsystem>())
			{
				const bool bTrackUntilSeen = (CullState == EVfxCullState::SpawnAndTrackVisionUntilSeen);
				VisionSubsystem->RegisterParticle(SpawnedComponent, const_cast<AActor*>(VisionTarget), bTrackUntilSeen);
			}
		}

		SpawnedComponent->ComponentTags.Add(Parameters.OriginalTag.GetTagName());
		if (IsValid(SpawnConfig))
		{
			SpawnedComponent->ComponentTags.Add(FName(*SpawnConfig->GetPathName()));
		}
		// 나이아가라 시스템의 User.StackCount 파라미터에 현재 스택 카운트 전달
		int32 StackCount = 1;
		if (IsValid(MyTarget) && Parameters.EffectContext.IsValid())
		{
			if (UAbilitySystemComponent* TargetASC = UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(MyTarget))
			{
				FGameplayEffectQuery Query;
				TArray<FActiveGameplayEffectHandle> ActiveHandles = TargetASC->GetActiveEffects(Query);
				for (const FActiveGameplayEffectHandle& ActiveHandle : ActiveHandles)
				{
					const FActiveGameplayEffect* ActiveGE = TargetASC->GetActiveGameplayEffect(ActiveHandle);
					if (ActiveGE && ActiveGE->Spec.GetContext().Get() == Parameters.EffectContext.Get())
					{
						StackCount = ActiveGE->Spec.GetStackCount();
						break;
					}
				}
			}
		}
		SpawnedComponent->SetVariableInt(TEXT("User.StackCount"), StackCount);
	}
	return true;
}

bool UGCN_SpawnNiagaraBySpawnConfig::OnActive_Implementation(AActor* MyTarget, const FGameplayCueParameters& Parameters) const
{
	// 1. 시각 효과 스폰
	OnExecute_Implementation(MyTarget, Parameters);

	// 2. 클라이언트-사이드 이동 동기화 (호스트가 아닌 경우에만 로컬 RootMotionSource 적용)
	if (IsValid(MyTarget) && !MyTarget->HasAuthority())
	{
		ACharacter* const Character = Cast<ACharacter>(MyTarget);
		UCharacterMovementComponent* const CMC = IsValid(Character) ? Character->GetCharacterMovement() : nullptr;

		// 방향(Normal)과 속도(RawMagnitude)가 유효한 경우에만 실행
		if (IsValid(CMC) && !Parameters.Normal.IsNearlyZero() && Parameters.RawMagnitude > 0.0f)
		{
			TSharedPtr<FRootMotionSource_ConstantForce> ConstantForce = MakeShared<FRootMotionSource_ConstantForce>();
			ConstantForce->InstanceName = FName(TEXT("ConstantForceMoveGEC_Client"));
			ConstantForce->AccumulateMode = ERootMotionAccumulateMode::Override;
			ConstantForce->Priority = 5;
			ConstantForce->Force = Parameters.Normal * Parameters.RawMagnitude;
			ConstantForce->Duration = (Parameters.NormalizedMagnitude > 0.0f) ? Parameters.NormalizedMagnitude : 5.0f;
			ConstantForce->FinishVelocityParams.Mode = ERootMotionFinishVelocityMode::MaintainLastRootMotionVelocity;

			CMC->ApplyRootMotionSource(ConstantForce);
		}
	}

	return true;
}

bool UGCN_SpawnNiagaraBySpawnConfig::OnRemove_Implementation(AActor* MyTarget, const FGameplayCueParameters& Parameters) const
{
	if (!IsValid(MyTarget))
	{
		return false;
	}

	const USkillNiagaraSpawnConfig* const SpawnConfig = GetSpawnConfigFromParameters(Parameters);
	if (!IsValid(SpawnConfig) || SpawnConfig->NiagaraSystem.IsNull())
	{
		return false;
	}

	UNiagaraSystem* const LoadedSystem = SpawnConfig->NiagaraSystem.LoadSynchronous();
	if (!IsValid(LoadedSystem))
	{
		return false;
	}

	// 캐릭터, Instigator, EffectCauser, TargetAttachComponent에서 동일한 NiagaraSystem과 SpawnConfig 고유 경로 태그를 가진 컴포넌트를 찾아 Deactivate
	const FName UniqueConfigTagName = FName(*SpawnConfig->GetPathName());
	TArray<AActor*> SearchActors;
	if (IsValid(MyTarget)) SearchActors.AddUnique(MyTarget);
	if (AActor* InstigatorActor = Parameters.Instigator.Get()) SearchActors.AddUnique(InstigatorActor);
	if (AActor* CauserActor = Parameters.EffectCauser.Get()) SearchActors.AddUnique(CauserActor);

	for (AActor* SearchActor : SearchActors)
	{
		TArray<UNiagaraComponent*> NCs;
		SearchActor->GetComponents<UNiagaraComponent>(NCs);
		for (UNiagaraComponent* NC : NCs)
		{
			if (IsValid(NC) && NC->GetAsset() == LoadedSystem)
			{
				if (NC->ComponentTags.Contains(UniqueConfigTagName))
				{
					NC->Deactivate();
				}
			}
		}
	}

	if (USceneComponent* AttachComp = Parameters.TargetAttachComponent.Get())
	{
		TArray<USceneComponent*> Children;
		AttachComp->GetChildrenComponents(true, Children);
		for (USceneComponent* Child : Children)
		{
			if (UNiagaraComponent* NC = Cast<UNiagaraComponent>(Child))
			{
				if (IsValid(NC) && NC->GetAsset() == LoadedSystem && NC->ComponentTags.Contains(UniqueConfigTagName))
				{
					NC->Deactivate();
				}
			}
		}
	}

	// 2. 클라이언트-사이드 이동 종료
	if (IsValid(MyTarget) && !MyTarget->HasAuthority())
	{
		ACharacter* const Character = Cast<ACharacter>(MyTarget);
		if (UCharacterMovementComponent* const CMC = IsValid(Character) ? Character->GetCharacterMovement() : nullptr)
		{
			CMC->RemoveRootMotionSource(FName(TEXT("ConstantForceMoveGEC_Client")));
		}
	}

	return true;
}
