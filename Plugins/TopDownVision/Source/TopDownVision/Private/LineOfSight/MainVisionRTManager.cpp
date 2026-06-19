#include "LineOfSight/MainVisionRTManager.h"

#include "RenderGraphUtils.h"
#include "Engine/World.h"
#include "Engine/Canvas.h"
#include "Kismet/KismetRenderingLibrary.h"
#include "Materials/MaterialParameterCollectionInstance.h"
#include "GameFramework/GameStateBase.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"

#include "TopDownVision/Public/LineOfSight/VisionComps/Vision_VisualComp.h"
#include "LineOfSight/GPU/LOSStampPass.h"
#include "LineOfSight/Management/VisionGameStateComp.h"
#include "LineOfSight/Management/Subsystem/LOSVisionSubsystem.h"
#include "LineOfSight/Management/Subsystem/LOSRequirementPoolSubsystem.h"
#include "LineOfSight/Management/Subsystem/WorldObstacleSubsystem.h"
#include "LineOfSight/Management/VisionPlayerStateComp.h"
#include "LineOfSight/Grid/GridVisionMap.h"
#include "TopDownVisionDebug.h"


UMainVisionRTManager::UMainVisionRTManager()
{
    PrimaryComponentTick.bCanEverTick = true;
    PrimaryComponentTick.bStartWithTickEnabled = true;

    UE_LOG(LOSVision, Log,
        TEXT("UMainVisionRTManager::Constructor >> Component constructed"));
}

void UMainVisionRTManager::BeginPlay()
{
    Super::BeginPlay();
    UE_LOG(LOSVision, Log, TEXT("UMainVisionRTManager::BeginPlay >> BeginPlay called"));
}

// -------------------------------------------------------------------------- //
//  Initialize
// -------------------------------------------------------------------------- //

void UMainVisionRTManager::InitializeMainVisionRTComp()
{
    if (!ShouldRunClientLogic())
        return;

    GridVisionMap = NewObject<UGridVisionMap>(this);
    check(GridVisionMap);
    GridVisionMap->Initialize(GridResolution, FVector2D::ZeroVector, MapWorldExtent);

    // Cache pre-baked obstacle data from the world subsystem
    if (UWorldObstacleSubsystem* ObstacleSub = GetWorld()->GetSubsystem<UWorldObstacleSubsystem>())
    {
        GridVisionMap->CacheObstacleData(ObstacleSub->GetTiles());
    }
    else
    {
        UE_LOG(LOSVision, Warning,
            TEXT("UMainVisionRTManager::InitializeMainVisionRTComp >> WorldObstacleSubsystem not found — grid will have no obstacles"));
    }

    // Create post-process material and bind the grid output texture
    if (LayeredLOSInterfaceMaterial)
    {
        LayeredLOSInterfaceMID = UMaterialInstanceDynamic::Create(
            LayeredLOSInterfaceMaterial, this);
        if (LayeredLOSInterfaceMID && GridVisionMap->GetOutputTexture())
        {
            LayeredLOSInterfaceMID->SetTextureParameterValue(
                LayeredLOSTextureParam, GridVisionMap->GetOutputTexture());
        }
    }

    // MPC setup (Static 1-time setup for Fog of War)
    MPCInstance = GetWorld()->GetParameterCollectionInstance(PostProcessMPC);
    if (MPCInstance)
    {
        MPCInstance->SetVectorParameterValue(MPCLocationParam, FLinearColor(0.f, 0.f, 0.f));
        MPCInstance->SetScalarParameterValue(MPCVisibleRangeParam, MapWorldExtent);
    }

    UE_LOG(LOSVision, Log,
        TEXT("UMainVisionRTManager::InitializeMainVisionRTComp >> Grid Vision initialized (Resolution=%d)"),
        GridResolution);

    Activate();
}

// -------------------------------------------------------------------------- //

static bool RectOverlapsWorld(
    const FVector& ACenter, float AHalfSize,
    const FVector& BCenter, float BHalfSize)
{
    return !(
        FMath::Abs(ACenter.X - BCenter.X) > (AHalfSize + BHalfSize) ||
        FMath::Abs(ACenter.Y - BCenter.Y) > (AHalfSize + BHalfSize)
    );
}

// -------------------------------------------------------------------------- //
//  Update
// -------------------------------------------------------------------------- //

void UMainVisionRTManager::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (PendingGridTask)
	{
		PendingGridTask->EnsureCompletion();
		delete PendingGridTask;
		PendingGridTask = nullptr;
	}

	Super::EndPlay(EndPlayReason);
}

void UMainVisionRTManager::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	if (!ShouldRunClientLogic())
		return;

	// Periodic update for standing still / environmental changes
	TimeSinceLastUpdate += DeltaTime;
	if (TimeSinceLastUpdate >= UpdateInterval)
	{
		TimeSinceLastUpdate = 0.f;
		UpdateCameraLOS();
	}
}



