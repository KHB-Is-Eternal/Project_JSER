// Fill out your copyright notice in the Description page of Project Settings.

#include "WatchTagAbility_Base.h"
#include "SkillSystem/SkillConfig/BaseSkillConfig.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystemInterface.h"
#include "Abilities/Tasks/AbilityTask_WaitGameplayEvent.h"
#include "SkillSystem/GameplayEffect/BaseGameplayEffect.h"
#include "AbilitySystemGlobals.h"
#include "SkillSystem/SkillDataAsset.h"
#include "CharacterSystem/GameplayTags/GameplayTags.h"

UWatchTagAbility_Base::UWatchTagAbility_Base()
{
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
	NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::ServerOnly;
}

void UWatchTagAbility_Base::OnGiveAbility(const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilitySpec& Spec)
{
	Super::OnGiveAbility(ActorInfo, Spec);
	
	PassiveConfig = Cast<UPassiveSkillConfig>(CachedConfig);

	if (ActorInfo != nullptr)
	{
		UAbilitySystemComponent* const MyASC = ActorInfo->AbilitySystemComponent.Get();
		if (MyASC != nullptr)
		{
			if (IsValid(PassiveConfig) && !PassiveConfig->TriggerAbility.IsNull())
			{
				USkillDataAsset* SkillAsset = PassiveConfig->TriggerAbility.LoadSynchronous();
				if (IsValid(SkillAsset) && IsValid(SkillAsset->SkillConfig))
				{
					FGameplayAbilitySpec TriggerSpec = SkillAsset->MakeSpec();
					TriggerSpec.Level = GetAbilityLevel();
					GrantedTriggerAbilityHandle = MyASC->GiveAbility(TriggerSpec);
				}
			}

			// 사망 태그 감지 델리게이트 등록 (부활 시 재활성화를 위함)
			MyASC->RegisterGameplayTagEvent(ProjectER::State::Life::Death, EGameplayTagEventType::NewOrRemoved)
				.AddUObject(this, &UWatchTagAbility_Base::OnDeathTagChanged, Spec.Handle);

			MyASC->TryActivateAbility(Spec.Handle);
		}
	}
}

void UWatchTagAbility_Base::OnRemoveAbility(const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilitySpec& Spec)
{
	if (ActorInfo != nullptr)
	{
		UAbilitySystemComponent* const MyASC = ActorInfo->AbilitySystemComponent.Get();
		if (MyASC != nullptr)
		{
			// 사망 태그 감시 델리게이트 등록 해제
			MyASC->RegisterGameplayTagEvent(ProjectER::State::Life::Death, EGameplayTagEventType::NewOrRemoved)
				.RemoveAll(this);

			if (GrantedTriggerAbilityHandle.IsValid())
			{
				MyASC->ClearAbility(GrantedTriggerAbilityHandle);
				GrantedTriggerAbilityHandle = FGameplayAbilitySpecHandle();
			}
		}
	}

	Super::OnRemoveAbility(ActorInfo, Spec);
}

void UWatchTagAbility_Base::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);

	if (!HasAuthority(&ActivationInfo))
	{
		return;
	}

	if (!PassiveConfig && CachedConfig)
	{
		PassiveConfig = Cast<UPassiveSkillConfig>(CachedConfig);
	}

	if (!IsValid(PassiveConfig) || PassiveConfig->EventTagsToWatch.IsEmpty())
	{
		UE_LOG(LogTemp, Warning, TEXT("UWatchTagAbility_Base: PassiveConfig가 없거나 EventTagsToWatch가 비어있습니다."));
		return;
	}

	for (const FGameplayTag& EventTag : PassiveConfig->EventTagsToWatch)
	{
		if (!EventTag.IsValid())
		{
			continue;
		}

		UAbilityTask_WaitGameplayEvent* WaitEventTask = UAbilityTask_WaitGameplayEvent::WaitGameplayEvent(this, EventTag);
		if (WaitEventTask != nullptr)
		{
			WaitEventTask->EventReceived.AddDynamic(this, &UWatchTagAbility_Base::OnEventReceived);
			WaitEventTask->ReadyForActivation();
		}
	}
}

