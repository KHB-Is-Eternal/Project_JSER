#include "LineOfSight/Grid/GridVisionMap.h"

#include "Engine/Texture2D.h"
#include "LineOfSight/WorldObstacle/ObstacleData.h"
#include "TopDownVisionDebug.h"
#include "RenderingThread.h"
#include "RHICommandList.h"


// -------------------------------------------------------------------------- //
//  Initialize
// -------------------------------------------------------------------------- //

void UGridVisionMap::Initialize(int32 InResolution, const FVector2D& InWorldCenter, float InWorldExtent)
{
	GridResolution = FMath::Max(8, InResolution);
	WorldExtent = FMath::Max(1.0f, InWorldExtent);
	WorldCenter = InWorldCenter;

	const int32 TotalCells = GridResolution * GridResolution;
	TargetVisibilityGrid.SetNumZeroed(TotalCells);
	CurrentVisibilityGrid.SetNumZeroed(TotalCells);
	BlurredGrid.SetNumZeroed(TotalCells);
	ColorGrid.SetNumZeroed(TotalCells);

	OutputTexture = UTexture2D::CreateTransient(GridResolution, GridResolution, PF_B8G8R8A8);
	if (OutputTexture)
	{
		OutputTexture->Filter = TF_Bilinear;
		OutputTexture->AddressX = TA_Clamp;
		OutputTexture->AddressY = TA_Clamp;
		OutputTexture->SRGB = 0;
		OutputTexture->NeverStream = true;
		OutputTexture->UpdateResource();

		UE_LOG(LOSVision, Log,
			TEXT("UGridVisionMap::Initialize >> Resolution=%d, WorldExtent=%.0f"),
			GridResolution, WorldExtent);
	}
	else
	{
		UE_LOG(LOSVision, Error,
			TEXT("UGridVisionMap::Initialize >> Failed to create output texture"));
	}
}

// -------------------------------------------------------------------------- //
//  Obstacle cache
// -------------------------------------------------------------------------- //

void UGridVisionMap::CacheObstacleData(const TArray<FObstacleMaskTile>& Tiles)
{
	CachedObstacles.Reset();
	CachedObstacles.Reserve(Tiles.Num());

	for (const FObstacleMaskTile& Tile : Tiles)
	{
		if (!Tile.Mask)
			continue;

		FTexturePlatformData* PlatformData = Tile.Mask->GetPlatformData();
		if (!PlatformData || PlatformData->Mips.IsEmpty())
		{
			UE_LOG(LOSVision, Warning,
				TEXT("UGridVisionMap::CacheObstacleData >> No platform data: %s"),
				*Tile.Mask->GetName());
			continue;
		}

		FTexture2DMipMap& Mip = PlatformData->Mips[0];
		const int32 Width = Mip.SizeX;
		const int32 Height = Mip.SizeY;
		const int64 BulkSize = Mip.BulkData.GetBulkDataSize();

		if (BulkSize == 0)
		{
			UE_LOG(LOSVision, Warning,
				TEXT("UGridVisionMap::CacheObstacleData >> Bulk data empty: %s"),
				*Tile.Mask->GetName());
			continue;
		}

		const void* RawData = Mip.BulkData.LockReadOnly();
		if (!RawData)
		{
			Mip.BulkData.Unlock();
			continue;
		}

		FCachedObstacleTile& Cached = CachedObstacles.AddDefaulted_GetRef();
		Cached.Width = Width;
		Cached.Height = Height;
		Cached.WorldBounds = Tile.WorldBounds;
		Cached.PixelData.SetNumUninitialized(Width * Height);

		const int32 BytesPerPixel = static_cast<int32>(BulkSize / (Width * Height));

		if (BytesPerPixel >= 4)
		{
			// BGRA8 / RGBA8 — extract R channel
			const FColor* Colors = static_cast<const FColor*>(RawData);
			for (int32 i = 0; i < Width * Height; ++i)
			{
				Cached.PixelData[i] = Colors[i].R;
			}
		}
		else if (BytesPerPixel == 1)
		{
			// Grayscale — direct copy
			FMemory::Memcpy(Cached.PixelData.GetData(), RawData, Width * Height);
		}
		else
		{
			UE_LOG(LOSVision, Warning,
				TEXT("UGridVisionMap::CacheObstacleData >> Unsupported BPP=%d: %s"),
				BytesPerPixel, *Tile.Mask->GetName());
			CachedObstacles.Pop();
			Mip.BulkData.Unlock();
			continue;
		}

		Mip.BulkData.Unlock();

		UE_LOG(LOSVision, Log,
			TEXT("UGridVisionMap::CacheObstacleData >> Cached tile: %s (%dx%d, BPP=%d)"),
			*Tile.Mask->GetName(), Width, Height, BytesPerPixel);
	}

	UE_LOG(LOSVision, Log,
		TEXT("UGridVisionMap::CacheObstacleData >> %d tiles cached"),
		CachedObstacles.Num());
}

