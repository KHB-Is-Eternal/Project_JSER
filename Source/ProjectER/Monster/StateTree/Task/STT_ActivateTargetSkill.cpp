#include "Monster/StateTree/Task/STT_ActivateTargetSkill.h"
#include "StateTreeLinker.h"
#include "StateTreeExecutionContext.h"

#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "Monster/BaseMonster.h"

FSTT_ActivateTargetSkill::FSTT_ActivateTargetSkill()
{
	bShouldCallTick = false;
}

bool FSTT_ActivateTargetSkill::Link(FStateTreeLinker& Linker)
{
	Linker.LinkExternalData(ActorHandle);
	return true;
}

const UStruct* FSTT_ActivateTargetSkill::GetInstanceDataType() const
{
	return FInstanceDataType::StaticStruct();
}

EStateTreeRunStatus FSTT_ActivateTargetSkill::EnterState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const
{
	AActor* Actor = Context.GetExternalDataPtr(ActorHandle);
	if (IsValid(Actor) == false)
	{
		UE_LOG(LogTemp, Warning, TEXT("FStateTreeExecutionContext::EnterState : Not ActorHandle"));
		return EStateTreeRunStatus::Failed;
	}
	UAbilitySystemComponent* ASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(Actor);
	if (IsValid(ASC) == false)
	{
		UE_LOG(LogTemp, Warning, TEXT("FStateTreeExecutionContext::EnterState : Not ASC"));
		return EStateTreeRunStatus::Failed;
	}

	FInstanceDataType& InstanceData = Context.GetInstanceData(*this);
	if (InstanceData.AbilityTag.IsValid() == false)
	{
		UE_LOG(LogTemp, Warning, TEXT("FStateTreeExecutionContext::EnterState : Not AbilityTag"));
		return EStateTreeRunStatus::Failed;
	}

	for (FGameplayAbilitySpec& Spec : ASC->GetActivatableAbilities())
	{
		if (Spec.DynamicAbilityTags.HasTagExact(InstanceData.AbilityTag))
		{
			// Ability 실행 시도
			if (ASC->TryActivateAbility(Spec.Handle) == false)
			{
				UE_LOG(LogTemp, Warning, TEXT("FStateTreeExecutionContext::EnterState : Try Activate Skill Fail"));
				return EStateTreeRunStatus::Failed;
			}

			// Ability 실행 성공했으면
			ABaseMonster* Monster = Cast<ABaseMonster>(Actor);
			if (IsValid(Monster) == false)
			{
				UE_LOG(LogTemp, Warning, TEXT("FStateTreeExecutionContext::EnterState : Not Monster"));
				return EStateTreeRunStatus::Failed;
			}

			FGameplayEventData Payload;
			Payload.Instigator = Actor;
			Payload.Target = Monster->GetTargetPlayer();
			Payload.TargetData = UAbilitySystemBlueprintLibrary::AbilityTargetDataFromActor(Monster->GetTargetPlayer());
			UAbilitySystemBlueprintLibrary::SendGameplayEventToActor(Actor, InstanceData.EventTag, Payload);

			break;
		}
	}

	return EStateTreeRunStatus::Running;
}

void FSTT_ActivateTargetSkill::ExitState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const
{
}
