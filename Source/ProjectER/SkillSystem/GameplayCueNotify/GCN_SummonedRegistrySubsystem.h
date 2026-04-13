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

	FGCN_SummonedKey() : Instigator(nullptr), ActivationTime(0.0f) {}
	FGCN_SummonedKey(AActor* InInstigator, float InTime) : Instigator(InInstigator), ActivationTime(InTime) {}

	bool operator==(const FGCN_SummonedKey& Other) const
	{
		// float 비교는 네트워크 리플리케이션 특성상 비트 단위로 동일하므로 == 연산이 안전합니다.
		return Instigator == Other.Instigator && FMath::IsNearlyEqual(ActivationTime, Other.ActivationTime, 1.e-4f);
	}

	friend uint32 GetTypeHash(const FGCN_SummonedKey& K)
	{
		// float 값을 uint32 비트로 변환하여 해시 계산
		uint32 TimeHash = FCrc::MemCrc32(&K.ActivationTime, sizeof(float));
		return HashCombine(GetTypeHash(K.Instigator), TimeHash);
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
	void RegisterVfxActor(AActor* Instigator, float ActivationTime, AActor* VfxActor);

	/** 시전자와 시각 정보에 매칭되는 비주얼 액터를 찾아 반환하고 레지스트리에서 제거합니다. */
	AActor* GetAndUnregisterVfxActor(AActor* Instigator, float ActivationTime);

private:
	/** (시전자, 예측키) -> 비주얼 액터 매핑 */
	TMap<FGCN_SummonedKey, TWeakObjectPtr<AActor>> VfxRegistry;
};
