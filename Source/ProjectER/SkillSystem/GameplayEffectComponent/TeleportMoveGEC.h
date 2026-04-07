// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "SkillSystem/GameplayEffectComponent/MoveBaseGEC.h"
#include "TeleportMoveGEC.generated.h"



UCLASS()
class PROJECTER_API UTeleportMoveGEC : public UMoveBaseGEC
{
	GENERATED_BODY()

	UTeleportMoveGEC();

	virtual float CalculateMoveDuration(const FGameplayEffectSpec& GESpec, const AActor* Instigator, const FVector& Direction) const override;

protected:
	virtual void Execute(AActor* Instigator, const FVector& Direction, const FGameplayEffectSpec& GESpec) const override;

private:
	FVector CalculateDestination(const FGameplayEffectSpec& GESpec, AActor* Instigator, const FVector& Direction) const;
	void UpdateLevelTracker(AActor* Actor) const;

public:
	UPROPERTY(EditDefaultsOnly, Category = "Teleport")
	bool bSweep = true;

	UPROPERTY(EditDefaultsOnly, Category = "Teleport")
	float NavProjectionRadius = 300.0f;

	UPROPERTY(EditDefaultsOnly, Category = "Teleport")
	float TeleportSafetyOffset = 2.0f;
};