// -------------------------------------------------------------------------- //
//  Grid update
// -------------------------------------------------------------------------- //

void UGridVisionMap::UpdateGrid(float DeltaTime, float BlendSpeed, float BlurSharpness, const TArray<FGridVisionProvider>& Providers)
{
	// Clear target grid
	FMemory::Memzero(TargetVisibilityGrid.GetData(), TargetVisibilityGrid.Num());

	// Compute target visibility for each provider
	for (const FGridVisionProvider& Provider : Providers)
	{
		if (Provider.Alpha <= KINDA_SMALL_NUMBER)
			continue;

		ComputeProviderVisibility(Provider, BlurSharpness);
	}

	// Temporal Blending
	const int32 NumCells = TargetVisibilityGrid.Num();
	const float Alpha = FMath::Clamp(DeltaTime * BlendSpeed, 0.f, 1.f);

	if (Alpha >= 1.0f - KINDA_SMALL_NUMBER || BlendSpeed <= 0.f)
	{
		// Instant snap
		FMemory::Memcpy(CurrentVisibilityGrid.GetData(), TargetVisibilityGrid.GetData(), NumCells);
	}
	else
	{
		// Smooth Lerp
		for (int32 i = 0; i < NumCells; ++i)
		{
			CurrentVisibilityGrid[i] = static_cast<uint8>(FMath::Lerp(
				static_cast<float>(CurrentVisibilityGrid[i]), 
				static_cast<float>(TargetVisibilityGrid[i]), 
				Alpha));
		}
	}

	ApplyBoxBlur();
}

// -------------------------------------------------------------------------- //
//  Per-provider ray marching
// -------------------------------------------------------------------------- //

void UGridVisionMap::ComputeProviderVisibility(const FGridVisionProvider& Provider, float BlurSharpness)
{
	const FIntPoint CenterGrid = WorldToGrid(Provider.WorldPosition);
	const float CellSize = (WorldExtent * 2.f) / GridResolution;
	const float GridRadius = FMath::Min(Provider.VisionRadius / CellSize, static_cast<float>(GridResolution * 2));
	const float RadiusSq = GridRadius * GridRadius;

	// Scale ray count with radius to guarantee dense coverage at the outer edge (Circumference = 2 * PI * R)
	// We use ~12 rays per radius unit to ensure at least ~2 rays per perimeter cell
	const int32 NumRays = FMath::Max(128, FMath::CeilToInt(GridRadius * 12.0f));

	for (int32 RayIdx = 0; RayIdx < NumRays; ++RayIdx)
	{
		const float Angle = (static_cast<float>(RayIdx) / NumRays) * 2.f * PI;
		const float DirX = FMath::Cos(Angle);
		const float DirY = FMath::Sin(Angle);

		// Use 0.5f step to guarantee no skipped cells along the ray path
		for (float Step = 0.f; Step <= GridRadius; Step += 0.5f)
		{
			const int32 CellX = CenterGrid.X + FMath::RoundToInt(DirX * Step);
			const int32 CellY = CenterGrid.Y + FMath::RoundToInt(DirY * Step);

			// Bounds check
			if (CellX < 0 || CellX >= GridResolution ||
				CellY < 0 || CellY >= GridResolution)
			{
				break;
			}

			// Obstacle check at this world position
			const FVector2D CellWorldPos = GridToWorld(CellX, CellY);
			if (SampleObstacle(CellWorldPos) > 128)
				break; // ray blocked by wall

			// Distance-based falloff
			const float DistSq = Step * Step;
			if (DistSq > RadiusSq)
				break;

			// 시야 경계를 더 뚜렷하게(블러 영역 축소) 만들기 위해 감쇄 강도를 증폭합니다.
			// 배수(예: 3.0f)가 클수록 외곽선이 선명해지고 블러가 줄어듭니다.
			const float Falloff = FMath::Clamp((1.f - (DistSq / RadiusSq)) * BlurSharpness, 0.f, 1.f);
			const uint8 Value = static_cast<uint8>(FMath::Clamp(
				FMath::RoundToInt(Falloff * Provider.Alpha * 255.f), 0, 255));

			// Max blend — brightest provider wins
			const int32 Idx = CellY * GridResolution + CellX;
			TargetVisibilityGrid[Idx] = FMath::Max(TargetVisibilityGrid[Idx], Value);
		}
	}
}

