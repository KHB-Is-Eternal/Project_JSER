// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Abilities/GameplayAbilityTargetActor.h"
#include "MouseLocationTargetActor.generated.h"

UCLASS()
class PROJECTER_API AMouseLocationTargetActor : public AGameplayAbilityTargetActor
{
	GENERATED_BODY()
	
public:
	AMouseLocationTargetActor();

	virtual void StartTargeting(UGameplayAbility* Ability) override;
	virtual void ConfirmTargetingAndContinue() override;
	virtual void Tick(float DeltaSeconds) override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	bool TryConfirmMouseLocation();
	bool SubmitExternalLocation(const FVector& InLocation);

	/** 최대 물리 사거리 주입 */
	void Setup(float InMaxRange);

protected:
	float MaxRange = 0.f;
};
