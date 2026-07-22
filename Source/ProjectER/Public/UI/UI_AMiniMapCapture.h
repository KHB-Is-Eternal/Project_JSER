
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"

#include "UI_AMiniMapCapture.generated.h"

UCLASS()
class PROJECTER_API AUI_AMiniMapCapture : public AActor
{
	GENERATED_BODY()
	
public:
    AUI_AMiniMapCapture();
    
    UFUNCTION()
    void UpdateMiniMap();

    // 미니맵 좌표 변환용 맵 기준 정보 (UI_MinimapProjection과 함께 사용)
    FVector GetMapCenter() const;
    float GetMapOrthoWidth() const;

protected:
    virtual void BeginPlay() override;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "UI|Minimap")
    class USceneComponent* RootScene; // 추가

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "UI|Minimap")
    class USceneCaptureComponent2D* CaptureComponent;

    UPROPERTY(EditAnywhere)
    class UTextureRenderTarget2D* MapRenderTarget;

private:
    // 게임 상태 변화 시 맵 재캡처 (페이즈 변경 — 파라미터 시그니처 맞춤용 래퍼)
    UFUNCTION()
    void OnPhaseChanged_Recapture(int32 NewPhase);
};
