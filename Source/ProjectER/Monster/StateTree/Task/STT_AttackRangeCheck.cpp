#include "Monster/StateTree/Task/STT_AttackRangeCheck.h"
#include "StateTreeLinker.h"
#include "StateTreeExecutionContext.h"

#include "Monster/BaseMonster.h"
#include "AbilitySystemComponent.h"
#include "Monster/GAS/AttributeSet/BaseMonsterAttributeSet.h"


FSTT_AttackRangeCheck::FSTT_AttackRangeCheck()
{
	bShouldCallTick = false;
}

bool FSTT_AttackRangeCheck::Link(FStateTreeLinker& Linker)
{
	Linker.LinkExternalData(ActorHandle);
	return true;
}

const UStruct* FSTT_AttackRangeCheck::GetInstanceDataType() const
{
	return FInstanceDataType::StaticStruct();
}

EStateTreeRunStatus FSTT_AttackRangeCheck::EnterState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const
{
	AActor* Actor = Context.GetExternalDataPtr(ActorHandle);
	if (IsValid(Actor) == false)
	{
		UE_LOG(LogTemp, Warning, TEXT("FSTT_AttackRangeCheck::EnterState : Not ActorHandle"));
		return EStateTreeRunStatus::Failed;
	}
	ABaseMonster* Monster = Cast<ABaseMonster>(Actor);
	if (IsValid(Monster) == false)
	{ 
		UE_LOG(LogTemp, Warning, TEXT("FSTT_AttackRangeCheck::EnterState : Not Monster"));
		return EStateTreeRunStatus::Failed;
	}
	AActor* Target = Monster->GetTargetPlayer();
	if (IsValid(Target) == false)
	{
		UE_LOG(LogTemp, Warning, TEXT("FSTT_AttackRangeCheck::EnterState : Not Target"));
		return EStateTreeRunStatus::Failed;
	}

	FAttackRangeCheckData& InstanceData = Context.GetInstanceData(*this);

	float AttackRange = Monster->GetAttributeSet()->GetAttackRange();
	float DistanceSq = FVector::DistSquared(Target->GetActorLocation(), Actor->GetActorLocation());
	float RangeSq = AttackRange * AttackRange;

	if (DistanceSq <= RangeSq)
	{
		if (InstanceData.AttackEventTag.IsValid() == false)
		{
			UE_LOG(LogTemp, Warning, TEXT("FSTT_AttackRangeCheck::EnterState : Not AttackEventTag"));
			return EStateTreeRunStatus::Failed;
		}
		Monster->SendStateTreeEvent(InstanceData.AttackEventTag);
		return EStateTreeRunStatus::Running;
	}
	else
	{
		if (InstanceData.TargetOnEventTag.IsValid() == false)
		{
			UE_LOG(LogTemp, Warning, TEXT("FSTT_AttackRangeCheck::EnterState : Not TargetOnEventTag"));
			return EStateTreeRunStatus::Failed;
		}
		Monster->SendStateTreeEvent(InstanceData.TargetOnEventTag);
		return EStateTreeRunStatus::Running;
	}
	
	return EStateTreeRunStatus::Failed;
}

void FSTT_AttackRangeCheck::ExitState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const
{
}
