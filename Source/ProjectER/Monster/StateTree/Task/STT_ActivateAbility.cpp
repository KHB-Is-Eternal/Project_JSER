#include "Monster/StateTree/Task/STT_ActivateAbility.h"
#include "StateTreeLinker.h"
#include "StateTreeExecutionContext.h"

#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"

FSTT_ActivateAbility::FSTT_ActivateAbility()
{
	bShouldCallTick = false;
}

bool FSTT_ActivateAbility::Link(FStateTreeLinker& Linker)
{
	Linker.LinkExternalData(ActorHandle);
	return true;
}

const UStruct* FSTT_ActivateAbility::GetInstanceDataType() const
{
	return FInstanceDataType::StaticStruct();
}

EStateTreeRunStatus FSTT_ActivateAbility::EnterState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const
{
	AActor* Actor = Context.GetExternalDataPtr(ActorHandle);
	if (IsValid(Actor) == false)
	{
		UE_LOG(LogTemp, Warning, TEXT("FSTT_ActivateAbility::EnterState : Not ActorHandle"));
		return EStateTreeRunStatus::Failed;
	}
	UAbilitySystemComponent* ASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(Actor);
	if (IsValid(ASC) == false)
	{
		UE_LOG(LogTemp, Warning, TEXT("FSTT_ActivateAbility::EnterState : Not ASC"));
		return EStateTreeRunStatus::Failed;
	}

	FInstanceDataType& InstanceData = Context.GetInstanceData(*this);

	if (InstanceData.AbilityTag.IsValid() == false)
	{
		UE_LOG(LogTemp, Warning, TEXT("FSTT_ActivateAbility::EnterState : Not AbilityTag"));
		return EStateTreeRunStatus::Failed;
	}
	FGameplayTagContainer Container = InstanceData.AbilityTag.GetSingleTagContainer();

	if (ASC->TryActivateAbilitiesByTag(Container) == false)
	{
		UE_LOG(LogTemp, Warning, TEXT("FSTT_ActivateAbility::EnterState : Not ActivateAbilitiesByTag"));
		return EStateTreeRunStatus::Failed;
	}

	return EStateTreeRunStatus::Running;
}

void FSTT_ActivateAbility::ExitState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const
{

}
