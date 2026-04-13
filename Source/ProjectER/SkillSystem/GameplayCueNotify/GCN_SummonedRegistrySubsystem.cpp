#include "SkillSystem/GameplayCueNotify/GCN_SummonedRegistrySubsystem.h"

void UGCN_SummonedRegistrySubsystem::RegisterVfxActor(AActor* Instigator, float ActivationTime, AActor* VfxActor)
{
	if (!VfxActor)
	{
		return;
	}

	FGCN_SummonedKey RegistryKey(Instigator, ActivationTime);
	VfxRegistry.Add(RegistryKey, VfxActor);
}

AActor* UGCN_SummonedRegistrySubsystem::GetAndUnregisterVfxActor(AActor* Instigator, float ActivationTime)
{
	FGCN_SummonedKey RegistryKey(Instigator, ActivationTime);
	
	if (TWeakObjectPtr<AActor>* FoundActorPtr = VfxRegistry.Find(RegistryKey))
	{
		AActor* VfxActor = FoundActorPtr->Get();
		VfxRegistry.Remove(RegistryKey);
		return VfxActor;
	}

	return nullptr;
}
