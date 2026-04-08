// Fill out your copyright notice in the Description page of Project Settings.


#include "SkillSystem/GameplayEffectComponent/LaunchHomingMissile.h"
#include "SkillSystem/Actor/BaseMissileActor/BaseMissileActor.h"
#include "SkillSystem/GameplayEffect/BaseGameplayEffect.h"
#include "SkillSystem/GameAbility/SkillBase.h"
#include "SkillSystem/SkillNiagaraSpawnConfig.h"
#include "SkillSystem/SkillSoundSpawnConfig.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "GameplayEffect.h"
#include "GameplayEffectTypes.h"
#include "GameFramework/Character.h"
#include "Components/SkeletalMeshComponent.h"
#include "SkillSystem/GAS/ProjectERGameplayEffectContext.h"
#include "GameFramework/GameStateBase.h"
#include "GameFramework/GameState.h"

// ============================================================================
// GEC
// ============================================================================

ULaunchHomingMissile::ULaunchHomingMissile()
{
}

void ULaunchHomingMissile::CollectCueConfigs(TArray<const UObject*>& OutConfigs) const
{
	// 미사일 발사 시 동반되는 효과들을 수집합니다. (Phase 2)
	if (MissileVfx) OutConfigs.Add(MissileVfx);
	if (MissileSound) OutConfigs.Add(MissileSound);
}

void ULaunchHomingMissile::OnGameplayEffectApplied(
	FActiveGameplayEffectsContainer& ActiveGEContainer,
	FGameplayEffectSpec& GESpec,
	FPredictionKey& PredictionKey) const
{
	Super::OnGameplayEffectApplied(ActiveGEContainer, GESpec, PredictionKey);

	// --- Context 검증 ---
	const FGameplayEffectContextHandle& ContextHandle = GESpec.GetEffectContext();
	const FGameplayEffectContext* EffectContext = ContextHandle.Get();
	if (!EffectContext)
	{
		return;
	}

	AActor* const Instigator = IsValid(ContextHandle.GetInstigator())
		? ContextHandle.GetInstigator()
		: ContextHandle.GetEffectCauser();
	if (!IsValid(Instigator))
	{
		return;
	}

	UWorld* const World = Instigator->GetWorld();
	if (!IsValid(World))
	{
		return;
	}

	// --- 클래스 세팅 검증 ---
	if (!IsValid(this->MissileActorClass))
	{
		return;
	}

	// --- 타겟 및 스폰 위치 계산 ---
	// 1순위: HitResult에서 타겟 추출
	AActor* TargetActor = nullptr;
	if (const FHitResult* HitResult = ContextHandle.GetHitResult())
	{
		TargetActor = HitResult->GetActor();
	}
	// 2순위: HitResult가 없거나 Actor가 없으면 GE가 적용된 대상(Owner)을 타겟으로 사용
	if (!IsValid(TargetActor))
	{
		TargetActor = GetTargetActorFromContainer(ActiveGEContainer);
	}

	if (!IsValid(TargetActor))
	{
		return;
	}

	const FTransform SpawnTransform = CalculateSpawnTransform(Instigator, TargetActor);

	// 2. 권한 확인: 실제 액터 소환은 서버에서만 수행합니다.
	if (!ActiveGEContainer.Owner || !ActiveGEContainer.Owner->IsOwnerActorAuthoritative())
	{
		return;
	}

	// --- 미사일 액터 지연 생성 ---
	APawn* const SpawnInstigator = Cast<APawn>(ContextHandle.GetInstigator());
	ABaseMissileActor* const MissileActor = World->SpawnActorDeferred<ABaseMissileActor>(
		this->MissileActorClass,
		SpawnTransform,
		Instigator,
		SpawnInstigator,
		ESpawnActorCollisionHandlingMethod::AlwaysSpawn);
	if (!IsValid(MissileActor))
	{
		return;
	}

	// --- 효과 Spec 생성 --- 
	UAbilitySystemComponent* const CauserASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(Instigator);
	USkillBase* const Skill = const_cast<USkillBase*>(Cast<USkillBase>(ContextHandle.GetAbility()));

	TArray<FGameplayEffectSpecHandle> EffectSpecs;
	if (IsValid(CauserASC) && IsValid(Skill))
	{
		for (const TSubclassOf<UBaseGameplayEffect>& EffectClass : this->Applied)
		{
			if (IsValid(EffectClass))
			{
				FGameplayEffectSpecHandle SpecHandle = CauserASC->MakeOutgoingSpec(EffectClass, GESpec.GetLevel(), ContextHandle);
				if (SpecHandle.IsValid())
				{
					EffectSpecs.Add(SpecHandle);
				}
			}
		}

		// 강화 효과(SkillProc) 확인 및 전이
		UBaseGEC::GetSkillProcEffects(CauserASC, Skill, MissileActor, ContextHandle, EffectSpecs);
	}
	
	// --- 적중 효과(VFX, Sound) 큐 파라미터 구성 ---
	// 적중 효과는 로컬 예측이 필수적이지 않으므로 기존 방식(InitializeMissile 시 전달)을 유지합니다. 
	FGameplayCueParameters HitVfxCueParams(GESpec);
	if (IsValid(this->ImpactVfx) && this->ImpactVfx->CueTag.IsValid())
	{
		HitVfxCueParams.OriginalTag = this->ImpactVfx->CueTag;
		HitVfxCueParams.Instigator = ContextHandle.GetInstigator();
		HitVfxCueParams.EffectCauser = MissileActor;
		HitVfxCueParams.Location = SpawnTransform.GetLocation();
		HitVfxCueParams.SourceObject = this->ImpactVfx;
		HitVfxCueParams.GameplayEffectLevel = GESpec.GetLevel();
	}

	FGameplayCueParameters HitSoundCueParams(GESpec);
	if (IsValid(this->ImpactSound) && this->ImpactSound->CueTag.IsValid())
	{
		HitSoundCueParams.OriginalTag = this->ImpactSound->CueTag;
		HitSoundCueParams.Instigator = ContextHandle.GetInstigator();
		HitSoundCueParams.EffectCauser = MissileActor;
		HitSoundCueParams.Location = SpawnTransform.GetLocation();
		HitSoundCueParams.SourceObject = this->ImpactSound;
		HitSoundCueParams.GameplayEffectLevel = GESpec.GetLevel();
	}

	// --- 미사일 초기화 ---
	// SpawnTransform의 방향을 직접 추출 (FinishSpawning 이전이므로 GetActorForwardVector() 불신뢰)
	const FVector InitialDirection = SpawnTransform.GetRotation().GetForwardVector();

	MissileActor->InitializeMissile(
		EffectSpecs,
		Instigator,
		TargetActor,
		HitVfxCueParams,
		HitSoundCueParams,
		this->InitialSpeed,
		this->MaxSpeed,
		this->HomingAccelerationMagnitude,
		this->ReachThreshold,
		this->bDestroyOnHit,
		InitialDirection
	);
	MissileActor->SetLifeSpan(this->LifeSpan);

	// --- 스폰 완료 ---
	MissileActor->FinishSpawning(SpawnTransform);

	// --- 렉 보상 (Lag Compensation / Fast-Forward) ---
	if (const FProjectERGameplayEffectContext* ERContext = static_cast<const FProjectERGameplayEffectContext*>(ContextHandle.Get()))
	{
		if (ERContext->ClientActivationTime > 0.0f)
		{
			if (AGameStateBase* GameState = World->GetGameState())
			{
				float ServerTime = GameState->GetServerWorldTimeSeconds();
				float Latency = ServerTime - ERContext->ClientActivationTime;
				if (Latency > 0.0f)
				{
					FVector Velocity = MissileActor->GetVelocity();
					if (!Velocity.IsNearlyZero())
					{
						FVector Correction = Velocity * Latency;
						MissileActor->AddActorWorldOffset(Correction);
					}
				}
			}
		}
	}

	// --- 시각 효과 실행 ---
	// [수정] 네이티브 GameplayCue 시스템이 UProjectERASC를 통해 자동으로 Config를 주입하여 실행합니다. (Phase 2)
	// 시전자 관련 효과는 몽타주 AnimNotify에서 담당합니다. (Phase 3)
	ExecuteVfx(GESpec, ContextHandle, Instigator, MissileActor);
	ExecuteSound(GESpec, ContextHandle, Instigator, MissileActor);
}

