// Fill out your copyright notice in the Description page of Project Settings.


#include "SkillBase.h"
#include "AbilitySystemComponent.h"
#include "Abilities/Tasks/AbilityTask_WaitGameplayTag.h"
#include "Abilities/Tasks/AbilityTask_PlayMontageAndWait.h"
#include "Abilities/Tasks/AbilityTask_WaitTargetData.h"
#include "Abilities/Tasks/AbilityTask_WaitGameplayEvent.h"
#include "Abilities/GameplayAbilityTargetActor.h"
#include "CharacterSystem/GAS/AttributeSet/BaseAttributeSet.h"
#include "SkillSystem/AbilityTask/AbilityTask_WaitGameplayEventSyn.h"
#include "SkillSystem/SkillConfig/BaseSkillConfig.h"
#include "SkillSystem/SkillDataAsset.h"
#include "SkillSystem/SkillData.h"
#include "SkillSystem/GameplayEffect/BaseGameplayEffect.h"
#include "SkillSystem/GameplayEffect/GE_SharedCooldown.h"
#include "Monster/BaseMonster.h"
#include "CharacterSystem/Character/BaseCharacter.h"
#include "CharacterSystem/Interface/TargetableInterface.h"
#include "GameModeBase/State/ER_PlayerState.h"

#include "AbilitySystemLog.h" // GAS 관련 로그 확인용
#include "AbilitySystemGlobals.h" // [김현수 추가분] 태그 체크용

#include "CharacterSystem/Player/BasePlayerController.h" // [김현수 추가분]
#include "SkillSystem/GAS/ProjectERGameplayEffectContext.h"
#include "GameFramework/GameStateBase.h"
#include "GameFramework/GameState.h"
#include "SkillSystem/GameplayEffectComponent/BaseGEC.h"

USkillBase::USkillBase()
{
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
	NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::LocalPredicted;
	ReplicationPolicy = EGameplayAbilityReplicationPolicy::ReplicateYes;
	CastingTag = FGameplayTag::RequestGameplayTag(FName("Skill.Animation.Casting"));
	ActiveTag = FGameplayTag::RequestGameplayTag(FName("Skill.Animation.Active"));
	//ActivationBlockedTags.AddTag(CastingTag);
	ActivationBlockedTags.AddTag(ActiveTag);
	ActivationBlockedTags.AddTag(FGameplayTag::RequestGameplayTag(FName("State.Life.Death")));
	ActivationBlockedTags.AddTag(FGameplayTag::RequestGameplayTag(FName("State.Life.Down")));
	// Hard CC: 모든 스킬 차단
	ActivationBlockedTags.AddTag(FGameplayTag::RequestGameplayTag(FName("State.Debuff.Hard.Stun")));
	ActivationBlockedTags.AddTag(FGameplayTag::RequestGameplayTag(FName("State.Debuff.Hard.Airborne")));
	// Soft CC: 침묵은 스킬 사용 차단 (이동은 가능)
	ActivationBlockedTags.AddTag(FGameplayTag::RequestGameplayTag(FName("State.Debuff.Soft.Silence")));
	
	CooldownGameplayEffectClass = UGE_SharedCooldown::StaticClass();
}

void USkillBase::SetSkillTagCount(FGameplayTag Tag, int32 Count)
{
	if (Tag.IsValid() && GetASC()) GetASC()->SetTagMapCount(Tag, Count);
}

void USkillBase::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
	UE_LOG(LogTemp, Warning, TEXT("[SkillBase] ActivateAbility Called! Skill: %s"), *GetName());

	// [김현수 추가분]
	if (AActor* const AvatarActor = GetAvatarActorFromActorInfo())
	{
		if (APlayerController* const PC = Cast<APlayerController>(AvatarActor->GetInstigatorController()))
		{
			if (ABasePlayerController* const BasePC = Cast<ABasePlayerController>(PC))
			{
				if (BasePC->IsCrafting())
				{
					BasePC->CancelCrafting();
				}
			}
		}
	}

	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);
}

void USkillBase::EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled)
{
	if (bWasCancelled) {
		OnCancelAbility();
	}
	SetSkillTagCount(CastingTag, 0);
	SetSkillTagCount(ActiveTag, 0);
	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}

void USkillBase::OnGiveAbility(const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilitySpec& Spec)
{
	Super::OnGiveAbility(ActorInfo, Spec);

	USkillDataAsset* DataAsset = Cast<USkillDataAsset>(Spec.SourceObject);
	CachedConfig = IsValid(DataAsset) ? DataAsset->SkillConfig : nullptr;
	DynamicCostGE = IsValid(CachedConfig) ? CachedConfig->CreateCostGameplayEffect(this) : nullptr;
}

//void USkillBase::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
//{
//	Super::PostEditChangeProperty(PropertyChangedEvent);
//}

