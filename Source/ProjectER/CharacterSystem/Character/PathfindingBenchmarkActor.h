#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "PathfindingBenchmarkActor.generated.h"

class ABaseCharacter;
class UCharacterData;

/**
 * 레벨에 배치하면 지정된 수의 캐릭터를 스폰하고
 * 목적지로 동시 이동 명령을 내려 길찾기 부하를 측정하는 테스트 액터.
 *
 * 사용법:
 * 1. 레벨에 이 액터를 배치 (NavMesh 위에)
 * 2. Details 패널에서 CharacterClass, SpawnCount 설정
 * 3. MoveTargets가 비어있으면 NavMesh 위의 랜덤 좌표를 자동으로 사용
 * 4. PIE 실행 -> 자동으로 유닛 스폰 + 이동 시작
 * 5. 콘솔: stat ProjectER_Pathfinding 으로 성능 확인
 * 6. Output Log에서 [Pathfinding Metrics] 로그 확인
 */
UCLASS()
class PROJECTER_API APathfindingBenchmarkActor : public AActor
{
	GENERATED_BODY()

public:
	APathfindingBenchmarkActor();

	/** 테스트 시작 */
	UFUNCTION(BlueprintCallable, Category = "Benchmark")
	void StartBenchmark();

	/** 테스트 중지 및 스폰 정리 */
	UFUNCTION(BlueprintCallable, Category = "Benchmark")
	void StopBenchmark();

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:
	/** 주기적 이동 명령 발행 */
	void IssueMovementCommands();

	/** NavMesh 위의 랜덤 좌표 반환 */
	FVector GetRandomNavMeshLocation(const FVector& Origin) const;

	/** 스폰된 테스트 캐릭터 배열 */
	UPROPERTY()
	TArray<TObjectPtr<ABaseCharacter>> SpawnedCharacters;

	/** 반복 타이머 핸들 */
	FTimerHandle BenchmarkTimerHandle;

protected:
	/** 스폰할 캐릭터 클래스 */
	UPROPERTY(EditAnywhere, Category = "Benchmark|Setup")
	TSubclassOf<ABaseCharacter> CharacterClass;

	/** 스폰할 캐릭터 수 */
	UPROPERTY(EditAnywhere, Category = "Benchmark|Setup", meta = (ClampMin = "1", ClampMax = "50"))
	int32 SpawnCount = 10;

	/** 스폰 위치 (이 액터 주변 반경) */
	UPROPERTY(EditAnywhere, Category = "Benchmark|Setup")
	float SpawnRadius = 500.0f;

	/**
	 * (선택) 이동 목적지 액터 배열.
	 * 비어있으면 NavMesh 위의 랜덤 좌표를 자동으로 사용합니다.
	 */
	UPROPERTY(EditAnywhere, Category = "Benchmark|Setup")
	TArray<TObjectPtr<AActor>> MoveTargets;

	/** 랜덤 목적지 검색 반경 (MoveTargets가 비어있을 때 사용) */
	UPROPERTY(EditAnywhere, Category = "Benchmark|Setup")
	float RandomMoveRadius = 5000.0f;

	/** 이동 명령 재발행 간격 (초) */
	UPROPERTY(EditAnywhere, Category = "Benchmark|Setup")
	float CommandInterval = 2.0f;

	/** BeginPlay에서 자동 시작 여부 */
	UPROPERTY(EditAnywhere, Category = "Benchmark|Setup")
	bool bAutoStart = false;

	/** HeroData (캐릭터 초기화용) */
	UPROPERTY(EditAnywhere, Category = "Benchmark|Setup")
	TObjectPtr<UCharacterData> TestHeroData;
};
