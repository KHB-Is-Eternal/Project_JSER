#include "Monster/StateTree/Task/STT_ActivateDirectionSkill.h"
#include "StateTreeLinker.h"
#include "StateTreeExecutionContext.h"

#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "Monster/BaseMonster.h"
#include "SkillSystem/GameAbility/SkillBase.h"

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

	ABaseMonster* Monster = Cast<ABaseMonster>(Actor);
	if (!Monster || !Monster->GetTargetPlayer())
	{
		UE_LOG(LogTemp, Warning, TEXT("FSTT_ActivateDirectionSkill::EnterState : Invalid Monster or Target"));
		return EStateTreeRunStatus::Failed;
	}

	AActor* TargetActor = Monster->GetTargetPlayer();
	FVector StartLocation = Actor->GetActorLocation();
	FVector TargetLocation = TargetActor->GetActorLocation();
	FVector Direction = (TargetLocation - StartLocation).GetSafeNormal();
	FVector AimPoint = StartLocation + Direction;

	// [Fast Track] 데이터 주입 시전
	FGameplayEventData Payload;
	Payload.Instigator = Actor;
	Payload.Target = TargetActor;
	FHitResult Hit; Hit.Location = AimPoint;
	Payload.TargetData = UAbilitySystemBlueprintLibrary::AbilityTargetDataFromHitResult(Hit);
	Payload.EventTag = InstanceData.AbilityTag;

	// [Fast Track] 엔진 함수인 TriggerAbilityFromGameplayEvent를 직접 사용하여 확정적 시전 시도
	FGameplayAbilitySpecHandle SpecHandle;
	for (const FGameplayAbilitySpec& Spec : ASC->GetActivatableAbilities())
	{
		if (Spec.DynamicAbilityTags.HasTagExact(InstanceData.AbilityTag) || Spec.GetDynamicSpecSourceTags().HasTagExact(InstanceData.AbilityTag))
		{
			SpecHandle = Spec.Handle;
			break;
		}
	}

	if (ASC->TriggerAbilityFromGameplayEvent(SpecHandle, ASC->AbilityActorInfo.Get(), InstanceData.AbilityTag, &Payload, *ASC) == false)
	{
		UE_LOG(LogTemp, Warning, TEXT("FSTT_ActivateDirectionSkill::EnterState : TriggerAbilityFromGameplayEvent Fail (Tag: %s)"), *InstanceData.AbilityTag.ToString());
		return EStateTreeRunStatus::Failed;
	}

	if (InstanceData.GameplayCueTag.IsValid())
	{
		FGameplayCueParameters Params; Params.Instigator = Actor; Params.Location = AimPoint;
		ASC->AddGameplayCue(InstanceData.GameplayCueTag, Params);
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