void USkillBase::ApplyCooldown(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo) const
{
	UGameplayEffect* CooldownGE = GetCooldownGameplayEffect();

	if (CooldownGE && CachedConfig)
	{
		FGameplayEffectSpecHandle SpecHandle = MakeOutgoingGameplayEffectSpec(CooldownGE->GetClass(), GetAbilityLevel());

		if (SpecHandle.IsValid())
		{
			float Duration = CachedConfig->Data.BaseCoolTime.GetValueAtLevel(GetAbilityLevel());
			
			// Skill Haste (스킬 가속) 반영
			if (UAbilitySystemComponent* ASC = GetASC())
			{
				float Haste = ASC->GetNumericAttribute(UBaseAttributeSet::GetCooldownReductionAttribute());
				// 공식: 최종 쿨타임 = 기본 쿨타임 / (1 + (스킬가속 / 100))
				Duration /= (1.0f + (FMath::Max(Haste, 0.0f) / 100.0f));
				Duration = FMath::Max(Duration, 0.1f); // 최소 쿨타임 보장
			}

			SpecHandle.Data.Get()->SetSetByCallerMagnitude(FGameplayTag::RequestGameplayTag(FName("Skill.Data.CoolTime")), Duration);
			SpecHandle.Data.Get()->DynamicGrantedTags.AppendTags(CachedConfig->Data.CoolTimeTags);
			ApplyGameplayEffectSpecToOwner(Handle, ActorInfo, ActivationInfo, SpecHandle);
		}
	}
}

const FGameplayTagContainer* USkillBase::GetCooldownTags() const
{
	// 데이터 에셋에 쿨타임 태그가 설정되어 있다면 그것을 우선적으로 반환합니다.
	if (CachedConfig && CachedConfig->Data.CoolTimeTags.IsValid())
	{
		return &CachedConfig->Data.CoolTimeTags;
	}

	return nullptr;
}

UGameplayEffect* USkillBase::GetCostGameplayEffect() const
{
	if (IsValid(DynamicCostGE))
	{
		return DynamicCostGE;
	}

	if (IsValid(CachedConfig))
	{
		USkillBase* MutableThis = const_cast<USkillBase*>(this);
		MutableThis->DynamicCostGE = CachedConfig->CreateCostGameplayEffect(MutableThis);

		if (IsValid(DynamicCostGE))
		{
			return DynamicCostGE;
		}
	}

	return Super::GetCostGameplayEffect();
}

void USkillBase::ApplyCost(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo) const
{
	// 1. 동적으로 만든 GE가 있는지 확인
	if (DynamicCostGE && ActorInfo->AbilitySystemComponent.IsValid())
	{
		// 2. 엔진 함수를 거치지 않고 직접 Spec 인스턴스 생성 (중요!)
		// 생성자 파라미터: (UGameplayEffect 인스턴스, Context, 레벨)
		FGameplayEffectSpec* NewSpec = new FGameplayEffectSpec(DynamicCostGE, MakeEffectContext(Handle, ActorInfo), GetAbilityLevel(Handle, ActorInfo));
		FGameplayEffectSpecHandle SpecHandle(NewSpec);

		if (SpecHandle.IsValid())
		{
			FGameplayAbilitySpec* AbilitySpec = ActorInfo->AbilitySystemComponent->FindAbilitySpecFromHandle(Handle);
			ApplyAbilityTagsToGameplayEffectSpec(*SpecHandle.Data.Get(), AbilitySpec);
			ApplyGameplayEffectSpecToOwner(Handle, ActorInfo, ActivationInfo, SpecHandle);
			return;
		}
	}

	
	Super::ApplyCost(Handle, ActorInfo, ActivationInfo);
}

void USkillBase::ExecuteSkill()
{
	auto* ASC = GetASC();
	auto* Avatar = GetAvatar();
	
	if (!ensure(CachedConfig) || !IsValid(ASC) || !IsValid(Avatar))
	{
		EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, true);
		return;
	}

	// 1. 자신에게 효과 적용
	ApplyExcutionEffectToSelf(CachedConfig->GetExecutionEffects());

	// 2. 권한 서버에서 처리할 공통 로직 (예: 이동 정지)
	if (HasAuthority(&CurrentActivationInfo))
	{
		if (ABaseCharacter* Character = Cast<ABaseCharacter>(Avatar))
		{
			Character->StopMove();
		}
	}

	// 3. 디버그 로그 및 클라이언트 전용 이벤트
	if (ASC->IsNetMode(NM_Client))
	{
		UE_LOG(LogTemp, Log, TEXT("[SkillBase] ExecuteSkill on Client. HasValidPredictionKey: %s"), 
			ASC->ScopedPredictionKey.IsValidKey() ? TEXT("True") : TEXT("False"));
	}

	if (IsLocallyControlled())
	{
		OnExecuteSkill_InClient();
	}
}

