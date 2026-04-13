#include "Monster/StateTree/Task/STT_ActivateDirectionSkill.h"
#include "StateTreeLinker.h"
#include "StateTreeExecutionContext.h"

#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "Monster/BaseMonster.h"

FSTT_ActivateDirectionSkill::FSTT_ActivateDirectionSkill()
{
	bShouldCallTick = false;
}

bool FSTT_ActivateDirectionSkill::Link(FStateTreeLinker& Linker)
{
	Linker.LinkExternalData(ActorHandle);
	return true;
}

const UStruct* FSTT_ActivateDirectionSkill::GetInstanceDataType() const
{
	return FInstanceDataType::StaticStruct();
}

EStateTreeRunStatus FSTT_ActivateDirectionSkill::EnterState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const
{
	AActor* Actor = Context.GetExternalDataPtr(ActorHandle);
	if (IsValid(Actor) == false)
	{
		UE_LOG(LogTemp, Warning, TEXT("FSTT_ActivateDirectionSkill::EnterState : Not ActorHandle"));
		return EStateTreeRunStatus::Failed;
	}
	UAbilitySystemComponent* ASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(Actor);
	if (IsValid(ASC) == false)
	{
		UE_LOG(LogTemp, Warning, TEXT("FSTT_ActivateDirectionSkill::EnterState : Not ASC"));
		return EStateTreeRunStatus::Failed;
	}

	FInstanceDataType& InstanceData = Context.GetInstanceData(*this);
	if (InstanceData.AbilityTag.IsValid() == false)
	{
		UE_LOG(LogTemp, Warning, TEXT("FSTT_ActivateDirectionSkill::EnterState : Not AbilityTag"));
		return EStateTreeRunStatus::Failed;
	}

	for (FGameplayAbilitySpec& Spec : ASC->GetActivatableAbilities())
	{
		if (Spec.DynamicAbilityTags.HasTagExact(InstanceData.AbilityTag))
		{
			// Ability 실행 시도
			if (ASC->TryActivateAbility(Spec.Handle) == false)
			{
				UE_LOG(LogTemp, Warning, TEXT("FSTT_ActivateDirectionSkill::EnterState : Try Activate Skill Fail"));
				return EStateTreeRunStatus::Failed;
			}
			
			// Ability 실행 성공했으면
			ABaseMonster* Monster = Cast<ABaseMonster>(Actor);
			if (IsValid(Monster) == false)
				{
					UE_LOG(LogTemp, Warning, TEXT("FSTT_ActivateDirectionSkill::EnterState : Not Monster"));
					return EStateTreeRunStatus::Failed;
				}
			AActor* TargetActor = Monster->GetTargetPlayer();
			if (IsValid(TargetActor) == false)
				{
					UE_LOG(LogTemp, Warning, TEXT("FSTT_ActivateDirectionSkill::EnterState : Not TargetActor"));
					return EStateTreeRunStatus::Failed;
				}
			
			FVector StartLocation = Actor->GetActorLocation();
			FVector TargetLocation = TargetActor->GetActorLocation();
			FVector Direction = (TargetLocation - StartLocation).GetSafeNormal();
			FVector AimPoint = StartLocation + Direction;

			FGameplayEventData Payload;
			Payload.Instigator = Monster;
			Payload.Target = TargetActor;
			FHitResult HitResult;
			HitResult.Location = AimPoint;
			Payload.TargetData = UAbilitySystemBlueprintLibrary::AbilityTargetDataFromHitResult(HitResult);
			UAbilitySystemBlueprintLibrary::SendGameplayEventToActor(Actor, InstanceData.EventTag, Payload);
			
			// Decal 생성
			if (InstanceData.GameplayCueTag.IsValid() == false)
				{
					UE_LOG(LogTemp, Warning, TEXT("FSTT_ActivateDirectionSkill::EnterState : Not GameplayCueTag"));
					return EStateTreeRunStatus::Failed;
				}
			FGameplayCueParameters Params;
			Params.Instigator = Actor;
			Params.Location = AimPoint;
			ASC->AddGameplayCue(InstanceData.GameplayCueTag, Params);
			
			break;
		}
	}

	return EStateTreeRunStatus::Running;
}

void FSTT_ActivateDirectionSkill::ExitState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const
{
	AActor* Actor = Context.GetExternalDataPtr(ActorHandle);
	if (IsValid(Actor) == false)
	{
		UE_LOG(LogTemp, Warning, TEXT("FSTT_ActivateDirectionSkill::ExitState : Not ActorHandle"));
		return;
	}
	UAbilitySystemComponent* ASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(Actor);
	if (IsValid(ASC) == false)
	{
		UE_LOG(LogTemp, Warning, TEXT("FSTT_ActivateDirectionSkill::ExitState : Not ASC"));
		return;
	}

	FInstanceDataType& InstanceData = Context.GetInstanceData(*this);
	if (InstanceData.GameplayCueTag.IsValid() == false)
	{
		UE_LOG(LogTemp, Warning, TEXT("FSTT_ActivateDirectionSkill::ExitState : Not GameplayCueTag"));
		return;
	}
	ASC->RemoveGameplayCue(InstanceData.GameplayCueTag);
}
