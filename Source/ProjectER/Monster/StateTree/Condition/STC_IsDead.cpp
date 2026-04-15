#include "Monster/StateTree/Condition/STC_IsDead.h"
#include "StateTreeLinker.h"
#include "StateTreeExecutionContext.h"
#include "Monster/BaseMonster.h"

FSTC_IsDead::FSTC_IsDead()
{
}

bool FSTC_IsDead::Link(FStateTreeLinker& Linker)
{
	Linker.LinkExternalData(ActorHandle);
	return true;
}

const UStruct* FSTC_IsDead::GetInstanceDataType() const
{
	return FInstanceDataType::StaticStruct();
}

bool FSTC_IsDead::TestCondition(FStateTreeExecutionContext& Context) const
{
	AActor* Actor = Context.GetExternalDataPtr(ActorHandle);
	if (IsValid(Actor) == false)
	{
		UE_LOG(LogTemp, Warning, TEXT("FSTC_IsDead::TestCondition : Not ActorHandle"));
		return false;
	}
	ABaseMonster* Monster = Cast<ABaseMonster>(Actor);
	if (IsValid(Monster) == false)
	{
		UE_LOG(LogTemp, Warning, TEXT("FSTC_IsDead::TestCondition : Not Monster"));
		return false;
	}

	FInstanceDataType& InstanceData = Context.GetInstanceData(*this);
	bool IsCombat = Monster->GetbIsDead();
	return IsCombat != InstanceData.Invert;
}