// -------------------------------------------------------------------------- //
//  Blur
// -------------------------------------------------------------------------- //

void UGridVisionMap::ApplyBoxBlur()
{
	const int32 Res = GridResolution;

	for (int32 Y = 0; Y < Res; ++Y)
	{
		for (int32 X = 0; X < Res; ++X)
		{
			int32 Sum = 0;
			int32 Count = 0;

			for (int32 DY = -1; DY <= 1; ++DY)
			{
				for (int32 DX = -1; DX <= 1; ++DX)
				{
					const int32 SX = X + DX;
					const int32 SY = Y + DY;

					if (SX >= 0 && SX < Res && SY >= 0 && SY < Res)
					{
						Sum += CurrentVisibilityGrid[SY * Res + SX];
						++Count;
					}
				}
			}

			BlurredGrid[Y * Res + X] = static_cast<uint8>(Sum / Count);
		}
	}
}

// -------------------------------------------------------------------------- //
//  GPU upload
// -------------------------------------------------------------------------- //

void UGridVisionMap::UploadToGPU()
{
	if (!OutputTexture || BlurredGrid.IsEmpty())
		return;

	const int32 TotalCells = GridResolution * GridResolution;
	for (int32 i = 0; i < TotalCells; ++i)
	{
		const uint8 Val = BlurredGrid[i];
		ColorGrid[i] = FColor(Val, Val, Val, Val);
	}

	// Safely queue a texture region update without recreating the RHI resource
	FUpdateTextureRegion2D* Region = new FUpdateTextureRegion2D(0, 0, 0, 0, GridResolution, GridResolution);
	
	// Allocate a temporary buffer that will be freed by the render thread after upload
	uint8* TextureData = (uint8*)FMemory::Malloc(ColorGrid.Num() * sizeof(FColor));
	FMemory::Memcpy(TextureData, ColorGrid.GetData(), ColorGrid.Num() * sizeof(FColor));

	OutputTexture->UpdateTextureRegions(0, 1, Region, GridResolution * 4, 4, TextureData, [](uint8* SrcData, const FUpdateTextureRegion2D* Regions)
	{
		FMemory::Free(SrcData);
		delete Regions;
	});
}

// -------------------------------------------------------------------------- //
//  Coordinate helpers
// -------------------------------------------------------------------------- //

FIntPoint UGridVisionMap::WorldToGrid(const FVector2D& WorldPos) const
{
	const float CellSize = (WorldExtent * 2.f) / GridResolution;
	const int32 X = FMath::FloorToInt((WorldPos.X - WorldCenter.X + WorldExtent) / CellSize);
	const int32 Y = FMath::FloorToInt((WorldPos.Y - WorldCenter.Y + WorldExtent) / CellSize);
	return FIntPoint(
		FMath::Clamp(X, 0, GridResolution - 1),
		FMath::Clamp(Y, 0, GridResolution - 1));
}

FVector2D UGridVisionMap::GridToWorld(int32 X, int32 Y) const
{
	const float CellSize = (WorldExtent * 2.f) / GridResolution;
	return FVector2D(
		WorldCenter.X - WorldExtent + (X + 0.5f) * CellSize,
		WorldCenter.Y - WorldExtent + (Y + 0.5f) * CellSize);
}

uint8 UGridVisionMap::SampleObstacle(const FVector2D& WorldPos) const
{
	for (const FCachedObstacleTile& Tile : CachedObstacles)
	{
		if (!Tile.WorldBounds.IsInside(WorldPos))
			continue;

		const FVector2D BoundsSize = Tile.WorldBounds.GetSize();
		if (BoundsSize.X <= 0.f || BoundsSize.Y <= 0.f)
			continue;

		const FVector2D Relative = WorldPos - Tile.WorldBounds.Min;
		const float U = Relative.X / BoundsSize.X;
		const float V = Relative.Y / BoundsSize.Y;

		const int32 PixelX = FMath::Clamp(FMath::FloorToInt(U * Tile.Width), 0, Tile.Width - 1);
		const int32 PixelY = FMath::Clamp(FMath::FloorToInt(V * Tile.Height), 0, Tile.Height - 1);

		return Tile.PixelData[PixelY * Tile.Width + PixelX];
	}

	return 0; // No obstacle data at this position
}
