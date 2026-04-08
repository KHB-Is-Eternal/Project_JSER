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
	AActor& Actor = Context.GetExternalData(ActorHandle);
	UAbilitySystemComponent* ASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(&Actor);

	FInstanceDataType& InstanceData = Context.GetInstanceData(*this);
	
	if (ASC)
	{
		TSubclassOf<UGE_AddTag> Effect = UGE_AddTag::StaticClass();
		FGameplayEffectContextHandle EffectContextHandle = ASC->MakeEffectContext();
		FGameplayEffectSpecHandle EffectSpecHandle = ASC->MakeOutgoingSpec(Effect, 1.0f, EffectContextHandle);
		if (EffectSpecHandle.IsValid())
		{
			EffectSpecHandle.Data.Get()->DynamicGrantedTags.AddTag(InstanceData.StateTag);

			InstanceData.ActiveEffectHandle = ASC->ApplyGameplayEffectSpecToSelf(*EffectSpecHandle.Data.Get());
			return EStateTreeRunStatus::Running;
		}
	}

	return EStateTreeRunStatus::Failed;
}

void FSTT_AddStateTag::ExitState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const
{
	AActor& Actor = Context.GetExternalData(ActorHandle);
	UAbilitySystemComponent* ASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(&Actor);

	FInstanceDataType& InstanceData = Context.GetInstanceData(*this);

	if (ASC && InstanceData.ActiveEffectHandle.IsValid())
	{
		ASC->RemoveActiveGameplayEffect(InstanceData.ActiveEffectHandle);
		InstanceData.ActiveEffectHandle.Invalidate();
	}
}


