#include "CharacterSystem/Character/PathfindingBenchmarkActor.h"
#include "CharacterSystem/Character/BaseCharacter.h"
#include "CharacterSystem/Data/CharacterData.h"
#include "Engine/World.h"
#include "TimerManager.h"
#include "AIController.h"
#include "NavigationSystem.h"

APathfindingBenchmarkActor::APathfindingBenchmarkActor()
{
	PrimaryActorTick.bCanEverTick = false;
	bReplicates = false;
}

void APathfindingBenchmarkActor::BeginPlay()
{
	Super::BeginPlay();

	if (bAutoStart)
	{
		FTimerHandle DelayHandle;
		GetWorldTimerManager().SetTimer(DelayHandle, this,
			&APathfindingBenchmarkActor::StartBenchmark, 3.0f, false);
	}
}

void APathfindingBenchmarkActor::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	StopBenchmark();
	Super::EndPlay(EndPlayReason);
}

void APathfindingBenchmarkActor::StartBenchmark()
{
	if (!CharacterClass)
	{
		UE_LOG(LogTemp, Error, TEXT("[Benchmark] CharacterClass is not set! Assign it in the Details panel."));
		return;
	}

	UWorld* const World = GetWorld();
	if (!World)
	{
		UE_LOG(LogTemp, Error, TEXT("[Benchmark] World is null!"));
		return;
	}

	UNavigationSystemV1* NavSys = FNavigationSystem::GetCurrent<UNavigationSystemV1>(World);
	if (!NavSys)
	{
		UE_LOG(LogTemp, Error, TEXT("[Benchmark] NavigationSystem is null! Ensure NavMeshBoundsVolume exists in the level."));
		return;
	}

	// MoveTargets가 비어있으면 랜덤 NavMesh 좌표를 사용한다고 알림
	if (MoveTargets.Num() == 0)
	{
		UE_LOG(LogTemp, Warning, TEXT("[Benchmark] MoveTargets is empty -> Using random NavMesh locations (Radius: %.0f)"), RandomMoveRadius);
	}

	// 기존 스폰 정리
	StopBenchmark();

	UE_LOG(LogTemp, Warning, TEXT("[Benchmark] === Pathfinding Benchmark Start - %d Units ==="), SpawnCount);
	UE_LOG(LogTemp, Warning, TEXT("[Benchmark] CharacterClass: %s"), *CharacterClass->GetName());
	UE_LOG(LogTemp, Warning, TEXT("[Benchmark] Spawn Center: %s | Radius: %.0f"), *GetActorLocation().ToString(), SpawnRadius);

	// 캐릭터 스폰
	int32 FailCount = 0;
	for (int32 i = 0; i < SpawnCount; ++i)
	{
		const float Angle = (2.0f * PI * i) / SpawnCount;
		const FVector Offset(FMath::Cos(Angle) * SpawnRadius, FMath::Sin(Angle) * SpawnRadius, 0.0f);
		FVector SpawnLoc = GetActorLocation() + Offset;

		// NavMesh 위에 스폰되도록 보정
		FNavLocation NavLoc;
		if (NavSys->ProjectPointToNavigation(SpawnLoc, NavLoc, FVector(500.0f, 500.0f, 500.0f)))
		{
			SpawnLoc = NavLoc.Location;
			SpawnLoc.Z += 96.0f; // 캡슐 반높이만큼 올려서 바닥에 묻히지 않게
		}

		FActorSpawnParameters SpawnParams;
		SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;

		ABaseCharacter* NewChar = World->SpawnActor<ABaseCharacter>(CharacterClass, SpawnLoc, FRotator::ZeroRotator, SpawnParams);
		if (NewChar)
		{
			if (TestHeroData)
			{
				NewChar->HeroData = TestHeroData;
			}

			// Controller 부여 -> PossessedBy -> InitAbilitySystem 트리거
			NewChar->SpawnDefaultController();

			SpawnedCharacters.Add(NewChar);
		}
		else
		{
			FailCount++;
			UE_LOG(LogTemp, Error, TEXT("[Benchmark] SpawnActor FAILED for unit %d at %s"), i, *SpawnLoc.ToString());
		}
	}

	UE_LOG(LogTemp, Warning, TEXT("[Benchmark] Spawn Result: %d/%d Succeeded, %d Failed"),
		SpawnedCharacters.Num(), SpawnCount, FailCount);

	if (SpawnedCharacters.Num() == 0)
	{
		UE_LOG(LogTemp, Error, TEXT("[Benchmark] No characters spawned! Check CharacterClass and spawn location."));
		return;
	}

	// 주기적 이동 명령 타이머
	GetWorldTimerManager().SetTimer(BenchmarkTimerHandle, this,
		&APathfindingBenchmarkActor::IssueMovementCommands, CommandInterval, true, 0.5f);
}

