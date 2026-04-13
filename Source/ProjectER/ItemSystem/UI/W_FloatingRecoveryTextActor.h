#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "W_FloatingRecoveryTextActor.generated.h"

class USceneComponent;
class UWidgetComponent;

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

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	TObjectPtr<USceneComponent> SceneRoot;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	TObjectPtr<UWidgetComponent> WidgetComp;

	UPROPERTY(EditDefaultsOnly, Category = "RecoveryText")
	float LifeSeconds = 1.0f;

	UPROPERTY(EditDefaultsOnly, Category = "RecoveryText")
	float FloatUpSpeed = 35.0f;

private:
	float ElapsedTime = 0.0f;
};