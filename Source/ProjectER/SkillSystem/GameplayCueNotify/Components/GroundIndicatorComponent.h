#pragma once

#include "CoreMinimal.h"
#include "GameFramework/SpringArmComponent.h"
#include "GroundIndicatorComponent.generated.h"

class UStaticMeshComponent;

/**
 * 지면을 자동으로 추적하여 바닥에 밀착되는 스킬 인디케이터/장판 전용 메쉬 컴포넌트입니다.
 * 내부에 스프링암(USpringArmComponent) 기능을 내포하여, 부모 본의 Pitch/Roll 상속을 차단하고 수평을 유지합니다.
 */
UCLASS(meta = (BlueprintSpawnableComponent))
class PROJECTER_API UGroundIndicatorComponent : public USpringArmComponent
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

	/** 내부 메쉬 컴포넌트의 머티리얼을 설정합니다. */
	void SetIndicatorMaterial(int32 ElementIndex, UMaterialInterface* Material);

	/** 내부 메쉬 컴포넌트의 머티리얼을 반환합니다. */
	UMaterialInterface* GetIndicatorMaterial(int32 ElementIndex) const;

	/** 내부 메쉬 컴포넌트의 월드 스케일을 설정합니다. */
	void SetIndicatorScale(const FVector& NewScale);

protected:
	/** 바닥으로 레이를 쏴서 현재 X,Y 위치 기준 바닥 높이(Z)로 강제 이동시킵니다. */
	void UpdateGroundPosition();

	/** 자식 메쉬의 생성 여부를 검사하고, 없을 경우 즉시 생성하여 설정 누락을 방지합니다. (지연 생성) */
	void EnsureIndicatorMeshCompExists();

protected:
	/** 내부 렌더링용 스태틱 메쉬 컴포넌트 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Indicator")
	TObjectPtr<UStaticMeshComponent> IndicatorMeshComp;

	/** 본 어태치먼트 여부를 외부에서 전달받아, 틱에서 트레이스를 쏠지 결정합니다. */
	bool bIsTrackingDynamicGround = false;
	/** 바닥에서 얼마나 띄워서 렌더링할 것인지 결정합니다. (Z-Fighting 깜빡임 방지용) */
	UPROPERTY(EditAnywhere, Category = "Indicator")
	float ZOffsetFromFloor = 2.0f;
};
