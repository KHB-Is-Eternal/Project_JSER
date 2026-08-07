#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "GameModeBase/PointActor/ER_PointActor.h"
#include "ER_ObjectSubsystem.generated.h"

struct FObjectClassConfig;

USTRUCT(BlueprintType)
struct FObjectInfo
{
	GENERATED_BODY()

public:
	TWeakObjectPtr<AActor> SpawnPoint;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TSubclassOf<AActor> ObjectClass;

	//TWeakObjectPtr<ABaseMonster> SpawnedActor;

	FName DAName;

	bool bIsSpawned = false;

	bool bIsReserved = false;

	ERegionType RegionType;
};

USTRUCT()
struct FSupplySpawnPick
{
	GENERATED_BODY()

public:
	ERegionType Region = ERegionType::None;
	int32 Index = INDEX_NONE;
};

UCLASS()
class PROJECTER_API UER_ObjectSubsystem : public UWorldSubsystem
{
	GENERATED_BODY()
	
public:
	void InitializeObjectPoints(TMap<FName, FObjectClassConfig>& ObjectClass);

	// 항공 보급 스폰
	void PickSupplySpawnIndex();
	void SpawnSupplyObject();

	// 보스 몬스터 스폰
	void PickBossSpawnIndex();
	void SpawnBossObject();

	void RegisterPoint(AActor* Point);
	void UnregisterPoint(AActor* Point);

	// 임시 안전 구역 제어
	void SpawnSafeZone(int32 NextHazardRegionID);
	void DespawnSafeZone();

public:
	TArray<TWeakObjectPtr<AActor>> Points;



private:
	void SpawnObjectInternal(FObjectInfo& Info);

	UPROPERTY()
	bool bIsInitialized = false;

	// 항공 보급 위치를 모아둘 배열
	TMap<ERegionType, TArray<FObjectInfo>> SupplyPointsByRegion;

	// 보스 스폰 위치를 모아둘 배열
	TArray<FObjectInfo> BossPoints;

	// 임시 안전 구역 위치를 모아둘 배열
	TArray<FObjectInfo> SafePoints;

	// 현재 스폰된 임시 안전 구역 액터 (하나만 추적)
	UPROPERTY()
	AActor* SpawnedSafeZoneActor = nullptr;

	// 선정된 항공 보급 위치 저장용 배열
	UPROPERTY()
	TArray<FSupplySpawnPick> PendingSupplyPicks;

	// 선정된 보스 위치 저장용 배열
	UPROPERTY()
	TArray<FSupplySpawnPick> PendingBossPicks;
	// 이후 생, 운 포인트가 나오면 배열 추가하기
};
