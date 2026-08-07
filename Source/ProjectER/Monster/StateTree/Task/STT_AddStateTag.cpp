#include "Monster/StateTree/Task/STT_AddStateTag.h"
#include "StateTreeLinker.h"
#include "StateTreeExecutionContext.h"

#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "Monster/GAS/GE/GE_AddTag.h"

FSTT_AddStateTag::FSTT_AddStateTag()
{
	bShouldCallTick = false;
}

bool FSTT_AddStateTag::Link(FStateTreeLinker& Linker)
{
	Linker.LinkExternalData(ActorHandle);
	return true;
}

const UStruct* FSTT_AddStateTag::GetInstanceDataType() const
{
	return FInstanceDataType::StaticStruct();
}

EStateTreeRunStatus FSTT_AddStateTag::EnterState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const
{
	AActor* Actor = Context.GetExternalDataPtr(ActorHandle);
	if (IsValid(Actor) == false)
	{
		UE_LOG(LogTemp, Warning, TEXT("FSTT_AddStateTag::EnterState : Not ActorHandle"));
		return EStateTreeRunStatus::Failed;
	}
	UAbilitySystemComponent* ASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(Actor);
	if (IsValid(ASC) == false)
	{
		UE_LOG(LogTemp, Warning, TEXT("FSTT_AddStateTag::EnterState : Not ASC"));
		return EStateTreeRunStatus::Failed;
	}

	FInstanceDataType& InstanceData = Context.GetInstanceData(*this);
	
	TSubclassOf<UGE_AddTag> Effect = UGE_AddTag::StaticClass();
	FGameplayEffectContextHandle EffectContextHandle = ASC->MakeEffectContext();
	FGameplayEffectSpecHandle EffectSpecHandle = ASC->MakeOutgoingSpec(Effect, 1.0f, EffectContextHandle);
	if (EffectSpecHandle.IsValid() == false)
	{
		UE_LOG(LogTemp, Warning, TEXT("FSTT_AddStateTag::EnterState : Not EffectSpecHandle"));
		return EStateTreeRunStatus::Failed;
	}
	if (InstanceData.StateTag.IsValid() == false)
	{
		UE_LOG(LogTemp, Warning, TEXT("FSTT_AddStateTag::EnterState : Not StateTag"));
		return EStateTreeRunStatus::Failed;
	}

	EffectSpecHandle.Data->DynamicGrantedTags.AddTag(InstanceData.StateTag);
	InstanceData.ActiveEffectHandle = ASC->ApplyGameplayEffectSpecToSelf(*EffectSpecHandle.Data);
	
	return EStateTreeRunStatus::Running;
}

void FSTT_AddStateTag::ExitState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const
{
	AActor* Actor = Context.GetExternalDataPtr(ActorHandle);
	if (IsValid(Actor) == false)
	{
		UE_LOG(LogTemp, Warning, TEXT("FSTT_AddStateTag::ExitState : Not ActorHandle"));
		return;
	}
	UAbilitySystemComponent* ASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(Actor);
	if (IsValid(ASC) == false)
	{
		UE_LOG(LogTemp, Warning, TEXT("FSTT_AddStateTag::ExitState : Not ASC"));
		return;
	}

	FInstanceDataType& InstanceData = Context.GetInstanceData(*this);
	if (InstanceData.ActiveEffectHandle.IsValid() == false)
	{
		UE_LOG(LogTemp, Warning, TEXT("FSTT_AddStateTag::ExitState : Not ActiveEffectHandle"));
		return;
	}

	ASC->RemoveActiveGameplayEffect(InstanceData.ActiveEffectHandle);
	InstanceData.ActiveEffectHandle = FActiveGameplayEffectHandle();
	InstanceData.ActiveEffectHandle.Invalidate();
}


