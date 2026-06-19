#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "TopDownVision/Public/ObstacleOcclusion/PhysicalOcclusion/FrustumToProjectionMatcherHelper.h"
#include "MainOcclusionPainter.generated.h"

class UTextureRenderTarget2D;
class APlayerController;

TOPDOWNVISION_API DECLARE_LOG_CATEGORY_EXTERN(OcclusionPainter, Log, All);

UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class TOPDOWNVISION_API UMainOcclusionPainter : public UActorComponent
{
    GENERATED_BODY()

public:

    UMainOcclusionPainter();

    virtual void BeginPlay() override;
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

    // ── Update — called externally by TopDownCameraComp tick ─────────────

    UFUNCTION(BlueprintCallable, Category="OcclusionPainter")
    void InitializeOcclusionComponent(APlayerController* InPC);

    UFUNCTION(BlueprintCallable, Category="OcclusionPainter")
    void UpdateOcclusionRT();// 

    // ── Camera source ─────────────────────────────────────────────────────

    UFUNCTION(BlueprintCallable, Category="OcclusionPainter")
    void SetPlayerController(APlayerController* InPC);

    // ── Config ────────────────────────────────────────────────────────────

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="OcclusionPainter")
    TObjectPtr<UTextureRenderTarget2D> OcclusionRT;

    // Default brush material — used when target has no per-target material set
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="OcclusionPainter")
    TObjectPtr<UMaterialInterface> DefaultBrushMaterial;

    // Scalar — scales brush intensity per target (0-1)
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="OcclusionPainter")
    FName RevealAlphaParam = TEXT("RevealAlpha");

    // ── Batching & Culling Options ────────────────────────────────────────

    // If true, utilizes a single SharedBrushMID to batch render all targets.
    // NOTE: Requires material asset to use Vertex Color Alpha for RevealAlpha
    // and Local UVs instead of TilePos/TileSize parameters.
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="OcclusionPainter|Optimization")
    bool bUseBatchedRenderer = true;

    // The maximum distance at which a target will be drawn to the occlusion RT.
    // Targets beyond this distance from the camera will be culled to save performance.
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="OcclusionPainter|Optimization")
    float MaxOcclusionDistance = 6000.f;

private:

    // Single shared Material Instance Dynamic used for batching all targets
    UPROPERTY(Transient)
    TObjectPtr<UMaterialInstanceDynamic> SharedBrushMID;

    void DrawProviderArea();
    bool RefreshFrustumParams();

    bool ShouldRunClientLogic();

    UPROPERTY(Transient)
    TObjectPtr<APlayerController> PlayerController;

    FCameraFrustumParams FrustumParams;

    bool bReady = false;
};