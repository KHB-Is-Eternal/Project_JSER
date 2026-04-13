#include "Monster/StateTree/Consideration/STCons_SkillUtility.h"
#include "StateTreeLinker.h"
#include "StateTreeExecutionContext.h"

#include "Monster/BaseMonster.h"

FSTCons_SkillUtility::FSTCons_SkillUtility()
{
}

bool FSTCons_SkillUtility::Link(FStateTreeLinker& Linker)
{
	Linker.LinkExternalData(ActorHandle);
	return true;
}

const UStruct* FSTCons_SkillUtility::GetInstanceDataType() const
{
	return FInstanceDataType::StaticStruct();
}

float FSTCons_SkillUtility::GetScore(FStateTreeExecutionContext& Context) const
{
	AActor* Actor = Context.GetExternalDataPtr(ActorHandle);
	if (IsValid(Actor) == false)
	{
		UE_LOG(LogTemp, Warning, TEXT("FSTCons_SkillUtility::GetScore : Not ActorHandle"));
		return -1.f;
	}
	ABaseMonster* Monster = Cast<ABaseMonster>(Actor);
	if (IsValid(Monster) == false)
	{
		UE_LOG(LogTemp, Warning, TEXT("FSTCons_SkillUtility::GetScore : Not Monster"));
		return -1.f;
	}

	return 0.5f;
}