FTransform ULaunchHomingMissile::CalculateSpawnTransform(
	const AActor* Instigator,
	const AActor* TargetActor) const
{
	if (!IsValid(Instigator))
	{
		return FTransform::Identity;
	}

	FVector SpawnLocation = Instigator->GetActorLocation();
	FRotator SpawnRotation = Instigator->GetActorRotation();

	// 1. 본(Bone) 위치 사용
	if (!this->BoneName.IsNone())
	{
		const ACharacter* Character = Cast<ACharacter>(Instigator);
		const USkeletalMeshComponent* Mesh = Character
			? Character->GetMesh()
			: Instigator->FindComponentByClass<USkeletalMeshComponent>();

		if (IsValid(Mesh) && Mesh->DoesSocketExist(this->BoneName))
		{
			SpawnLocation = Mesh->GetSocketLocation(this->BoneName);
		}
	}

	// 2. 타겟을 향한 방향 계산
	if (IsValid(TargetActor))
	{
		// 타겟의 중심이나 원격 위치 대신, 실제 유도 지점인 RootComponent의 위치를 정확히 조준합니다.
		const FVector TargetLocation = TargetActor->GetRootComponent()
			? TargetActor->GetRootComponent()->GetComponentLocation()
			: TargetActor->GetActorLocation();

		FVector Dir = TargetLocation - SpawnLocation;
		if (!Dir.IsNearlyZero())
		{
			SpawnRotation = Dir.Rotation();
		}
	}

	return FTransform(SpawnRotation, SpawnLocation);
}

AActor* ULaunchHomingMissile::GetTargetActorFromContainer(FActiveGameplayEffectsContainer& ActiveGEContainer) const
{
	return ActiveGEContainer.Owner ? ActiveGEContainer.Owner->GetOwner() : nullptr;
}

void ULaunchHomingMissile::ExecuteVfx(
	const FGameplayEffectSpec& GESpec,
	const FGameplayEffectContextHandle& ContextHandle,
	AActor* Instigator,
	ABaseMissileActor* MissileActor) const
{
	// [수정] 수동으로 GameplayCue를 실행하던 로직을 제거합니다. 
}

void ULaunchHomingMissile::ExecuteSound(
	const FGameplayEffectSpec& GESpec,
	const FGameplayEffectContextHandle& ContextHandle,
	AActor* Instigator,
	ABaseMissileActor* MissileActor) const
{
	// [수정] 수동으로 GameplayCue를 실행하던 로직을 제거합니다. 
}