void APathfindingBenchmarkActor::StopBenchmark()
{
	GetWorldTimerManager().ClearTimer(BenchmarkTimerHandle);

	for (ABaseCharacter* Char : SpawnedCharacters)
	{
		if (IsValid(Char))
		{
			Char->Destroy();
		}
	}
	SpawnedCharacters.Empty();

	UE_LOG(LogTemp, Warning, TEXT("[Benchmark] === Benchmark Stopped ==="));
}

void APathfindingBenchmarkActor::IssueMovementCommands()
{
	int32 IssuedCount = 0;
	int32 InvalidCount = 0;
	int32 NavFailCount = 0;

	for (ABaseCharacter* Char : SpawnedCharacters)
	{
		if (!IsValid(Char))
		{
			InvalidCount++;
			continue;
		}

		FVector Destination = FVector::ZeroVector;

		if (MoveTargets.Num() > 0)
		{
			// MoveTargets가 있으면 랜덤 하나 선택
			const int32 TargetIdx = FMath::RandRange(0, MoveTargets.Num() - 1);
			if (IsValid(MoveTargets[TargetIdx]))
			{
				Destination = MoveTargets[TargetIdx]->GetActorLocation();
			}
			else
			{
				continue;
			}
		}
		else
		{
			// MoveTargets가 비어있으면 NavMesh 위 랜덤 좌표 사용
			// 캐릭터 자신의 위치를 기준으로 검색 (BenchmarkActor가 아님)
			Destination = GetRandomNavMeshLocation(Char->GetActorLocation());
			if (Destination.IsZero())
			{
				NavFailCount++;
				continue;
			}
		}

		Char->MoveToLocation(Destination);
		IssuedCount++;
	}

	// 실패가 있을 때만 상세 로그 출력
	if (InvalidCount > 0 || NavFailCount > 0)
	{
		UE_LOG(LogTemp, Warning, TEXT("[Benchmark] Issued: %d | Invalid: %d | NavFail: %d (of %d total)"),
			IssuedCount, InvalidCount, NavFailCount, SpawnedCharacters.Num());
	}
	else
	{
		UE_LOG(LogTemp, Log, TEXT("[Benchmark] Movement commands issued to %d/%d units"),
			IssuedCount, SpawnedCharacters.Num());
	}
}

FVector APathfindingBenchmarkActor::GetRandomNavMeshLocation(const FVector& Origin) const
{
	UNavigationSystemV1* NavSys = FNavigationSystem::GetCurrent<UNavigationSystemV1>(GetWorld());
	if (!NavSys) return FVector::ZeroVector;

	FNavLocation RandomLoc;

	// GetRandomPointInNavigableRadius: Reachable 조건 없이 NavMesh 위의 아무 점 검색 (더 관대함)
	if (NavSys->GetRandomPointInNavigableRadius(Origin, RandomMoveRadius, RandomLoc))
	{
		return RandomLoc.Location;
	}

	// 실패 시 더 작은 반경으로 재시도
	if (NavSys->GetRandomPointInNavigableRadius(Origin, 1000.0f, RandomLoc))
	{
		return RandomLoc.Location;
	}

	return FVector::ZeroVector;
}
