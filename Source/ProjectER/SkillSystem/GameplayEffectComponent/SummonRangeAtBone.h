// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "SkillSystem/GameplayEffectComponent/SummonRangeBaseGEC.h"
#include "SummonRangeAtBone.generated.h"

/**
 * 
 */
class ABaseRangeOverlapEffectActor;

struct FGameplayEffectContextHandle;
struct FGameplayCueParameters;



UCLASS()
class PROJECTER_API USummonRangeAtBone : public USummonRangeBaseGEC
{
	GENERATED_BODY()
	
public:
	USummonRangeAtBone();

protected:
	virtual bool ShouldProcessOnInstigator(const AActor* Instigator) const override;
	virtual FTransform CalculateOriginTransform(const FGameplayEffectSpec& GESpec, const AActor* Instigator, const AActor* TargetActor) const override;
	virtual void InitializeRangeActor(ABaseRangeOverlapEffectActor* RangeActor, AActor* Instigator, const FGameplayEffectContextHandle& Context, const FGameplayCueParameters& HitTargetVfxCueParameters, const FGameplayCueParameters& HitTargetSoundCueParameters) const override;

public:
	UPROPERTY(EditDefaultsOnly, Category = "Summon Settings|Base")
	FName BoneName;

	UPROPERTY(EditDefaultsOnly, Category = "Summon Settings|Rotation")
	bool bUseInstigatorRotation = false;

	UPROPERTY(EditDefaultsOnly, Category = "Summon Settings|Attachment")
	bool bAttachToBone = false;
};
