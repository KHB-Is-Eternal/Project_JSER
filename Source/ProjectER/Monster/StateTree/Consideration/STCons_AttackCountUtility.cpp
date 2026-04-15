#include "Monster/StateTree/Consideration/STCons_AttackCountUtility.h"
#include "StateTreeLinker.h"
#include "StateTreeExecutionContext.h"
#include "AbilitySystemComponent.h"

#include "Monster/BaseMonster.h"

FSTCons_AttackCountUtility::FSTCons_AttackCountUtility()
{
}

bool FSTCons_AttackCountUtility::Link(FStateTreeLinker& Linker)
{
	Linker.LinkExternalData(ActorHandle);
	return true;
}

const UStruct* FSTCons_AttackCountUtility::GetInstanceDataType() const
{
	return FInstanceDataType::StaticStruct();
}

float FSTCons_AttackCountUtility::GetScore(FStateTreeExecutionContext& Context) const
{
	AActor* Actor = Context.GetExternalDataPtr(ActorHandle);
	if (IsValid(Actor) == false)
	{
		UE_LOG(LogTemp, Warning, TEXT("FSTCons_AttackCountUtility::GetScore : Not ActorHandle"));
		return -1.f;
	}
	ABaseMonster* Monster = Cast<ABaseMonster>(Actor);
	if (IsValid(Monster) == false)
	{
		UE_LOG(LogTemp, Warning, TEXT("FSTCons_AttackCountUtility::GetScore : Not Monster"));
		return -1.f;
	}
	UAbilitySystemComponent* ASC = Monster->GetAbilitySystemComponent();
	if (IsValid(ASC) == false)
	{
		UE_LOG(LogTemp, Warning, TEXT("FSTCons_AttackCountUtility::GetScore : Not ASC"));
		return -1.f;
	}

	FInstanceDataType& InstanceData = Context.GetInstanceData(*this);
	if (InstanceData.AttackCountThreshold <= Monster->GetAttackCount())
	{
		if (ASC->HasMatchingGameplayTag(FGameplayTag::RequestGameplayTag("Cooldown.Skill.R")))
		{
			return 0.f;
		}

		return 1.f;
	}


	return 0.f;
}