void USkillBase::OnActiveTagEventReceived(FGameplayEventData Payload)
{
	// [ProjectER] 클라이언트 사이드 예측 실행을 보장하기 위해 예측 창을 엽니다.
	FScopedPredictionWindow ScopedWindow(GetASC(), IsLocallyControlled());

	if (!TryExecuteSkill())
	{
		EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, true);
	}
	else{
		SetSkillTagCount(CastingTag, 0);
		SetSkillTagCount(ActiveTag, 1);
		
		// 클라이언트가 전달한 정확한 시전 시작 시간을 저장 (렉 보상 및 VFX 동기화용)
		this->SyncedActivationTime = Payload.EventMagnitude;

		ExecuteSkill();
	}
}

void USkillBase::OnCastingTagEventReceived(FGameplayEventData Payload)
{
	if (CachedConfig && CachedConfig->Data.bIsUseCasting)
	{
		SetSkillTagCount(CastingTag, 1);
	}
}

void USkillBase::OnMontageInterrupted()
{
	if (IsActive())
	{
		EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, true);
	}
}

void USkillBase::OnMontageCancelled()
{
	if (IsActive())
	{
		EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, true);
	}
}

void USkillBase::OnMontageCompleted()
{
	CompleteFinishSkill();
}

void USkillBase::PlayAnimMontage()
{
	if (!IsValid(CachedConfig) || !IsValid(CachedConfig->Data.AnimMontage)) return;
	UAbilityTask_PlayMontageAndWait* PlayTask = UAbilityTask_PlayMontageAndWait::CreatePlayMontageAndWaitProxy(this, TEXT("SkillAnimation"), CachedConfig->Data.AnimMontage);
	if (!IsValid(PlayTask)) return;

	PlayTask->OnInterrupted.AddDynamic(this, &USkillBase::OnMontageInterrupted);
	PlayTask->OnCancelled.AddDynamic(this, &USkillBase::OnMontageCancelled);
	PlayTask->OnCompleted.AddDynamic(this, &USkillBase::OnMontageCompleted);
	PlayTask->ReadyForActivation();

	ABaseCharacter* BaseCharacter = Cast<ABaseCharacter>(GetAvatar());
	if (!IsValid(BaseCharacter)) return;
	BaseCharacter->StopMove();
}

void USkillBase::SetWaitEventActiveTag()
{
	UAbilityTask_WaitGameplayEventSyn* WaitEventTask = UAbilityTask_WaitGameplayEventSyn::WaitEventClientToServer(this, ActiveTag);
	WaitEventTask->OnEventReceived.AddDynamic(this, &USkillBase::OnActiveTagEventReceived);
	WaitEventTask->ReadyForActivation();
}

void USkillBase::SetWaitEventCastingTag()
{
	UAbilityTask_WaitGameplayEventSyn* WaitEventTask = UAbilityTask_WaitGameplayEventSyn::WaitEventClientToServer(this, CastingTag);
	WaitEventTask->OnEventReceived.AddDynamic(this, &USkillBase::OnCastingTagEventReceived);
	WaitEventTask->ReadyForActivation();
}

void USkillBase::PrepareToActiveSkill()
{
	if (!IsValid(CachedConfig)) return;

	if (!CommitAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo))
	{
		EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
		return;
	}

	SetWaitEventActiveTag();
	if (CachedConfig->Data.bIsUseCasting) SetWaitEventCastingTag();
	PlayAnimMontage();
	//if (IsLocallyControlled() || HasAuthority(&CurrentActivationInfo)) PlayAnimMontage();
}

void USkillBase::ApplyExcutionEffectToSelf(const TArray<TSubclassOf<UBaseGameplayEffect>>& SkillEffectDataAssets, FGameplayEffectContextHandle ContextHandle)
{
	ApplyEffectToTargetInternal(GetASC(), SkillEffectDataAssets, ContextHandle);
}

