// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "SkillSystem/GameplayEffectComponent/SummonRangeAtBone.h"
#include "AouraGEC.generated.h"



/**
 * 캐릭터 본에 부착되어 지속시간 동안 주기적으로 효과를 적용하는 GEC
 */
UCLASS(DontCollapseCategories)
class PROJECTER_API UAouraGEC : public USummonRangeAtBone
{
	GENERATED_BODY()

	UAouraGEC();

protected:
	virtual void InitializeRangeActor(ABaseRangeOverlapEffectActor* RangeActor, AActor* Instigator, const FGameplayEffectContextHandle& Context, const FGameplayCueParameters& HitTargetVfxCueParameters, const FGameplayCueParameters& HitTargetSoundCueParameters, const FGameplayEffectSpec& ParentSpec) const override;

public:
	virtual FSkillTooltipData GetTooltipDescription(int32 Level, TSubclassOf<class USkillBase> AbilityClass) const override;

	UPROPERTY(EditDefaultsOnly, Category = "Summon|Periodic")
	float Period = 1.0f;

	UPROPERTY(EditDefaultsOnly, Category = "Summon|Periodic")
	bool bApplyImmediately = true;
};
