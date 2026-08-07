#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "GridVisionMap.generated.h"

struct FObstacleMaskTile;

/**
 * Lightweight data for a single vision provider.
 * Copied by value for thread safety when used with async tasks.
 */
struct FGridVisionProvider
{
	FVector2D WorldPosition = FVector2D::ZeroVector;
	float VisionRadius = 0.f;
	float Alpha = 0.f;
};

/**
 * CPU-based 2D grid vision map.
 *
 * Replaces the GPU render-target pipeline (CameraLocalRT + M_FeatherBlur + FeatheredRT)
 * with a uint8 grid computed entirely on the CPU.
 *
 * The grid represents the camera's viewport area (WorldCenter ± WorldExtent).
 * Each cell stores a visibility value (0 = dark, 255 = fully visible).
 * After computation, the grid is uploaded to a UTexture2D for use by the
 * post-process material via LayeredLOSInterfaceMID.
 *
 * Usage:
 *   1. Initialize() — allocate grid and create output texture
 *   2. CacheObstacleData() — read pre-baked obstacle tiles into CPU arrays (once)
 *   3. UpdateGrid() — recompute visibility for current frame's providers
 *   4. UploadToGPU() — push result to OutputTexture (game thread only)
 */
UCLASS()
class TOPDOWNVISION_API UGridVisionMap : public UObject
{
	GENERATED_BODY()

public:

	/** Allocate the grid and create the output texture. */
	void Initialize(int32 InResolution, const FVector2D& InWorldCenter, float InWorldExtent);

	/** Read pre-baked obstacle tile textures into CPU-side arrays.
	 *  Call once after WorldObstacleSubsystem finishes loading tiles. */
	void CacheObstacleData(const TArray<FObstacleMaskTile>& Tiles);

	/** Recompute the visibility grid for the providers.
	 *  This is the heavy work — safe to call from a background thread. */
	void UpdateGrid(float DeltaTime, float BlendSpeed, float BlurSharpness, const TArray<FGridVisionProvider>& Providers);

	/** Copy the blurred grid into OutputTexture. Must be called on the game thread. */
	void UploadToGPU();

	UTexture2D* GetOutputTexture() const { return OutputTexture; }
	int32 GetResolution() const { return GridResolution; }

private:

	// --- Configuration --- //
	int32 GridResolution = 256;
	float WorldExtent = 0.f;
	FVector2D WorldCenter = FVector2D::ZeroVector;

	// --- Grid data --- //
	TArray<uint8> TargetVisibilityGrid;
	TArray<uint8> CurrentVisibilityGrid;
	TArray<uint8> BlurredGrid;
	TArray<FColor> ColorGrid;

	// --- Output --- //
	UPROPERTY(Transient)
	TObjectPtr<UTexture2D> OutputTexture = nullptr;

	// --- Cached obstacle data (CPU-side copy of pre-baked tiles) --- //
	struct FCachedObstacleTile
	{
		TArray<uint8> PixelData;   // R channel, row-major
		int32 Width = 0;
		int32 Height = 0;
		FBox2D WorldBounds = FBox2D(ForceInit);
	};
	TArray<FCachedObstacleTile> CachedObstacles;

	// --- Internal helpers --- //
	FIntPoint WorldToGrid(const FVector2D& WorldPos) const;
	FVector2D GridToWorld(int32 X, int32 Y) const;
	uint8 SampleObstacle(const FVector2D& WorldPos) const;
	void ComputeProviderVisibility(const FGridVisionProvider& Provider, float BlurSharpness);
	void ApplyBoxBlur();
};
