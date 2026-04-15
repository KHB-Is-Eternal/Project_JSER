#include "Monster/StateTree/Task/STT_ActivateGroundSkill.h"
#include "StateTreeLinker.h"
#include "StateTreeExecutionContext.h"

#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "Monster/BaseMonster.h"

FSTT_ActivateGroundSkill::FSTT_ActivateGroundSkill()
{
	bShouldCallTick = false;
}

bool FSTT_ActivateGroundSkill::Link(FStateTreeLinker& Linker)
{
	Linker.LinkExternalData(ActorHandle);
	return true;
}

const UStruct* FSTT_ActivateGroundSkill::GetInstanceDataType() const
{
	return FInstanceDataType::StaticStruct();
}

EStateTreeRunStatus FSTT_ActivateGroundSkill::EnterState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const
{
	AActor* Actor = Context.GetExternalDataPtr(ActorHandle);
	if (IsValid(Actor) == false)
	{
		UE_LOG(LogTemp, Warning, TEXT("FSTT_ActivateGroundSkill::EnterState : Not ActorHandle"));
		return EStateTreeRunStatus::Failed;
	}
	UAbilitySystemComponent* ASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(Actor);
	if (IsValid(ASC) == false)
	{
		UE_LOG(LogTemp, Warning, TEXT("FSTT_ActivateGroundSkill::EnterState : Not ASC"));
		return EStateTreeRunStatus::Failed;
	}

	FInstanceDataType& InstanceData = Context.GetInstanceData(*this);
	if (InstanceData.AbilityTag.IsValid() == false)
	{
		UE_LOG(LogTemp, Warning, TEXT("FSTT_ActivateGroundSkill::EnterState : Not AbilityTag"));
		return EStateTreeRunStatus::Failed;
	}

	for (FGameplayAbilitySpec& Spec : ASC->GetActivatableAbilities())
	{
		if (Spec.DynamicAbilityTags.HasTagExact(InstanceData.AbilityTag))
		{
			// Ability 실행 시도
			if (ASC->TryActivateAbility(Spec.Handle) == false)
			{
				UE_LOG(LogTemp, Warning, TEXT("FSTT_ActivateGroundSkill::EnterState : Try Activate Skill Fail"));
				return EStateTreeRunStatus::Failed;
			}

			// Ability 실행 성공했으면
			ABaseMonster* Monster = Cast<ABaseMonster>(Actor);
			if (IsValid(Monster) == false)
			{
				UE_LOG(LogTemp, Warning, TEXT("FSTT_ActivateGroundSkill::EnterState : Not Monster"));
				return EStateTreeRunStatus::Failed;
			}
			AActor* TargetActor = Monster->GetTargetPlayer();
			if (IsValid(TargetActor) == false)
			{
				UE_LOG(LogTemp, Warning, TEXT("FSTT_ActivateGroundSkill::EnterState : Not TargetActor"));
				return EStateTreeRunStatus::Failed;
			}

			FVector TargetLocation = TargetActor->GetActorLocation();

			FGameplayEventData Payload;
			Payload.Instigator = Actor;
			Payload.Target = TargetActor;
			FHitResult HitResult;
			HitResult.Location = TargetLocation;
			Payload.TargetData = UAbilitySystemBlueprintLibrary::AbilityTargetDataFromHitResult(HitResult);
			UAbilitySystemBlueprintLibrary::SendGameplayEventToActor(Actor, InstanceData.EventTag, Payload);
			
			// Decal 생성
			if (InstanceData.GameplayCueTag.IsValid() == false)
			{
				UE_LOG(LogTemp, Warning, TEXT("FSTT_ActivateGroundSkill::EnterState : Not GameplayCueTag"));
				return EStateTreeRunStatus::Failed;
			}
			FGameplayCueParameters Params;
			Params.Instigator = Actor;
			Params.Location = TargetLocation;
			ASC->AddGameplayCue(InstanceData.GameplayCueTag, Params);
			
			break;
		}
	}

	return EStateTreeRunStatus::Running;
}

void FSTT_ActivateGroundSkill::ExitState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const
{
	AActor* Actor = Context.GetExternalDataPtr(ActorHandle);
	if (IsValid(Actor) == false)
	{
		UE_LOG(LogTemp, Warning, TEXT("FSTT_ActivateGroundSkill::ExitState : Not ActorHandle"));
		return;
	}
	UAbilitySystemComponent* ASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(Actor);
	if (IsValid(ASC) == false)
	{
		UE_LOG(LogTemp, Warning, TEXT("FSTT_ActivateGroundSkill::ExitState : Not ASC"));
		return;
	}

	FInstanceDataType& InstanceData = Context.GetInstanceData(*this);
	if (InstanceData.GameplayCueTag.IsValid() == false)
	{
		UE_LOG(LogTemp, Warning, TEXT("FSTT_ActivateGroundSkill::ExitState : Not GameplayCueTag"));
		return;
	}
	ASC->RemoveGameplayCue(InstanceData.GameplayCueTag);
}
