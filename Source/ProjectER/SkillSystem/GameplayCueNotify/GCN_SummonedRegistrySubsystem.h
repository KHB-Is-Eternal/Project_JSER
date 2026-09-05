#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "GameplayPrediction.h"
#include "GCN_SummonedRegistrySubsystem.generated.h"

/**
 * 시전자와 예측 키의 조합을 식별자로 사용하는 구조체
 */
USTRUCT()
struct FGCN_SummonedKey
{
	GENERATED_BODY()

	UPROPERTY()
	TWeakObjectPtr<AActor> Instigator;

	UPROPERTY()
	float ActivationTime;

	UPROPERTY()
	TWeakObjectPtr<const UObject> SourceObject;

	FGCN_SummonedKey() : Instigator(nullptr), ActivationTime(0.0f), SourceObject(nullptr) {}
	FGCN_SummonedKey(AActor* InInstigator, float InTime, const UObject* InSourceObject = nullptr) 
		: Instigator(InInstigator), ActivationTime(InTime), SourceObject(InSourceObject) {}

	bool operator==(const FGCN_SummonedKey& Other) const
	{
		bool bSourceMatch = (!SourceObject.IsValid() || !Other.SourceObject.IsValid() || SourceObject == Other.SourceObject || SourceObject->GetClass() == Other.SourceObject->GetClass());
		return Instigator == Other.Instigator && FMath::IsNearlyEqual(ActivationTime, Other.ActivationTime, 1.e-4f) && bSourceMatch;
	}

	friend uint32 GetTypeHash(const FGCN_SummonedKey& K)
	{
		uint32 TimeHash = FCrc::MemCrc32(&K.ActivationTime, sizeof(float));
		uint32 KeyHash = HashCombine(GetTypeHash(K.Instigator), TimeHash);
		return HashCombine(KeyHash, GetTypeHash(K.SourceObject));
	}
};

/**
 * 소환된 비주얼 액터(GCN)와 판정 액터 사이의 동기화를 관리하는 서브시스템
 */
UCLASS()
class PROJECTER_API UGCN_SummonedRegistrySubsystem : public UWorldSubsystem
{
	GENERATED_BODY()

public:
	/** 비주얼 액터를 레지스트리에 등록합니다. (시각 정보 기반) */
	void RegisterVfxActor(AActor* Instigator, float ActivationTime, AActor* VfxActor, const UObject* SourceObject = nullptr);

	/** 시전자와 시각 정보에 매칭되는 비주얼 액터를 찾아 반환하고 레지스트리에서 제거합니다. */
	AActor* GetAndUnregisterVfxActor(AActor* Instigator, float ActivationTime, const UObject* SourceObject = nullptr);

	/** 특정 시전자와 시각 정보에 매칭되는 비주얼 액터가 등록되어 있는지 확인합니다. */
	bool IsVfxActorRegistered(AActor* Instigator, float ActivationTime, const UObject* SourceObject = nullptr) const;

	/** 시전자 기준으로 허용 오차 내에서 비주얼 액터를 검색합니다. (클라이언트-서버 시간 불일치 보상용) */
	AActor* FindAndUnregisterVfxActorFuzzy(AActor* Instigator, float TargetTime, float Tolerance = -1.0f, const UObject* SourceObject = nullptr);

	/** 비주얼 액터가 아직 오지 않았을 때, 판정 액터를 대기열에 등록합니다. */
	void RegisterPendingActorFuzzy(AActor* Instigator, float ActivationTime, AActor* PendingActor, const UObject* SourceObject = nullptr);

	float GetDefaultHandshakeTolerance() const { return DefaultHandshakeTolerance; }

private:
	/** 기본 시각 효과 핸드셰이크 허용 오차 시간 (초) */
	float DefaultHandshakeTolerance = 0.5f;

	/** (시전자, 예측키) -> 비주얼 액터 매핑 */
	TMap<FGCN_SummonedKey, TWeakObjectPtr<AActor>> VfxRegistry;

	/** (시전자, 예측키) -> 판정 액터 매핑 (VFX가 나중에 도착할 경우 대비) */
	TMap<FGCN_SummonedKey, TWeakObjectPtr<AActor>> PendingActors;
};
