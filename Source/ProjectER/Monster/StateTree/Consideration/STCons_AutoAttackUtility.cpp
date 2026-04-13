#include "Monster/StateTree/Consideration/STCons_AutoAttackUtility.h"
#include "StateTreeLinker.h"
#include "StateTreeExecutionContext.h"

#include "Monster/BaseMonster.h"

FSTCons_AutoAttackUtility::FSTCons_AutoAttackUtility()
{

}

bool FSTCons_AutoAttackUtility::Link(FStateTreeLinker& Linker)
{
	Linker.LinkExternalData(ActorHandle);
	return true;
}

const UStruct* FSTCons_AutoAttackUtility::GetInstanceDataType() const
{
	return FInstanceDataType::StaticStruct();
}

float FSTCons_AutoAttackUtility::GetScore(FStateTreeExecutionContext& Context) const
{
	AActor* Actor = Context.GetExternalDataPtr(ActorHandle);
	if (IsValid(Actor) == false)
	{
		UE_LOG(LogTemp, Warning, TEXT("FSTCons_AutoAttackUtility::GetScore : Not ActorHandle"));
		return -1.f;
	}
	ABaseMonster* Monster = Cast<ABaseMonster>(Actor);
	if (IsValid(Monster) == false)
	{
		UE_LOG(LogTemp, Warning, TEXT("FSTCons_AutoAttackUtility::GetScore : Not Monster"));
		return -1.f;
	}

	if (Monster->GetIsFirstAttack() == false)
	{
		return 1.f;
	}

	return 0.f;
}
