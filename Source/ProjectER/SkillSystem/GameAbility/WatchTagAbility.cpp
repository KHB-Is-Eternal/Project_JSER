// Fill out your copyright notice in the Description page of Project Settings.

#include "WatchTagAbility.h"
#include "AbilitySystemComponent.h"
#include "Abilities/Tasks/AbilityTask_WaitGameplayEvent.h"
#include "AbilitySystemInterface.h"
#include "SkillSystem/GameplayEffect/BaseGameplayEffect.h"

UWatchTagAbility::UWatchTagAbility()
{
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
	NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::ServerOnly; // 패시브 이벤트 판정은 서버 신뢰
}

void UWatchTagAbility::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);

	if (!HasAuthority(&ActivationInfo) || !EventTagToWatch.IsValid())
	{
		return;
	}

	// 무한 대기 Task 생성
	UAbilityTask_WaitGameplayEvent* WaitEventTask = UAbilityTask_WaitGameplayEvent::WaitGameplayEvent(this, EventTagToWatch);
	if (WaitEventTask)
	{
		WaitEventTask->EventReceived.AddDynamic(this, &UWatchTagAbility::OnEventReceived);
		WaitEventTask->ReadyForActivation();
	}
}

void UWatchTagAbility::OnEventReceived(FGameplayEventData Payload)
{
	AActor* TargetToApply = nullptr;

	if (bApplyToTriggerInstigator)
	{
		// 이벤트를 발생시킨 주체에게 적용 (Payload.Instigator)
		TargetToApply = const_cast<AActor*>(Payload.Instigator.Get());
	}
	else
	{
		// 나 자신(어빌리티 주인)에게 적용
		TargetToApply = GetAvatarActorFromActorInfo();
	}

	if (TargetToApply)
	{
		ApplySkillEffects(TargetToApply);
	}
}

void UWatchTagAbility::ApplySkillEffects(AActor* TargetActor)
{
	UAbilitySystemComponent* MyASC = GetAbilitySystemComponentFromActorInfo();
	UAbilitySystemComponent* TargetASC = TargetActor ? TargetActor->FindComponentByInterface<IAbilitySystemInterface>() ? Cast<IAbilitySystemInterface>(TargetActor)->GetAbilitySystemComponent() : nullptr : nullptr;
	
	// 인터페이스 미사용 시 일반 컴포넌트 찾기 시도
	if (!TargetASC && TargetActor)
	{
		TargetASC = TargetActor->FindComponentByClass<UAbilitySystemComponent>();
	}

	if (!MyASC || !TargetASC || SkillEffectDataAssets.Num() <= 0)
	{
		return;
	}

	FGameplayEffectContextHandle ContextHandle = MyASC->MakeEffectContext();
	ContextHandle.AddInstigator(GetAvatarActorFromActorInfo(), TargetActor);
	ContextHandle.SetAbility(this);

	for (const TSubclassOf<UBaseGameplayEffect>& EffectClass : SkillEffectDataAssets)
	{
		if (!IsValid(EffectClass)) continue;

		FGameplayEffectSpecHandle SpecHandle = MyASC->MakeOutgoingSpec(EffectClass, GetAbilityLevel(), ContextHandle);
		if (SpecHandle.IsValid())
		{
			MyASC->ApplyGameplayEffectSpecToTarget(*SpecHandle.Data.Get(), TargetASC);
		}
	}
}
