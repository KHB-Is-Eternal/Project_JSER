#include "Monster/StateTree/Condition/STC_IsCombat.h"
#include "StateTreeLinker.h"
#include "StateTreeExecutionContext.h"
#include "Monster/BaseMonster.h"

FSTC_IsCombat::FSTC_IsCombat()
{
}

bool FSTC_IsCombat::Link(FStateTreeLinker& Linker)
{
	Linker.LinkExternalData(ActorHandle);
	return true;
}

const UStruct* FSTC_IsCombat::GetInstanceDataType() const
{
	return FInstanceDataType::StaticStruct();
}

bool FSTC_IsCombat::TestCondition(FStateTreeExecutionContext& Context) const
{
	AActor* Actor = Context.GetExternalDataPtr(ActorHandle);
	if (IsValid(Actor) == false)
	{
		UE_LOG(LogTemp, Warning, TEXT("FSTC_IsCombat::TestCondition : Not ActorHandle"));
		return false;
	}
	ABaseMonster* Monster = Cast<ABaseMonster>(Actor);
	if (IsValid(Monster) == false)
	{
		UE_LOG(LogTemp, Warning, TEXT("FSTC_IsCombat::TestCondition : Not Monster"));
		return false;
	}

	FInstanceDataType& InstanceData = Context.GetInstanceData(*this);
	bool IsCombat = Monster->GetbIsCombat();
	return IsCombat != InstanceData.Invert;
}
