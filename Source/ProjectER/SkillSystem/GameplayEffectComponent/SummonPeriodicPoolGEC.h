// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "SkillSystem/GameplayEffectComponent/SummonRangeGEC.h"
#include "SummonPeriodicPoolGEC.generated.h"

UENUM(BlueprintType)
enum class ESummonOriginType : uint8
{
    Context,
    InstigatorBone
};



UCLASS(DontCollapseCategories)
class PROJECTER_API USummonPeriodicPoolGEC : public USummonRangeGEC
{
    GENERATED_BODY()

public:
    USummonPeriodicPoolGEC();

    virtual FSkillTooltipData GetTooltipDescription(int32 Level, TSubclassOf<class USkillBase> AbilityClass) const override;

protected:
    virtual FTransform CalculateOriginTransform(const FGameplayEffectSpec& GESpec, const AActor* Instigator, const AActor* TargetActor) const override;
    virtual void InitializeRangeActor(ABaseRangeOverlapEffectActor* RangeActor, AActor* Instigator, const FGameplayEffectContextHandle& Context, const FGameplayCueParameters& HitTargetVfxCueParameters, const FGameplayCueParameters& HitTargetSoundCueParameters, const FGameplayEffectSpec& ParentSpec) const override;

public:
    UPROPERTY(EditDefaultsOnly, Category = "Summon|Periodic")
    float Period = 1.0f;

    UPROPERTY(EditDefaultsOnly, Category = "Summon|Periodic")
    bool bApplyImmediately = true;

    UPROPERTY(EditDefaultsOnly, Category = "Summon|Periodic")
    ESummonOriginType OriginType = ESummonOriginType::Context;

    UPROPERTY(EditDefaultsOnly, Category = "Summon|Periodic", meta = (EditCondition = "OriginType == ESummonOriginType::InstigatorBone"))
    FName SummonBoneName;

    UPROPERTY(EditDefaultsOnly, Instanced, Category = "Summon|VFX")
    TObjectPtr<USkillNiagaraSpawnConfig> PeriodicVfx;

    UPROPERTY(EditDefaultsOnly, Instanced, Category = "Summon|SFX")
    TObjectPtr<USkillSoundSpawnConfig> PeriodicSound;
};