void UMainVisionRTManager::UpdateCameraLOS()
{
    if (!ShouldRunClientLogic())
        return;

    if (!GridVisionMap)
        return;

    // 1. If a task is already running, check if it's done
    if (PendingGridTask)
    {
        if (PendingGridTask->IsDone())
        {
            // Task is finished! Upload results to GPU
            GridVisionMap->UploadToGPU();

            // Write the CPU Grid result directly to the legacy RTs so that hardcoded Editor materials continue to work
            if (UTexture2D* GridOutput = GridVisionMap->GetOutputTexture())
            {
                auto DrawToRT = [&](UTextureRenderTarget2D* TargetRT)
                {
                    if (!TargetRT) return;

                    // 에셋의 원래 크기(예: 256)를 무시하고 GridResolution(예: 512)에 맞춰 렌더 타겟의 해상도를 동적으로 변경합니다.
                    if (TargetRT->SizeX != GridResolution || TargetRT->SizeY != GridResolution)
                    {
                        TargetRT->ResizeTarget(GridResolution, GridResolution);
                    }

                    UCanvas* Canvas = nullptr;
                    FDrawToRenderTargetContext Context;
                    FVector2D RTSize(TargetRT->SizeX, TargetRT->SizeY);

                    UKismetRenderingLibrary::BeginDrawCanvasToRenderTarget(
                        GetWorld(), TargetRT, Canvas, RTSize, Context);
                    
                    if (Canvas)
                    {
                        FCanvasTileItem Tile(
                            FVector2D::ZeroVector, GridOutput->GetResource(), RTSize,
                            FVector2D::ZeroVector, FVector2D(1, 1), FLinearColor::White);
                        Tile.BlendMode = SE_BLEND_Opaque;
                        Canvas->DrawItem(Tile);
                    }
                    UKismetRenderingLibrary::EndDrawCanvasToRenderTarget(GetWorld(), Context);
                };

                // This takes < 0.1ms compared to the 13ms ApplyFeatheredBlurToRT
                DrawToRT(CameraLocalRT);
                DrawToRT(FeatheredRT);
            }

            delete PendingGridTask;
            PendingGridTask = nullptr;
        }
        // If still running, we just wait and DO NOT gather providers to save CPU
    }
    
    // 2. Start a new task only if no task is running
    if (!PendingGridTask)
    {
        TArray<UVision_VisualComp*> ActiveProviders;
        if (GetVisibleProviders(ActiveProviders))
        {
            TArray<FGridVisionProvider> GridProviders;
            GridProviders.Reserve(ActiveProviders.Num());

            const FVector StaticMapCenter = FVector::ZeroVector;

            for (UVision_VisualComp* Provider : ActiveProviders)
            {
                if (!Provider || !Provider->GetOwner() || !Provider->IsVisionProvider())
                    continue;

                const bool bInRange = RectOverlapsWorld(
                    StaticMapCenter, MapWorldExtent,
                    Provider->GetOwner()->GetActorLocation(), Provider->GetVisibleRange());

                if (bInRange)
                {
                    FGridVisionProvider GridProv;
                    GridProv.WorldPosition = FVector2D(Provider->GetOwner()->GetActorLocation());
                    GridProv.VisionRadius = Provider->GetVisibleRange();
                    GridProv.Alpha = Provider->GetVisibilityAlpha();
                    GridProviders.Add(GridProv);
                }
            }

            // Start a new background task
            PendingGridTask = new FAsyncTask<FGridVisionAsyncTask>(
                GridVisionMap, UpdateInterval, TemporalBlendSpeed, MoveTemp(GridProviders));
            PendingGridTask->StartBackgroundTask();
        }
    }
}



bool UMainVisionRTManager::GetVisibleProviders(
    TArray<UVision_VisualComp*>& OutProviders) const
{
    ULOSVisionSubsystem* Subsystem = GetWorld()->GetSubsystem<ULOSVisionSubsystem>();
    if (!Subsystem)
    {
        UE_LOG(LOSVision, Warning,
            TEXT("UMainVisionRTManager::GetVisibleProviders >> Subsystem not found"));
        return false;
    }

    UVisionPlayerStateComp* LocalVisionPS =
        ULOSVisionSubsystem::GetLocalVisionPS(GetWorld());

    if (LocalVisionPS && LocalVisionPS->GetTeamChannel() != EVisionChannel::None)
    {
        // Use CanSeeTeam across all providers — identical filter to what
        // InitializeSameTeamEvaluators and ReevaluateTargetVisibility use.
        // This ensures the RT stamps exactly the providers whose evaluators
        // are running locally.
        for (UVision_VisualComp* P : Subsystem->GetAllProviders())
        {
            if (!P || !P->GetOwner())
                continue;

            if (LocalVisionPS->CanSeeTeam(P->GetVisionChannel()))
                OutProviders.AddUnique(P);
        }
    }

    // Fallback — team not yet replicated, use the local pawn's own provider
    // so the screen is never blank during early frames.
    if (OutProviders.IsEmpty())
    {
        APlayerController* PC = GEngine->GetFirstLocalPlayerController(GetWorld());
        if (APawn* Pawn = PC ? PC->GetPawn() : nullptr)
            if (UVision_VisualComp* Own =
                Pawn->FindComponentByClass<UVision_VisualComp>())
                OutProviders.Add(Own);
    }

    return OutProviders.Num() > 0;
}

bool UMainVisionRTManager::ShouldRunClientLogic() const
{
    // 에디터 미리보기(프리뷰 월드) 창에서 렌더 타겟 에셋을 무단으로 갱신하는 것을 방지합니다.
    if (GetWorld() && !GetWorld()->IsGameWorld())
    {
        return false;
    }

    return GetNetMode() != NM_DedicatedServer;
}



uint32 UMainVisionRTManager::MakeChannelBitMask(
    const TArray<EVisionChannel>& ChannelEnums)
{
    uint32 Mask = 0;
    for (EVisionChannel Channel : ChannelEnums)
    {
        if (Channel == EVisionChannel::None)
            continue;

        uint8 Index = static_cast<uint8>(Channel);
        check(Index < 32);
        Mask |= (1u << Index);
    }
    return Mask;
}