void USkillBase::ApplyEffectToTargetInternal(UAbilitySystemComponent* TargetASC, const TArray<TSubclassOf<UBaseGameplayEffect>>& Effects, FGameplayEffectContextHandle ContextHandle)
{
	UAbilitySystemComponent* const SourceASC = GetASC();
	AActor* const Avatar = GetAvatar();

	if (!ensure(SourceASC) || !ensure(Avatar) || !IsValid(TargetASC) || Effects.Num() <= 0)
	{
		return;
	}

	// 1. 컨텍스트 초기화 및 커스텀 데이터(시각 동기화) 주입
	if (!ContextHandle.IsValid())
	{
		ContextHandle = SourceASC->MakeEffectContext();
	}
	ContextHandle.AddInstigator(Avatar, Avatar);
	ContextHandle.SetAbility(this);

	if (FProjectERGameplayEffectContext* ERContext = ProjectERContextUtils::GetMutableProjectERContext(ContextHandle))
	{
		if (this->SyncedActivationTime > 0.0f)
		{
			ERContext->ClientActivationTime = this->SyncedActivationTime;
		}
		else if (UWorld* World = GetWorld())
		{
			if (AGameStateBase* GameState = World->GetGameState())
			{
				ERContext->ClientActivationTime = GameState->GetServerWorldTimeSeconds();
			}
		}
	}

	// 2. 각 이팩트 순회하며 적용
	for (const TSubclassOf<UBaseGameplayEffect>& EffectClass : Effects)
	{
		if (!IsValid(EffectClass)) continue;

		// 개별 이펙트용 컨텍스트 독립화 (오염 방지)
		FGameplayEffectContextHandle EffectSpecificContext = ContextHandle.Duplicate();
		FGameplayEffectSpecHandle SpecHandle = SourceASC->MakeOutgoingSpec(EffectClass, GetAbilityLevel(), EffectSpecificContext);
		
		if (SpecHandle.IsValid())
		{
			// [GEC Hook] Phase 1, 2: 좌표 보정 및 클라이언트 예측 비주얼 생성
			if (const UBaseGameplayEffect* GEInstance = Cast<UBaseGameplayEffect>(EffectClass->GetDefaultObject()))
			{
				for (const TObjectPtr<UGameplayEffectComponent>& Component : GEInstance->GetGEComponents())
				{
					if (const UBaseGEC* BaseGEC = Cast<UBaseGEC>(Component))
					{
						FGameplayEffectContextHandle SpecContext = SpecHandle.Data.Get()->GetContext();

						BaseGEC->PreApplyEffect(SourceASC, SpecContext, *SpecHandle.Data.Get());
						
						if (IsLocallyControlled())
						{
							BaseGEC->OnExecutePredictive(SourceASC, SpecContext, *SpecHandle.Data.Get());
						}
					}
				}
			}

			// Phase 3: 최종 적용
			SourceASC->ApplyGameplayEffectSpecToTarget(*SpecHandle.Data.Get(), TargetASC);
		}
	}
}

bool USkillBase::TryExecuteSkill()
{
	auto* ASC = GetASC();
	if (!IsValid(ASC) || !IsValid(CachedConfig)) {
		return false;
	}

	if (CachedConfig->Data.bIsUseCasting && ASC->HasMatchingGameplayTag(CastingTag) == false){
		return false;
	}

	FGameplayTagContainer RelevantTags;
	if (!DoesAbilitySatisfyTagRequirements(*ASC, nullptr, nullptr, &RelevantTags)){
		// 만약 차단된 원인이 오직 ActiveTag 하나뿐이라면 (글로벌 차단 태그 제외), 통과시킵니다.
		RelevantTags.RemoveTag(UAbilitySystemGlobals::Get().ActivateFailTagsBlockedTag);
		if (RelevantTags.Num() == 1 && RelevantTags.HasTag(ActiveTag))
		{
			return true;
		}
		return false;
	}

	return true;
}

void USkillBase::CompleteFinishSkill()
{
	//SetSkillTagCount(ActiveTag, 0);
	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
}

FGameplayTag USkillBase::GetInputTag()
{
	return CachedConfig ? CachedConfig->Data.InputKeyTag : FGameplayTag();
}

ETargetRelationship USkillBase::GetSkillTargetRelationship()
{
	return CachedConfig ? CachedConfig->Data.ApplyTo : ETargetRelationship::None;
}

bool USkillBase::IsValidRelationship(AActor* Target)
{
	if (!IsValid(Target) || !IsValid(CachedConfig)) return false;

	auto* Instigator = GetAvatar();
	auto* I_Instigator = Cast<ITargetableInterface>(Instigator);
	auto* I_Target = Cast<ITargetableInterface>(Target);

	bool IsInstigatorImplementsInterface = Instigator->GetClass()->ImplementsInterface(UTargetableInterface::StaticClass());
	bool TargetImplementsInterface = Target->GetClass()->ImplementsInterface(UTargetableInterface::StaticClass());

	if (!IsInstigatorImplementsInterface || !TargetImplementsInterface)
	{
		return false;
	}

	//if (!IsValid(I_Instigator) || !IsValid()) return false;
	if (!I_Instigator || !I_Target) return false;

	bool bIsSameTeam = (I_Instigator->GetTeamType() == I_Target->GetTeamType());
	const ETargetRelationship& Relationship = CachedConfig->Data.ApplyTo;

	if (Relationship == ETargetRelationship::Friend) return bIsSameTeam;
	if (Relationship == ETargetRelationship::Enemy)  return !bIsSameTeam && I_Target->IsTargetable();

	return false;
}

void USkillBase::OnCancelAbility()
{

}

void USkillBase::OnExecuteSkill_InClient()
{

}