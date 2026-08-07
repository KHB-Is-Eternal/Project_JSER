// Fill out your copyright notice in the Description page of Project Settings.

#include "TopDownVision/Public/LineOfSight/VisionComps/Vision_VisualComp.h"

#include "LineOfSight/LOSVisual/LOSStampDrawerComp.h"
#include "LineOfSight/WorldObstacle/LOSObstacleDrawerComponent.h"
#include "LineOfSight/WorldObstacle/LocalTextureSampler.h"
#include "LineOfSight/LOSVisual/VisibilityMeshComp.h"
#include "LineOfSight/ObjectTracing/TopDown2DShapeComp.h"

#include "LineOfSight/Management/Subsystem/LOSVisionSubsystem.h"
#include "LineOfSight/Management/Subsystem/LOSRequirementPoolSubsystem.h"
#include "LineOfSight/VisionComps/Vision_EvaluatorComp.h"
#include "ObstacleOcclusion/Manager/OcclusionSubsystem.h"

#include "GameFramework/GameStateBase.h"
#include "LineOfSight/Management/VisionGameStateComp.h"
#include "LineOfSight/Management/VisionPlayerStateComp.h"
#include "LineOfSight/MainVisionRTManager.h"
#include "TopDownVisionDebug.h"


UVision_VisualComp::UVision_VisualComp()
{
    PrimaryComponentTick.bCanEverTick = false;

    ObstacleDrawer = CreateDefaultSubobject<ULOSObstacleDrawerComponent>(TEXT("ObstacleDrawer"));
    StampDrawer    = CreateDefaultSubobject<ULOSStampDrawerComp>(TEXT("StampDrawer"));
    VisibilityMesh = CreateDefaultSubobject<UVisibilityMeshComp>(TEXT("VisibilityMesh"));
    ShapeComp      = CreateDefaultSubobject<UTopDown2DShapeComp>(TEXT("2DShapeComp"));

    bIsVisionProvider = true;
}

void UVision_VisualComp::BeginPlay()
{
    Super::BeginPlay();

    if (ShapeComp)
    {
        if (USceneComponent* Root = GetOwner()->GetRootComponent())
            ShapeComp->AttachToComponent(Root, FAttachmentTransformRules::SnapToTargetNotIncludingScale);
        else
            UE_LOG(LOSVision, Warning, TEXT("[%s] BeginPlay >> No root for ShapeComp"), *GetOwner()->GetName());
    }

    if (!ShouldRunClientLogic())
        return;
}

void UVision_VisualComp::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    Super::EndPlay(EndPlayReason);

    if (GetWorld())
        GetWorld()->GetTimerManager().ClearTimer(FadeTimerHandle);



    if (ULOSVisionSubsystem* Subsystem = GetWorld()->GetSubsystem<ULOSVisionSubsystem>())
        Subsystem->UnregisterProvider(this, VisionChannel);

    // 풀에서 받은 가시성 MID 세트 반납 (미보유 시 무시)
    if (ULOSRequirementPoolSubsystem* PoolSub = GetWorld()->GetSubsystem<ULOSRequirementPoolSubsystem>())
        PoolSub->ReleaseVisibilityMIDsFor(this);

    if (OcclusionTargetIndex != INDEX_NONE)
    {
        if (UOcclusionSubsystem* OccSub = GetWorld()->GetSubsystem<UOcclusionSubsystem>())
            if (UPrimitiveComponent* Root = Cast<UPrimitiveComponent>(GetOwner()->GetRootComponent()))
                OccSub->UnregisterTarget(Root);
        OcclusionTargetIndex = INDEX_NONE;
    }
}

void UVision_VisualComp::OnRegister() { Super::OnRegister(); }

// -------------------------------------------------------------------------- //
//  Initialize
// -------------------------------------------------------------------------- //

