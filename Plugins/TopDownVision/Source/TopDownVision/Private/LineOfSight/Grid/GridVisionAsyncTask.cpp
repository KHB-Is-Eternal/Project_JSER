#include "LineOfSight/Grid/GridVisionAsyncTask.h"
#include "LineOfSight/Grid/GridVisionMap.h"

void FGridVisionAsyncTask::DoWork()
{
	if (GridMap && GridMap->IsValidLowLevel())
	{
		// This runs on a background thread.
		// It safely updates the GridMap's internal buffers.
		// The GameThread must NOT access these buffers while this task is running.
		GridMap->UpdateGrid(WorldCenter, Providers);
	}
}
