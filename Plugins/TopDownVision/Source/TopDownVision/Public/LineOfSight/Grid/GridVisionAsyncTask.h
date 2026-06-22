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
		float InDeltaTime,
		float InBlendSpeed,
		TArray<FGridVisionProvider>&& InProviders)
		: GridMap(InGridMap)
		, DeltaTime(InDeltaTime)
		, BlendSpeed(InBlendSpeed)
		, Providers(MoveTemp(InProviders))
	{
	}

	void DoWork()
	{
		if (GridMap)
		{
			GridMap->UpdateGrid(DeltaTime, BlendSpeed, Providers);
		}
	}

	FORCEINLINE TStatId GetStatId() const
	{
		RETURN_QUICK_DECLARE_CYCLE_STAT(FGridVisionAsyncTask, STATGROUP_ThreadPoolAsyncTasks);
	}

private:
	UGridVisionMap* GridMap = nullptr;
	float DeltaTime = 0.f;
	float BlendSpeed = 0.f;
	TArray<FGridVisionProvider> Providers;
};
