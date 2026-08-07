#include "SkillSystem/GameplayCueNotify/GCN_SummonedRegistrySubsystem.h"
#include "SkillSystem/Interfaces/SkillSummonedActor.h"

void UGCN_SummonedRegistrySubsystem::RegisterVfxActor(AActor* Instigator, float ActivationTime, AActor* VfxActor)
{
	if (!VfxActor) return;

	// 1. 이미 기다리고 있는 판정 액터가 있는지 확인 (Late Binding)
	float Tolerance = DefaultHandshakeTolerance;
	float BestDelta = Tolerance;
	FGCN_SummonedKey BestKey;
	AActor* BestPendingActor = nullptr;

	for (auto It = PendingActors.CreateIterator(); It; ++It)
	{
		if (It.Key().Instigator == Instigator && It.Value().IsValid())
		{
			if (It.Key().ActivationTime == 0.0f || ActivationTime == 0.0f)
			{
				BestKey = It.Key();
				BestPendingActor = It.Value().Get();
				break;
			}

			float Delta = FMath::Abs(It.Key().ActivationTime - ActivationTime);
			if (Delta < BestDelta)
			{
				BestDelta = Delta;
				BestKey = It.Key();
				BestPendingActor = It.Value().Get();
			}
		}
		else if (!It.Value().IsValid())
		{
			It.RemoveCurrent(); // 가비지 컬렉션 체크
		}
	}

	if (BestPendingActor)
	{
		PendingActors.Remove(BestKey);
		
		// 2. 즉시 핸드셰이크 수행 (VFX를 액터에 부착)
		FAttachmentTransformRules AttachRules(EAttachmentRule::SnapToTarget, true);
		VfxActor->AttachToActor(BestPendingActor, AttachRules);

		// 판정 액터의 수명을 VFX도 따라가도록 설정
		if (BestPendingActor->GetLifeSpan() > 0.0f)
		{
			VfxActor->SetLifeSpan(BestPendingActor->GetLifeSpan());
		}

		if (BestPendingActor->GetClass()->ImplementsInterface(USkillSummonedActor::StaticClass()) || 
			BestPendingActor->Implements<USkillSummonedActor>())
		{
			ISkillSummonedActor::Execute_OnVfxHandshakeCompleted(BestPendingActor, VfxActor);
		}
		return;
	}

	// 3. 기다리는 액터가 없다면 VFX 레지스트리에 등록
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

		if (VfxActor)
		{
	
				
			return VfxActor;
		}
	}



	return nullptr;
}

bool UGCN_SummonedRegistrySubsystem::IsVfxActorRegistered(AActor* Instigator, float ActivationTime) const
{
	FGCN_SummonedKey RegistryKey(Instigator, ActivationTime);
	return VfxRegistry.Contains(RegistryKey);
}

AActor* UGCN_SummonedRegistrySubsystem::FindAndUnregisterVfxActorFuzzy(AActor* Instigator, float TargetTime, float Tolerance)
{
	if (Tolerance < 0.0f)
	{
		Tolerance = DefaultHandshakeTolerance;
	}

	// 1. 정확한 매칭 우선 시도 (TargetTime이 0.0f가 아닐 때만)
	if (TargetTime > 0.0f)
	{
		if (AActor* ExactMatch = GetAndUnregisterVfxActor(Instigator, TargetTime))
		{
			return ExactMatch;
		}
	}

	// 2. Instigator 기준 최근접 시간 퍼지 매칭 (클라이언트-서버 시간 차이 보상) + 와일드카드 매칭
	float BestDelta = Tolerance;
	FGCN_SummonedKey BestKey;
	AActor* BestActor = nullptr;

	for (auto& Pair : VfxRegistry)
	{
		if (Pair.Key.Instigator == Instigator && Pair.Value.IsValid())
		{
			// Simulated Proxy 등에서 시전 시간을 모른 채(0.0f) 등록된 경우 와일드카드로 즉시 매칭
			if (Pair.Key.ActivationTime == 0.0f || TargetTime == 0.0f)
			{
				BestKey = Pair.Key;
				BestActor = Pair.Value.Get();
				break;
			}

			float Delta = FMath::Abs(Pair.Key.ActivationTime - TargetTime);
			if (Delta < BestDelta)
			{
				BestDelta = Delta;
				BestKey = Pair.Key;
				BestActor = Pair.Value.Get();
			}
		}
	}

	if (BestActor)
	{
		VfxRegistry.Remove(BestKey);

		return BestActor;
	}

	return nullptr;
}

void UGCN_SummonedRegistrySubsystem::RegisterPendingActorFuzzy(AActor* Instigator, float ActivationTime, AActor* PendingActor)
{
	if (!PendingActor) return;

	FGCN_SummonedKey RegistryKey(Instigator, ActivationTime);
	PendingActors.Add(RegistryKey, PendingActor);


}
