#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "W_FloatingRecoveryTextActor.generated.h"

class USceneComponent;
class UWidgetComponent;
class UW_FloatingRecoveryText;
class APlayerController;

UCLASS()
class PROJECTER_API AW_FloatingRecoveryTextActor : public AActor
{
	GENERATED_BODY()

public:
	AW_FloatingRecoveryTextActor();

	UFUNCTION(BlueprintCallable, Category = "RecoveryText")
	void InitRecoveryText(int32 Amount, bool bIsMana);

protected:
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	TObjectPtr<USceneComponent> SceneRoot;

	// BP에서 WidgetClass를 지정해두는 용도로만 유지
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	TObjectPtr<UWidgetComponent> WidgetComp;

	UPROPERTY(EditDefaultsOnly, Category = "RecoveryText")
	float LifeSeconds = 1.0f;

	UPROPERTY(EditDefaultsOnly, Category = "RecoveryText")
	float FloatUpSpeed = 35.0f;

	UPROPERTY(EditDefaultsOnly, Category = "RecoveryText")
	float ScreenOffsetY = -30.0f;

private:
	void UpdateWidgetScreenPosition();

	float ElapsedTime = 0.0f;

	// 실제 화면에 띄울 viewport widget
	UPROPERTY(Transient)
	TObjectPtr<UW_FloatingRecoveryText> FloatingWidget;

	UPROPERTY(Transient)
	TObjectPtr<APlayerController> CachedLocalPC;
};