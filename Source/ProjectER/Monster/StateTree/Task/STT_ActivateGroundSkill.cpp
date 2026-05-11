#include "Monster/StateTree/Task/STT_ActivateGroundSkill.h"
#include "StateTreeLinker.h"
#include "StateTreeExecutionContext.h"

#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "Monster/BaseMonster.h"
#include "SkillSystem/GameAbility/SkillBase.h"

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

	ABaseMonster* Monster = Cast<ABaseMonster>(Actor);
	if (!Monster || !Monster->GetTargetPlayer())
	{
		UE_LOG(LogTemp, Warning, TEXT("FSTT_ActivateGroundSkill::EnterState : Invalid Monster or Target"));
		return EStateTreeRunStatus::Failed;
	}

	AActor* TargetActor = Monster->GetTargetPlayer();
	FVector TargetLocation = TargetActor->GetActorLocation();

	// [Fast Track] 데이터 주입 시전
	FGameplayEventData Payload;
	Payload.Instigator = Actor;
	Payload.Target = TargetActor;
	FHitResult Hit; Hit.Location = TargetLocation;
	Payload.TargetData = UAbilitySystemBlueprintLibrary::AbilityTargetDataFromHitResult(Hit);
	Payload.EventTag = InstanceData.AbilityTag;

	if (USkillBase::ActivateSkillByTag(ASC, InstanceData.AbilityTag, Payload) == false)
	{
		UE_LOG(LogTemp, Warning, TEXT("FSTT_ActivateGroundSkill::EnterState : ActivateSkillByTag Fail (Tag: %s)"), *InstanceData.AbilityTag.ToString());
		return EStateTreeRunStatus::Failed;
	}

	if (InstanceData.GameplayCueTag.IsValid())
	{
		FGameplayCueParameters Params; Params.Instigator = Actor; Params.Location = TargetLocation;
		ASC->AddGameplayCue(InstanceData.GameplayCueTag, Params);
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
