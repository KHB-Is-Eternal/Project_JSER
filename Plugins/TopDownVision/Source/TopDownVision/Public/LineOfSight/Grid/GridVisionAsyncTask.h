#pragma once

#include "CoreMinimal.h"
#include "Async/AsyncWork.h"
#include "LineOfSight/Grid/GridVisionMap.h"

class UGridVisionMap;

/**
 * Async wrapper for grid vision computation.
 * Runs UGridVisionMap::UpdateGrid on a background thread.
 *
 * Phase 4 implementation — declared here as a stub for future use.
 *
 * Usage:
 *   auto* Task = new FAutoDeleteAsyncTask<FGridVisionAsyncTask>(GridMap, Center, MoveTemp(Providers));
 *   Task->StartBackgroundTask();
 *   // On next frame: check IsDone(), then call GridMap->UploadToGPU()
 */
class FGridVisionAsyncTask : public FNonAbandonableTask
{
	friend class FAutoDeleteAsyncTask<FGridVisionAsyncTask>;

public:
	FGridVisionAsyncTask(
		UGridVisionMap* InGridMap,
		const FVector2D& InWorldCenter,
		TArray<FGridVisionProvider>&& InProviders)
		: GridMap(InGridMap)
		, WorldCenter(InWorldCenter)
		, Providers(MoveTemp(InProviders))
	{
	}

	void DoWork();

	FORCEINLINE TStatId GetStatId() const
	{
		RETURN_QUICK_DECLARE_CYCLE_STAT(FGridVisionAsyncTask, STATGROUP_ThreadPoolAsyncTasks);
	}

private:
	UGridVisionMap* GridMap;
	FVector2D WorldCenter;
	TArray<FGridVisionProvider> Providers;
};
