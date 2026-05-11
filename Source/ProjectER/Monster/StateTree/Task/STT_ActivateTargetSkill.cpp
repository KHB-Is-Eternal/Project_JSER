#include "Monster/StateTree/Task/STT_ActivateTargetSkill.h"
#include "StateTreeLinker.h"
#include "StateTreeExecutionContext.h"

#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "Monster/BaseMonster.h"
#include "SkillSystem/GameAbility/SkillBase.h"

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

	ABaseMonster* Monster = Cast<ABaseMonster>(Actor);
	if (IsValid(Monster) == false)
	{
		UE_LOG(LogTemp, Warning, TEXT("FStateTreeExecutionContext::EnterState : Not Monster"));
		return EStateTreeRunStatus::Failed;
	}

	// [Fast Track] 데이터를 포함한 이벤트로 즉시 시전 시도
	FGameplayEventData Payload;
	Payload.Instigator = Actor;
	Payload.Target = Monster->GetTargetPlayer();
	Payload.TargetData = UAbilitySystemBlueprintLibrary::AbilityTargetDataFromActor(Monster->GetTargetPlayer());
	Payload.EventTag = InstanceData.AbilityTag;

	// [Fast Track] 데이터를 직접 지목하여 즉시 시전 시도
	if (USkillBase::ActivateSkillByTag(ASC, InstanceData.AbilityTag, Payload) == false)
	{
		UE_LOG(LogTemp, Warning, TEXT("FSTT_ActivateTargetSkill::EnterState : ActivateSkillByTag Fail (Tag: %s)"), *InstanceData.AbilityTag.ToString());
		return EStateTreeRunStatus::Failed;
	}

	return EStateTreeRunStatus::Running;
}

void FSTT_ActivateTargetSkill::ExitState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const
{
}
