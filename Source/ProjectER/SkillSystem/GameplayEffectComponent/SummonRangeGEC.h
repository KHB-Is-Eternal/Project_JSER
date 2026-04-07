// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "SkillSystem/GameplayEffectComponent/SummonRangeBaseGEC.h"
#include "SummonRangeGEC.generated.h"

/**
 * 
 */

class ABaseRangeOverlapEffectActor;

struct FGameplayEffectContextHandle;
struct FGameplayCueParameters;

UCLASS()
class PROJECTER_API USummonRangeGEC : public USummonRangeBaseGEC
{
	GENERATED_BODY()
	
public:
	USummonRangeGEC();

protected:
	virtual FTransform CalculateOriginTransform(const FGameplayEffectSpec& GESpec, const AActor* Instigator, const AActor* TargetActor) const override;
	FVector GetAnyLocation(const FGameplayEffectContextHandle& ContextHandle) const;

public:
	UPROPERTY(EditDefaultsOnly, Category = "Summon Settings|Rotation")
	bool bLookAtTargetLocation = false;
};
