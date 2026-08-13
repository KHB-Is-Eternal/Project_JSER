#include "SkillSystem/GameplayCueNotify/GCN_SummonedRegistrySubsystem.h"
#include "SkillSystem/Interfaces/SkillSummonedActor.h"

void UGCN_SummonedRegistrySubsystem::RegisterVfxActor(AActor* Instigator, float ActivationTime, AActor* VfxActor, const UObject* SourceObject)
{
	if (!VfxActor) return;

	FString InstigatorName = Instigator ? Instigator->GetName() : TEXT("None");
	FString VfxName = VfxActor->GetName();
	FString SourceName = SourceObject ? SourceObject->GetName() : TEXT("None");

	UE_LOG(LogTemp, Warning, TEXT("[HandshakeLog] RegisterVfxActor -> Instigator: %s | Time: %.4f | VfxActor: %s | SourceObject: %s"),
		*InstigatorName, ActivationTime, *VfxName, *SourceName);

	// 1. 이미 기다리고 있는 판정 액터가 있는지 확인 (Late Binding)
	float Tolerance = DefaultHandshakeTolerance;
	float BestDelta = Tolerance;
	FGCN_SummonedKey BestKey;
	AActor* BestPendingActor = nullptr;

	for (auto It = PendingActors.CreateIterator(); It; ++It)
	{
		if (It.Key().Instigator == Instigator && It.Value().IsValid())
		{
			if (SourceObject && It.Key().SourceObject.IsValid())
			{
				const UObject* KeySource = It.Key().SourceObject.Get();
				if (KeySource != SourceObject && KeySource->GetClass() != SourceObject->GetClass())
				{
					UE_LOG(LogTemp, Log, TEXT("[HandshakeLog] RegisterVfxActor -> Mismatch SourceObject: Pending(%s) vs Vfx(%s)"),
						*KeySource->GetName(), *SourceObject->GetName());
					continue;
				}
			}

			if (It.Key().ActivationTime == 0.0f && ActivationTime == 0.0f)
			{
				BestKey = It.Key();
				BestPendingActor = It.Value().Get();
				break;
			}

			if (It.Key().ActivationTime > 0.0f && ActivationTime > 0.0f)
			{
				float Delta = FMath::Abs(It.Key().ActivationTime - ActivationTime);
				if (Delta < BestDelta)
				{
					BestDelta = Delta;
					BestKey = It.Key();
					BestPendingActor = It.Value().Get();
				}
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
		
		UE_LOG(LogTemp, Warning, TEXT("[HandshakeLog] RegisterVfxActor -> MATCH FOUND! Attaching VfxActor(%s) to PendingActor(%s)"),
			*VfxName, *BestPendingActor->GetName());

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
	FGCN_SummonedKey RegistryKey(Instigator, ActivationTime, SourceObject);
	VfxRegistry.Add(RegistryKey, VfxActor);
	UE_LOG(LogTemp, Log, TEXT("[HandshakeLog] RegisterVfxActor -> Added to VfxRegistry (Pending Logic Actor)"));
}

AActor* UGCN_SummonedRegistrySubsystem::GetAndUnregisterVfxActor(AActor* Instigator, float ActivationTime, const UObject* SourceObject)
{
	FGCN_SummonedKey RegistryKey(Instigator, ActivationTime, SourceObject);
	
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

bool UGCN_SummonedRegistrySubsystem::IsVfxActorRegistered(AActor* Instigator, float ActivationTime, const UObject* SourceObject) const
{
	FGCN_SummonedKey RegistryKey(Instigator, ActivationTime, SourceObject);
	return VfxRegistry.Contains(RegistryKey);
}

AActor* UGCN_SummonedRegistrySubsystem::FindAndUnregisterVfxActorFuzzy(AActor* Instigator, float TargetTime, float Tolerance, const UObject* SourceObject)
{
	FString InstigatorName = Instigator ? Instigator->GetName() : TEXT("None");
	FString SourceName = SourceObject ? SourceObject->GetName() : TEXT("None");

	UE_LOG(LogTemp, Warning, TEXT("[HandshakeLog] FindAndUnregisterVfxActorFuzzy -> Instigator: %s | TargetTime: %.4f | SourceObject: %s"),
		*InstigatorName, TargetTime, *SourceName);

	if (Tolerance < 0.0f)
	{
		Tolerance = DefaultHandshakeTolerance;
	}

	// 1. 정확한 매칭 우선 시도 (TargetTime이 0.0f가 아닐 때만)
	if (TargetTime > 0.0f)
	{
		if (AActor* ExactMatch = GetAndUnregisterVfxActor(Instigator, TargetTime, SourceObject))
		{
			UE_LOG(LogTemp, Warning, TEXT("[HandshakeLog] FindAndUnregisterVfxActorFuzzy -> EXACT MATCH FOUND: %s"), *ExactMatch->GetName());
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
			if (SourceObject && Pair.Key.SourceObject.IsValid())
			{
				const UObject* KeySource = Pair.Key.SourceObject.Get();
				if (KeySource != SourceObject && KeySource->GetClass() != SourceObject->GetClass())
				{
					UE_LOG(LogTemp, Log, TEXT("[HandshakeLog] FindAndUnregisterVfxActorFuzzy -> Mismatch SourceObject: VfxKey(%s) vs Target(%s)"),
						*KeySource->GetName(), *SourceObject->GetName());
					continue;
				}
			}

			// Simulated Proxy 등에서 시전 시간을 모른 채(0.0f) 등록된 경우 와일드카드로 즉시 매칭
			if (Pair.Key.ActivationTime == 0.0f && TargetTime == 0.0f)
			{
				BestKey = Pair.Key;
				BestActor = Pair.Value.Get();
				break;
			}

			if (Pair.Key.ActivationTime > 0.0f && TargetTime > 0.0f)
			{
				float Delta = FMath::Abs(Pair.Key.ActivationTime - TargetTime);
				if (Delta < BestDelta)
				{
					BestDelta = Delta;
					BestKey = Pair.Key;
					BestActor = Pair.Value.Get();
				}
			}
		}
	}

	if (BestActor)
	{
		VfxRegistry.Remove(BestKey);
		UE_LOG(LogTemp, Warning, TEXT("[HandshakeLog] FindAndUnregisterVfxActorFuzzy -> FUZZY MATCH FOUND: %s (Delta: %.4f)"),
			*BestActor->GetName(), BestDelta);
		return BestActor;
	}

	UE_LOG(LogTemp, Log, TEXT("[HandshakeLog] FindAndUnregisterVfxActorFuzzy -> NO MATCH FOUND."));
	return nullptr;
}

void UGCN_SummonedRegistrySubsystem::RegisterPendingActorFuzzy(AActor* Instigator, float ActivationTime, AActor* PendingActor, const UObject* SourceObject)
{
	if (!PendingActor) return;

	FString InstigatorName = Instigator ? Instigator->GetName() : TEXT("None");
	FString PendingName = PendingActor->GetName();
	FString SourceName = SourceObject ? SourceObject->GetName() : TEXT("None");

	UE_LOG(LogTemp, Warning, TEXT("[HandshakeLog] RegisterPendingActorFuzzy -> Instigator: %s | Time: %.4f | PendingActor: %s | SourceObject: %s"),
		*InstigatorName, ActivationTime, *PendingName, *SourceName);

	FGCN_SummonedKey RegistryKey(Instigator, ActivationTime, SourceObject);
	PendingActors.Add(RegistryKey, PendingActor);
}
