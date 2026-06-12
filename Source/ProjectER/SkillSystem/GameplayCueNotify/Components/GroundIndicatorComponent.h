#pragma once

#include "CoreMinimal.h"
#include "Components/StaticMeshComponent.h"
#include "GroundIndicatorComponent.generated.h"

/**
 * 지면을 자동으로 추적하여 바닥에 밀착되는 스킬 인디케이터/장판 전용 메쉬 컴포넌트입니다.
 * 부모의 X,Y 좌표와 Yaw 회전만 상속받고, Z축 높이는 실시간으로 지면(ECC_GameTraceChannel9)을 찾아 강제로 고정시킵니다.
 */
UCLASS(meta = (BlueprintSpawnableComponent))
class PROJECTER_API UGroundIndicatorComponent : public UStaticMeshComponent
{
	GENERATED_BODY()

public:
	UGroundIndicatorComponent();

protected:
	virtual void BeginPlay() override;

public:
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	/** 특정 본(소켓)에 부착되어 매 프레임 상하 위치 보정(트레이스)이 필요한지 여부를 설정합니다. */
	void SetTrackingDynamicGround(bool bInTracking) { bIsTrackingDynamicGround = bInTracking; }

protected:
	/** 바닥으로 레이를 쏴서 현재 X,Y 위치 기준 바닥 높이(Z)로 강제 이동시킵니다. */
	void UpdateGroundPosition();

protected:
	/** 본 어태치먼트 여부를 외부에서 전달받아, 틱에서 트레이스를 쏠지 결정합니다. */
	bool bIsTrackingDynamicGround = false;
	/** 바닥에서 얼마나 띄워서 렌더링할 것인지 결정합니다. (Z-Fighting 깜빡임 방지용) */
	UPROPERTY(EditAnywhere, Category = "Indicator")
	float ZOffsetFromFloor = 2.0f;
};
