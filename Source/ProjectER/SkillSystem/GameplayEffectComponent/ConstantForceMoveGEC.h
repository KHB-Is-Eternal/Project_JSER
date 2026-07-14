// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "SkillSystem/GameplayEffectComponent/MoveBaseGEC.h"
#include "ConstantForceMoveGEC.generated.h"

UCLASS(DontCollapseCategories)
class PROJECTER_API UConstantForceMoveGEC : public UMoveBaseGEC
{
	GENERATED_BODY()

public:
	UConstantForceMoveGEC();

	virtual float CalculateMoveDuration(const FGameplayEffectSpec& GESpec, const AActor* Instigator, const FVector& Direction) const override;
	virtual FSkillTooltipData GetTooltipDescription(int32 Level, TSubclassOf<class USkillBase> AbilityClass) const override;

protected:
	virtual void Execute(AActor* Instigator, const FVector& Direction, const FGameplayEffectSpec& GESpec, FPredictionKey PredictionKey) const override;

public:
	UPROPERTY(EditDefaultsOnly, Category = "Move|ConstantForce")
	float MoveSpeed = 1500.0f;
};