void UVision_VisualComp::Initialize()
{
    if (!ShouldRunClientLogic())
        return;

    // Grid Vision always skips allocating local rendering resources (Obstacle / Stamp)
    // and only initializes visibility fading mesh.
    if (VisibilityMesh)
    {
        // MeshKey가 설정된 액터는 풀에서 미리 만든 MID 세트를 우선 사용하고,
        // 키 미등록/풀 고갈 시 기존처럼 메시의 머티리얼로 MID를 생성(소유 모드)한다.
        bool bPooledMIDs = false;
        if (ULOSRequirementPoolSubsystem* PoolSub = GetWorld()->GetSubsystem<ULOSRequirementPoolSubsystem>())
            bPooledMIDs = PoolSub->AcquireVisibilityMIDsFor(this);

        if (!bPooledMIDs)
            VisibilityMesh->Initialize();
    }

    UE_LOG(LOSVision, Log,
        TEXT("[%s] Initialize >> Lightweight Grid Vision mode (IsVisionProvider: %s)"),
        *GetOwner()->GetName(),
        bIsVisionProvider ? TEXT("True") : TEXT("False"));

    CachedEvaluatorComp = GetOwner()->FindComponentByClass<UVision_EvaluatorComp>();

    if (ULOSVisionSubsystem* Subsystem = GetWorld()->GetSubsystem<ULOSVisionSubsystem>())
        Subsystem->RegisterProvider(this, VisionChannel);

    if (UOcclusionSubsystem* OccSub = GetWorld()->GetSubsystem<UOcclusionSubsystem>())
        if (UPrimitiveComponent* Root = Cast<UPrimitiveComponent>(GetOwner()->GetRootComponent()))
            OcclusionTargetIndex = OccSub->RegisterTarget(Root, nullptr, GetVisibleRange());
}

void UVision_VisualComp::SetIndicatorRange(float NewIndicatorRange) { IndicatorRange = NewIndicatorRange; }

// -------------------------------------------------------------------------- //


// -------------------------------------------------------------------------- //
//  Update
// -------------------------------------------------------------------------- //

void UVision_VisualComp::UpdateVision()
{
    // Handled asynchronously by Grid Vision System
}

void UVision_VisualComp::ToggleLOSStampUpdate(bool bIsOn)
{
    if (StampDrawer) StampDrawer->ToggleLOSStampUpdate(bIsOn);
}

bool UVision_VisualComp::IsUpdating() const
{
    return false;
}

// -------------------------------------------------------------------------- //
//  Visibility fade
// -------------------------------------------------------------------------- //

void UVision_VisualComp::SetVisible(bool bVisible, bool bInstant)
{
    const float NewTargetAlpha = bVisible ? 1.0f : 0.0f;

    // Only broadcast when the target direction genuinely changes.
    // ReevaluateTargetVisibility may call SetVisible repeatedly with the
    // same value (e.g. every time VisibleActors ticks); firing delegates
    // on every redundant call breaks Blueprint listeners and causes spurious
    // pool acquire/release cycles.
    if (!FMath::IsNearlyEqual(NewTargetAlpha, TargetVisibilityAlpha))
    {
        TargetVisibilityAlpha = NewTargetAlpha;

        if (bVisible) OnTargetRevealed.Broadcast();
        else          OnTargetHidden.Broadcast();
    }

    if (bInstant)
    {
        VisibilityAlpha = TargetVisibilityAlpha;
        GetWorld()->GetTimerManager().ClearTimer(FadeTimerHandle);
        if (VisibilityMesh) VisibilityMesh->UpdateVisibility(bVisible);
        return;
    }

    if (!GetWorld()->GetTimerManager().IsTimerActive(FadeTimerHandle))
    {
        GetWorld()->GetTimerManager().SetTimer(
            FadeTimerHandle,
            FTimerDelegate::CreateUObject(this, &UVision_VisualComp::UpdateVisibilityFade, FadeTickInterval),
            FadeTickInterval, true);
    }
}