void UWatchTagAbility_Base::OnEventReceived(FGameplayEventData Payload)
{
	if (!IsValid(PassiveConfig))
	{
		return;
	}

	// [Option A] 쿨타임 중이면 누적 및 발동을 완전 차단합니다.
	UAbilitySystemComponent* const MyASC = GetAbilitySystemComponentFromActorInfo();
	if (MyASC != nullptr && GetCooldownTags() != nullptr)
	{
		if (MyASC->HasAnyMatchingGameplayTags(*GetCooldownTags()))
		{
			return;
		}
	}

	// 공통 발동 대상(QueryActor)을 미리 추출
	AActor* const QueryActor = ResolveQueryTargetActor(Payload, PassiveConfig->QueryTarget);

	// 태그 쿼리 조건이 설정된 경우, 지정된 대상(QueryTarget)에게 검사를 실행합니다.
	if (!PassiveConfig->RequiredTagQuery.IsEmpty())
	{
		if (QueryActor == nullptr)
		{
			return;
		}

		const UAbilitySystemComponent* const QueryASC = UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(QueryActor);
		if (QueryASC == nullptr)
		{
			return;
		}

		FGameplayTagContainer QueryActorTags;
		QueryASC->GetOwnedGameplayTags(QueryActorTags);

		if (!PassiveConfig->RequiredTagQuery.Matches(QueryActorTags))
		{
			return;
		}
	}

	// 추가적인 스탯(Attribute) 발동 조건 검사
	if (!CheckAttributeConditions(Payload))
	{
		return;
	}

	// 조건 달성 여부를 자식 클래스에게 위임합니다.
	float FinalMagnitude = 0.0f;
	if (ProcessEventAndCheckCondition(Payload, FinalMagnitude))
	{
		ExecuteTriggerAction(QueryActor, FinalMagnitude);
	}
}

AActor* UWatchTagAbility_Base::ResolveQueryTargetActor(const FGameplayEventData& Payload, EPassiveQueryTarget TargetType) const
{
	switch (TargetType)
	{
		case EPassiveQueryTarget::Instigator:
			return const_cast<AActor*>(Payload.Instigator.Get());
		case EPassiveQueryTarget::Target:
		{
			if (Payload.Target.Get() != nullptr)
			{
				return const_cast<AActor*>(Payload.Target.Get());
			}
			if (Payload.ContextHandle.IsValid())
			{
				if (const FHitResult* HitResult = Payload.ContextHandle.GetHitResult())
				{
					return HitResult->GetActor();
				}
			}
			return nullptr;
		}
		case EPassiveQueryTarget::Self:
		default:
			return GetAvatarActorFromActorInfo();
	}
}

bool UWatchTagAbility_Base::CheckAttributeConditions(const FGameplayEventData& Payload) const
{
	if (!IsValid(PassiveConfig) || PassiveConfig->RequiredAttributeConditions.IsEmpty())
	{
		return true;
	}

	UAbilitySystemComponent* const MyASC = GetAbilitySystemComponentFromActorInfo();
	if (MyASC == nullptr)
	{
		return false;
	}

	for (const FAttributeCondition& Condition : PassiveConfig->RequiredAttributeConditions)
	{
		// 1. 태그 필터링 (SourceTags & TargetTags)
		UAbilitySystemComponent* InstigatorASC = nullptr;
		if (Payload.Instigator.Get() != nullptr)
		{
			InstigatorASC = UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(const_cast<AActor*>(Payload.Instigator.Get()));
		}
		if (InstigatorASC != nullptr && !Condition.Modifier.SourceTags.IsEmpty())
		{
			FGameplayTagContainer InstigatorTags;
			InstigatorASC->GetOwnedGameplayTags(InstigatorTags);
			if (!Condition.Modifier.SourceTags.RequirementsMet(InstigatorTags))
			{
				return false;
			}
		}

		AActor* TargetActorForTags = ResolveQueryTargetActor(Payload, EPassiveQueryTarget::Target);
		UAbilitySystemComponent* TargetASCForTags = nullptr;
		if (TargetActorForTags != nullptr)
		{
			TargetASCForTags = UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(TargetActorForTags);
		}
		if (TargetASCForTags != nullptr && !Condition.Modifier.TargetTags.IsEmpty())
		{
			FGameplayTagContainer TargetTags;
			TargetASCForTags->GetOwnedGameplayTags(TargetTags);
			if (!Condition.Modifier.TargetTags.RequirementsMet(TargetTags))
			{
				return false;
			}
		}

		// 2. 스탯 시뮬레이션 평가 (Modifier Attribute가 세팅된 경우만)
		if (Condition.Modifier.Attribute.IsValid())
		{
			const AActor* const StatTargetActor = ResolveQueryTargetActor(Payload, Condition.QueryTarget);
			if (StatTargetActor == nullptr)
			{
				return false;
			}

			const UAbilitySystemComponent* const StatTargetASC = UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(const_cast<AActor*>(StatTargetActor));
			if (StatTargetASC == nullptr)
			{
				return false;
			}

			if (!StatTargetASC->HasAttributeSetForAttribute(Condition.Modifier.Attribute))
			{
				return false;
			}

			// 항상 버프가 반영된 현재 스탯(Current Value)을 추출
			const float OriginalValue = StatTargetASC->GetNumericAttribute(Condition.Modifier.Attribute);

			// ModifierMagnitude 평가를 위한 임시 Spec 생성
			const UGameplayEffect* DummyGE = GetDefault<UGameplayEffect>();
			FGameplayEffectContextHandle Context = MyASC->MakeEffectContext();
			AActor* Instigator = const_cast<AActor*>(Payload.Instigator.Get());
			AActor* EffectCauser = Payload.ContextHandle.IsValid() ? Payload.ContextHandle.GetEffectCauser() : Instigator;
			if (EffectCauser == nullptr)
			{
				EffectCauser = Instigator;
			}
			Context.AddInstigator(Instigator, EffectCauser);
			FGameplayEffectSpec TempSpec(DummyGE, Context, GetAbilityLevel());

			float EvaluatedMagnitude = 0.0f;
			Condition.Modifier.ModifierMagnitude.AttemptCalculateMagnitude(TempSpec, EvaluatedMagnitude, false);

			// ModifierOp에 따른 시뮬레이션 값 도출
			float SimulatedValue = OriginalValue;
			switch (Condition.Modifier.ModifierOp)
			{
				case EGameplayModOp::Additive:
					SimulatedValue = OriginalValue + EvaluatedMagnitude;
					break;
				case EGameplayModOp::Multiplicitive:
					SimulatedValue = OriginalValue * EvaluatedMagnitude;
					break;
				case EGameplayModOp::Division:
					if (!FMath::IsNearlyZero(EvaluatedMagnitude))
					{
						SimulatedValue = OriginalValue / EvaluatedMagnitude;
					}
					break;
				case EGameplayModOp::Override:
					SimulatedValue = EvaluatedMagnitude;
					break;
			}

			const float Threshold = Condition.ThresholdValue.GetValueAtLevel(GetAbilityLevel());

			bool bPassed = false;
			switch (Condition.CompareType)
			{
				case EAttributeCompareType::GreaterThan:
					bPassed = (SimulatedValue > Threshold);
					break;
				case EAttributeCompareType::GreaterThanOrEqual:
					bPassed = (SimulatedValue >= Threshold);
					break;
				case EAttributeCompareType::LessThan:
					bPassed = (SimulatedValue < Threshold);
					break;
				case EAttributeCompareType::LessThanOrEqual:
					bPassed = (SimulatedValue <= Threshold);
					break;
				case EAttributeCompareType::Equal:
					bPassed = FMath::IsNearlyEqual(SimulatedValue, Threshold);
					break;
			}

			if (!bPassed)
			{
				return false;
			}
		}
	}

	return true;
}

