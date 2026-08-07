#include "Monster/StateTree/Task/STT_SetCollisionProfile.h"
#include "StateTreeLinker.h"
#include "StateTreeExecutionContext.h"
#include "Monster/BaseMonster.h"
#include "Components/CapsuleComponent.h"

FSTT_SetCollisionProfile::FSTT_SetCollisionProfile()
{
	bShouldCallTick = false;
}

bool FSTT_SetCollisionProfile::Link(FStateTreeLinker& Linker)
{
	Linker.LinkExternalData(ActorHandle);
	return true;
}

const UStruct* FSTT_SetCollisionProfile::GetInstanceDataType() const
{
	return FInstanceDataType::StaticStruct();
}

EStateTreeRunStatus FSTT_SetCollisionProfile::EnterState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const
{
	AActor* Actor = Context.GetExternalDataPtr(ActorHandle);
	if (IsValid(Actor) == false)
	{
		UE_LOG(LogTemp, Warning, TEXT("FSTT_SetCollisionProfile::EnterState : Not ActorHandle"));
		return EStateTreeRunStatus::Failed;
	}
	ABaseMonster* Monster = Cast<ABaseMonster>(Actor);
	if (IsValid(Monster) == false)
	{
		UE_LOG(LogTemp, Warning, TEXT("FSTT_SetCollisionProfile::EnterState : Not Monster"));
		return EStateTreeRunStatus::Failed;
	}

	FInstanceDataType& InstanceData = Context.GetInstanceData(*this);
	FCollisionResponseTemplate Template;
	if (UCollisionProfile::Get()->GetProfileTemplate(InstanceData.StartProfileName.Name, Template) == false)
	{
		UE_LOG(LogTemp, Warning, TEXT("FSTT_SetCollisionProfile::EnterState : Not CollisionStartProfile"));
		return EStateTreeRunStatus::Failed;
	}
	Monster->Multicast_SetCollisionProfileName(InstanceData.StartProfileName.Name);

	return EStateTreeRunStatus::Running;
}

void FSTT_SetCollisionProfile::ExitState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const
{
	AActor* Actor = Context.GetExternalDataPtr(ActorHandle);
	if (IsValid(Actor) == false)
	{
		UE_LOG(LogTemp, Warning, TEXT("FSTT_SetCollisionProfile::ExitState : Not ActorHandle"));
		return;
	}
	ABaseMonster* Monster = Cast<ABaseMonster>(Actor);
	if (IsValid(Monster) == false)
	{
		UE_LOG(LogTemp, Warning, TEXT("FSTT_SetCollisionProfile::ExitState : Not Monster"));
		return;
	}

	FInstanceDataType& InstanceData = Context.GetInstanceData(*this);
	FCollisionResponseTemplate Template;
	if (UCollisionProfile::Get()->GetProfileTemplate(InstanceData.EndProfileName.Name, Template) == false)
	{
		UE_LOG(LogTemp, Warning, TEXT("FSTT_SetCollisionProfile::EnterState : Not CollisionEndProfile"));
		return;
	}
	Monster->Multicast_SetCollisionProfileName(InstanceData.EndProfileName.Name);
}
