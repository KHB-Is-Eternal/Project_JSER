// Fill out your copyright notice in the Description page of Project Settings.

#include "WatchTagAbility_Base.h"
#include "SkillSystem/SkillConfig/BaseSkillConfig.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystemInterface.h"
#include "Abilities/Tasks/AbilityTask_WaitGameplayEvent.h"
#include "SkillSystem/GameplayEffect/BaseGameplayEffect.h"

UWatchTagAbility_Base::UWatchTagAbility_Base()
{
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
	NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::ServerOnly;
}

void UWatchTagAbility_Base::OnGiveAbility(const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilitySpec& Spec)
{
	Super::OnGiveAbility(ActorInfo, Spec);
	
	PassiveConfig = Cast<UPassiveSkillConfig>(CachedConfig);
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

	// 태그 쿼리 조건이 설정된 경우, 지정된 대상(QueryTarget)에게 검사를 실행합니다.
	if (!PassiveConfig->RequiredTagQuery.IsEmpty())
	{
		const AActor* const QueryActor = ResolveQueryTargetActor(Payload);
		if (QueryActor == nullptr)
		{
			return;
		}

		const UAbilitySystemComponent* const QueryASC = QueryActor->FindComponentByClass<UAbilitySystemComponent>();
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

	// 조건 달성 여부를 자식 클래스에게 위임합니다.
	if (ProcessEventAndCheckCondition(Payload))
	{
		ExecuteTriggerAction(GetAvatarActorFromActorInfo());
	}
}

AActor* UWatchTagAbility_Base::ResolveQueryTargetActor(const FGameplayEventData& Payload) const
{
	switch (PassiveConfig->QueryTarget)
	{
		case EPassiveQueryTarget::Instigator:
			return const_cast<AActor*>(Payload.Instigator.Get());
		case EPassiveQueryTarget::Target:
			return const_cast<AActor*>(Payload.Target.Get());
		case EPassiveQueryTarget::Self:
		default:
			return GetAvatarActorFromActorInfo();
	}
}

void UWatchTagAbility_Base::ExecuteTriggerAction(AActor* TargetActor)
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

	// TriggerAbility가 설정된 경우 어빌리티를 발동합니다 (애니메이션, 하드 CC 처리용).
	if (PassiveConfig->TriggerAbility != nullptr)
	{
		MyASC->TryActivateAbilityByClass(PassiveConfig->TriggerAbility);
		return;
	}

	// TriggerAbility가 없는 경우 이펙트를 직접 적용합니다.
	ApplyTriggerEffects(TargetActor);
}

void UWatchTagAbility_Base::ApplyTriggerEffects(AActor* TargetActor)
{
	if (PassiveConfig->TriggerEffects.IsEmpty() || TargetActor == nullptr)
	{
		return;
	}

	UAbilitySystemComponent* const MyASC = GetAbilitySystemComponentFromActorInfo();
	UAbilitySystemComponent* TargetASC = nullptr;

	if (const IAbilitySystemInterface* const ASCInterface = Cast<IAbilitySystemInterface>(TargetActor))
	{
		TargetASC = ASCInterface->GetAbilitySystemComponent();
	}

	if (TargetASC == nullptr)
	{
		TargetASC = TargetActor->FindComponentByClass<UAbilitySystemComponent>();
	}

	if (MyASC == nullptr || TargetASC == nullptr)
	{
		return;
	}

	FGameplayEffectContextHandle ContextHandle = MyASC->MakeEffectContext();
	ContextHandle.AddInstigator(GetAvatarActorFromActorInfo(), TargetActor);
	ContextHandle.SetAbility(this);

	for (const TSubclassOf<UBaseGameplayEffect>& EffectClass : PassiveConfig->TriggerEffects)
	{
		if (!IsValid(EffectClass))
		{
			continue;
		}

		const FGameplayEffectSpecHandle SpecHandle = MyASC->MakeOutgoingSpec(EffectClass, GetAbilityLevel(), ContextHandle);
		if (SpecHandle.IsValid())
		{
			MyASC->ApplyGameplayEffectSpecToTarget(*SpecHandle.Data.Get(), TargetASC);
		}
	}
}