void UWatchTagAbility_Base::ExecuteTriggerAction(AActor* TargetActor, float EventMagnitude)
{
	if (!IsValid(PassiveConfig))
	{
		return;
	}

	UAbilitySystemComponent* const MyASC = GetAbilitySystemComponentFromActorInfo();
	if (MyASC == nullptr)
	{
		return;
	}

	// 발동 시 쿨타임 적용 (USkillBase의 쿨타임 로직 재사용)
	CommitAbilityCooldown(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, false);

	// 동적으로 부여된 TriggerAbility가 존재하면 실행
	if (GrantedTriggerAbilityHandle.IsValid())
	{
		MyASC->TryActivateAbility(GrantedTriggerAbilityHandle);
	}

	// TriggerAbility 활성화 시도 후, TriggerEffects도 함께 적용
	ApplyTriggerEffects(TargetActor, EventMagnitude);
}

void UWatchTagAbility_Base::ApplyTriggerEffects(AActor* TargetActor, float EventMagnitude)
{
	if (PassiveConfig->Effects.IsEmpty() || TargetActor == nullptr)
	{
		return;
	}

	UAbilitySystemComponent* const TargetASC = UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(TargetActor);
	if (TargetASC == nullptr)
	{
		return;
	}

	// 훅에서 쓸 수 있도록 캐싱
	CurrentEventMagnitude = EventMagnitude;

	// 부모의 함수를 원터치로 호출 (파라미터 추가 없음!)
	// ContextHandle은 빈 값을 넘기면 내부에서 알아서 생성합니다.
	ApplyEffectToTargetInternal(TargetASC, PassiveConfig->Effects, PassiveConfig->MagnitudeCalculators);
}

void UWatchTagAbility_Base::OnEffectSpecCreated(FGameplayEffectSpecHandle& SpecHandle) const
{
	if (IsValid(PassiveConfig) && PassiveConfig->SetByCallerTag.IsValid())
	{
		SpecHandle.Data.Get()->SetSetByCallerMagnitude(PassiveConfig->SetByCallerTag, CurrentEventMagnitude);
	}
}

void UWatchTagAbility_Base::OnDeathTagChanged(const FGameplayTag Tag, int32 NewCount, FGameplayAbilitySpecHandle SpecHandle)
{
	// 사망 태그가 제거되었고(부활함), 현재 패시브가 비활성화 상태인 경우 재활성화
	if (NewCount == 0 && !IsActive())
	{
		UAbilitySystemComponent* const MyASC = GetAbilitySystemComponentFromActorInfo();
		if (MyASC != nullptr)
		{
			MyASC->TryActivateAbility(SpecHandle);
		}
	}
}


