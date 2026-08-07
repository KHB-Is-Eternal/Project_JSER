#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Vision_EvaluatorComp.generated.h"

class USphereComponent;
class UVision_VisualComp;
class UTopDown2DShapeComp;
class ULOSObstacleDrawerComponent;

UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class TOPDOWNVISION_API UVision_EvaluatorComp : public UActorComponent
{
    GENERATED_BODY()

public:
    UVision_EvaluatorComp();

protected:
    virtual void BeginPlay() override;

public:
    void PrepareDetectionSphere();

    UFUNCTION(BlueprintCallable)
    void InitializeEvaluator(UVision_VisualComp* DirectParamComp);

    /** Called by VisionPlayerStateComp when local team is assigned/changed.
     *  Late-initializes same-team evaluators that were skipped at BeginPlay
     *  because the team channel was not yet known. */
    void InitializeIfSameTeam();

    UFUNCTION(BlueprintCallable)
    void DirectCacheVisualComp(UVision_VisualComp* DirectParamComp);

    void FindAndCacheVisualComp();

    UFUNCTION(BlueprintCallable)
    void SetEvaluationEnabled(bool bEnabled);

    void SyncDetectionRadius();

    // Overlap one more target object type on top of VisionTargetChannel.
    // For owners whose targets are not Pawns (e.g. a ward detecting other wards).
    void AddVisionTargetChannel(ECollisionChannel InChannel);

    // For owners that need to tune the sensor beyond the helpers above.
    UFUNCTION(BlueprintCallable, Category="Evaluator")
    USphereComponent* GetDetectionSphere() const { return DetectionSphere; }

    UFUNCTION(BlueprintCallable)
    void BP_DrawDebugSphereComp(float DrawTime);

private:
    // --- Overlap callbacks --- //
    UFUNCTION()
    void OnDetectionSphereBeginOverlap(
        UPrimitiveComponent* OverlappedComponent,
        AActor* OtherActor,
        UPrimitiveComponent* OtherComp,
        int32 OtherBodyIndex,
        bool bFromSweep,
        const FHitResult& SweepResult);

    UFUNCTION()
    void OnDetectionSphereEndOverlap(
        UPrimitiveComponent* OverlappedComponent,
        AActor* OtherActor,
        UPrimitiveComponent* OtherComp,
        int32 OtherBodyIndex);

    // --- Evaluation --- //
    void EvaluateTick();
    void EvaluateTarget(AActor* Target, UVision_VisualComp* TargetVisual);
    bool EvaluateWallObstacle(AActor* Target, UTopDown2DShapeComp* ShapeComp);

    // --- Visibility reporting --- //
    void ReportVisibility(AActor* Target, bool bVisible);
    void ReportVisibilityIfChanged(AActor* Target, bool bVisible);
    void CommitHide(AActor* Target);

    // RPC — crosses network boundary, subsystem handles logic on server side
    UFUNCTION(Server, Reliable)
    void Server_ReportVisibility(AActor* Target, EVisionChannel Channel, bool bVisible);
    void Server_ReportVisibility_Implementation(
        AActor* Target, EVisionChannel Channel, bool bVisible);

    // --- Helpers --- //
    UVision_VisualComp* GetVisualComp(AActor* Target) const;
    bool ShouldRunServerLogic() const;
    bool IsSameTeamAsLocalPlayer() const;
    void StartEvaluationTimer();
    void StopEvaluationTimer();

protected:
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Evaluator")
    USphereComponent* DetectionSphere = nullptr;

    // Object type of the targets the DetectionSphere overlaps.
    // GameTraceChannel1 was unusable here — it is declared as a trace-only channel,
    // so no primitive ever carries it as an object type.
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Evaluator")
    TEnumAsByte<ECollisionChannel> VisionTargetChannel = ECC_Pawn;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Evaluator")
    float DetectionRadius = 1200.f;

    // LOSChannel — only vision obstacles respond to it. Visibility would also be
    // blocked by the observer's own mesh and by the target itself.
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Evaluator")
    TEnumAsByte<ECollisionChannel> WallTraceChannel = ECC_GameTraceChannel1;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Evaluator",
        meta=(ClampMin="0.01"))
    float EvaluationInterval = 0.1f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Evaluator")
    FName TargetTag = TEXT("VisionTarget");

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Evaluator|Debug")
    bool bDrawDebug = false;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Evaluator",
        meta=(AllowPrivateAccess="true", ClampMin="0.0"))
    float HideHysteresisDelay = 0.3f;

private:
    UPROPERTY(Transient)
    TSet<AActor*> OverlappingTargets;

    FTimerHandle EvaluationTimerHandle;

    UPROPERTY(Transient)
    UVision_VisualComp* CachedVisualComp = nullptr;

    UPROPERTY(Transient)
    TMap<AActor*, bool> LastReportedVisibility;

    TMap<AActor*, FTimerHandle> PendingHideTimers;

    bool bIsInitialized = false;
};