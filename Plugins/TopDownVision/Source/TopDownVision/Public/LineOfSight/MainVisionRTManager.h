#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Engine/CanvasRenderTarget2D.h"
#include "Materials/MaterialInterface.h"
#include "Materials/MaterialInstance.h"
#include "LineOfSight/VisionData.h"
#include "LineOfSight/Grid/GridVisionAsyncTask.h"
#include "MainVisionRTManager.generated.h"

class UVision_VisualComp;
class ULOSRequirementPoolSubsystem;
class UGridVisionMap;

/*
 * Composites all in-range LOS stamps onto CameraLocalRT each frame.
 *
 * Pool integration:
 *   Drives slot acquire/release on ULOSRequirementPoolSubsystem based on
 *   RectOverlapsWorld bounds check each frame.
 *   Calls are routed through UVision_VisualComp::OnPoolSlotAcquired /
 *   OnPoolSlotReleased — the manager never touches sub-components directly.
 */
UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class TOPDOWNVISION_API UMainVisionRTManager : public UActorComponent
{
	GENERATED_BODY()

public:
	UMainVisionRTManager();

	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

protected:
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	virtual void BeginPlay() override;

public:

	UFUNCTION(BlueprintCallable, Category="LineOfSight")
	void InitializeMainVisionRTComp();

	UFUNCTION(BlueprintCallable, Category="LineOfSight")
	void UpdateCameraLOS();

	/*UFUNCTION(BlueprintCallable, Category="LineOfSight")
	void UpdateVisionRTs();*/

	UCanvasRenderTarget2D* GetCameraLOSTexture() const { return CameraLocalRT; }

	UFUNCTION(BlueprintCallable, Category="LineOfSight")
	UMaterialInstanceDynamic* GetLayeredMID() const { return LayeredLOSInterfaceMID; }

	UFUNCTION(BlueprintCallable, Category="LineOfSight")
	bool IsGridVisionEnabled() const { return bUseGridVision; }

private:

	bool GetVisibleProviders(TArray<UVision_VisualComp*>& OutProviders) const;

	bool ShouldRunClientLogic() const;

	/** Lazy-cached pool subsystem — resolved once on first UpdateCameraLOS call. */
	ULOSRequirementPoolSubsystem* GetPoolSubsystem() const;

protected:

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Vision")
	bool bUseCPU = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Vision")
	bool bUseGridVision = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Vision|Grid", meta=(EditCondition="bUseGridVision"))
	int32 GridResolution = 256;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Vision")
	bool bDrawTextureRange = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Vision|Grid", meta=(EditCondition="bUseGridVision"))
	float MapWorldExtent = 20000.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Vision|Grid", meta=(EditCondition="bUseGridVision"))
	float UpdateInterval = 0.033f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Vision")
	UCanvasRenderTarget2D* CameraLocalRT = nullptr;



	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Vision")
	UMaterialParameterCollection* PostProcessMPC = nullptr;

	UPROPERTY()
	UMaterialParameterCollectionInstance* MPCInstance = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="MPC")
	FName MPCLocationParam = TEXT("VisionCenterLocation");

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="MPC")
	FName MPCVisibleRangeParam = TEXT("VisibleRange");

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="MPC")
	FName MPCNearSightRangeParam = TEXT("NearSightRange");

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Vision")
	UMaterialInterface* LayeredLOSInterfaceMaterial = nullptr;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Vision")
	UMaterialInstanceDynamic* LayeredLOSInterfaceMID = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Vision|Grid", meta=(EditCondition="bUseGridVision"))
	float TemporalBlendSpeed = 5.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Vision|Grid", meta=(EditCondition="bUseGridVision"))
	float BlurSharpness = 3.0f;

	float TimeSinceLastUpdate = 0.f;

	// UpdateCameraLOS의 같은 프레임 중복 실행 방지 (자체 틱 + DrawUpdates 이중 호출 대비, 006 합-5)
	uint64 LastUpdateFrame = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Vision")
	FName LayeredLOSTextureParam = TEXT("RenderTarget");

	UPROPERTY(EditAnywhere, Category="Vision")
	uint32 CameraViewChannelMask = 0xFFFFFFFF;

	static uint32 MakeChannelBitMask(const TArray<EVisionChannel>& ChannelEnums);

private:
	UPROPERTY(Transient)
	TObjectPtr<UGridVisionMap> GridVisionMap = nullptr;

	UPROPERTY(Transient)
	TArray<TObjectPtr<UVision_VisualComp>> CachedValidProviders;

	UPROPERTY(Transient)
	mutable TObjectPtr<ULOSRequirementPoolSubsystem> CachedPoolSubsystem = nullptr;

	// The background task processing grid vision
	FAsyncTask<FGridVisionAsyncTask>* PendingGridTask = nullptr;
};
