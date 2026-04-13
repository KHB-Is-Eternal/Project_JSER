#include "Monster/GAS/GA/GA_MonsterState.h"
#include "Monster/BaseMonster.h"
#include "AbilitySystemComponent.h"

#include "Abilities/Tasks/AbilityTask_WaitGameplayTag.h"
#include "Abilities/Tasks/AbilityTask_PlayMontageAndWait.h"

UGA_MonsterState::UGA_MonsterState()
{
	bReplicateInputDirectly = false;                                    // 예측 실행 하려면..?
	ReplicationPolicy = EGameplayAbilityReplicationPolicy::ReplicateNo; // ReplicateYes
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
	NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::ServerOnly; // LocalPredicted
	NetSecurityPolicy = EGameplayAbilityNetSecurityPolicy::ServerOnly;
	
	bServerRespectsRemoteAbilityCancellation = false;
	bRetriggerInstancedAbility = false;
}

void UGA_MonsterState::OnGiveAbility(const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilitySpec& Spec)
{
	Super::OnGiveAbility(ActorInfo, Spec);

}

void UGA_MonsterState::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);

	if (!CommitAbility(Handle, ActorInfo, ActivationInfo))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	if (bIsUseWaitTag)
	{
		if (StateInitData.WaitTag.IsValid())
		{
			UAbilityTask_WaitGameplayTagRemoved* WaitRemoveTask = UAbilityTask_WaitGameplayTagRemoved::WaitGameplayTagRemove(this, StateInitData.WaitTag);
			WaitRemoveTask->Removed.AddDynamic(this, &UGA_MonsterState::OnTagRemoved);
			WaitRemoveTask->ReadyForActivation();
		}
	}

	ABaseMonster* Monster = Cast<ABaseMonster>(GetOwningActorFromActorInfo());
	if (IsValid(Monster) == false)
	{
		UE_LOG(LogTemp, Warning, TEXT("UGA_MonsterState::ActivateAbility : Not Monster"));
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}
	if (IsValid(Monster->MonsterData) == false)
	{
		UE_LOG(LogTemp, Warning, TEXT("UGA_MonsterState::ActivateAbility : Not MonsterData"));
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	TObjectPtr<UAnimMontage> Montage = *Monster->MonsterData->Montages.Find(StateInitData.MontageType);
	if (IsValid(Montage) == false)
	{
		UE_LOG(LogTemp, Warning, TEXT("UGA_MonsterState::ActivateAbility : Not Montage"));
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}
	
	UAbilityTask_PlayMontageAndWait* WaitPlayMontageTask =
		UAbilityTask_PlayMontageAndWait::CreatePlayMontageAndWaitProxy(
			this,
			NAME_None,
			Montage,
			1.f,
			NAME_None,
			true,
			1.f,
			0.f,
			false
		);

	WaitPlayMontageTask->OnCompleted.AddDynamic(this, &UGA_MonsterState::OnMontageCompleted);
	WaitPlayMontageTask->OnBlendedIn.AddDynamic(this, &UGA_MonsterState::OnMontageBlendIn);
	WaitPlayMontageTask->OnBlendOut.AddDynamic(this, &UGA_MonsterState::OnMontageBlendOut);
	WaitPlayMontageTask->OnInterrupted.AddDynamic(this, &UGA_MonsterState::OnMontageInterrupt);
	WaitPlayMontageTask->OnCancelled.AddDynamic(this, &UGA_MonsterState::OnMontageCancel);
	WaitPlayMontageTask->ReadyForActivation();
	
	
	if (StateInitData.SoundCueTag.IsValid())
	{
		FGameplayCueParameters Params;
		Params.Instigator = Monster;
		ActorInfo->AbilitySystemComponent->ExecuteGameplayCue(StateInitData.SoundCueTag, Params);
	}

	if (StateInitData.NiagaraCueTag.IsValid())
	{
		FGameplayCueParameters Params;
		Params.Instigator = Monster;
		ActorInfo->AbilitySystemComponent->ExecuteGameplayCue(StateInitData.NiagaraCueTag, Params);
	}
}

void UGA_MonsterState::EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled)
{
	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}

void UGA_MonsterState::OnMontageCompleted()
{
}

void UGA_MonsterState::OnMontageBlendIn()
{
}

void UGA_MonsterState::OnMontageBlendOut()
{
}

void UGA_MonsterState::OnMontageInterrupt()
{
}

void UGA_MonsterState::OnMontageCancel()
{
}

void UGA_MonsterState::OnTagRemoved()
{
	Super::EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);

}