void UVision_VisualComp::UpdateVisibilityFade(float DeltaTime)
{
    if (!IsValid(this) || !IsValid(GetOwner()) || !GetWorld())
    {
        if (GetWorld()) GetWorld()->GetTimerManager().ClearTimer(FadeTimerHandle);
        return;
    }

    VisibilityAlpha = FMath::FInterpTo(VisibilityAlpha, TargetVisibilityAlpha, DeltaTime, FadeSpeed);

    if (VisibilityMesh) VisibilityMesh->UpdateVisibility(VisibilityAlpha);

    if (OcclusionTargetIndex != INDEX_NONE)
        if (UOcclusionSubsystem* OccSub = GetWorld()->GetSubsystem<UOcclusionSubsystem>())
            OccSub->UpdateTargetByIndex(OcclusionTargetIndex, VisibilityAlpha, -1.f);

    if (FMath::IsNearlyEqual(VisibilityAlpha, TargetVisibilityAlpha, 0.005f))
    {
        VisibilityAlpha = TargetVisibilityAlpha;

        if (OcclusionTargetIndex != INDEX_NONE)
            if (UOcclusionSubsystem* OccSub = GetWorld()->GetSubsystem<UOcclusionSubsystem>())
                OccSub->UpdateTargetByIndex(OcclusionTargetIndex, VisibilityAlpha, -1.f);

        GetWorld()->GetTimerManager().ClearTimer(FadeTimerHandle);

        if (VisibilityAlpha > 0) OnTargetRevealComplete.Broadcast();
        else                     OnTargetHideComplete.Broadcast();

        UE_LOG(LOSVision, Verbose, TEXT("[%s] UpdateVisibilityFade >> Complete: %.2f"),
            *GetOwner()->GetName(), VisibilityAlpha);
    }
}

// -------------------------------------------------------------------------- //
//  Range / channel
// -------------------------------------------------------------------------- //

void UVision_VisualComp::SetVisionRange(float NewRange)
{
    VisionRange = FMath::Clamp(NewRange, 0.f, MaxVisionRange);
    if (StampDrawer) StampDrawer->OnVisionRangeChanged(VisionRange, MaxVisionRange);
    if (!CachedEvaluatorComp)
        CachedEvaluatorComp = GetOwner()->FindComponentByClass<UVision_EvaluatorComp>();
    if (CachedEvaluatorComp) CachedEvaluatorComp->SyncDetectionRadius();
}

UMaterialInstanceDynamic* UVision_VisualComp::GetStampMID() const
{
    return StampDrawer ? StampDrawer->GetLOSMaterialMID() : nullptr;
}

void UVision_VisualComp::SetVisionChannel(EVisionChannel InVC)
{
    FString TempDebug = TopDownVisionDebug::GetClientDebugName(GetOwner());
    VisionChannel = InVC;

    RefreshOcclusionAndEvaluatorRadius();
}

void UVision_VisualComp::RefreshOcclusionAndEvaluatorRadius()
{
    if (OcclusionTargetIndex != INDEX_NONE)
    {
        if (UWorld* World = GetWorld())
        {
            if (UOcclusionSubsystem* OccSub = World->GetSubsystem<UOcclusionSubsystem>())
            {
                OccSub->UpdateTargetByIndex(OcclusionTargetIndex, VisibilityAlpha, GetVisibleRange());
            }
        }
    }

    if (CachedEvaluatorComp)
    {
        CachedEvaluatorComp->SyncDetectionRadius();
    }
}

void UVision_VisualComp::UpdateVisionRange(float NewRange)
{
    VisionRange = FMath::Clamp(NewRange, 0.f, MaxVisionRange);
}

bool UVision_VisualComp::IsSharedVisionChannel() const
{
    return VisionChannel == GetLocalPlayerVisionChannel();
}

EVisionChannel UVision_VisualComp::GetLocalPlayerVisionChannel() const
{
    if (UWorld* World = GetWorld())
    {
        if (ULOSVisionSubsystem* Subsystem = World->GetSubsystem<ULOSVisionSubsystem>())
        {
            if (UVisionPlayerStateComp* LocalVPS = Subsystem->GetLocalVisionPS(World))
            {
                return LocalVPS->GetTeamChannel();
            }
        }
    }
    return EVisionChannel::None;
}

float UVision_VisualComp::GetVisibleRange() const
{
    if (!IsSharedVisionChannel() && IndicatorRange > 0.f)
    {
        return IndicatorRange;
    }
    return VisionRange;
}

float UVision_VisualComp::GetMaxVisibleRange() const
{
    if (!IsSharedVisionChannel() && IndicatorRange > 0.f)
    {
        return IndicatorRange;
    }
    return MaxVisionRange;
}

bool UVision_VisualComp::ShouldRunClientLogic() const
{
    return GetNetMode() != NM_DedicatedServer;
}

