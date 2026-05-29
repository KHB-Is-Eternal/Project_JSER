// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "SkillSystem/GameplayEffectComponent/BaseGEC.h"
#include "GameplayTagContainer.h"
#include "ScalableFloat.h"
#include "CooldownRefundGEC.generated.h"

/**
 * 특정 태그를 가진 쿨타임 이펙트의 남은 지속시간을 감소시킵니다.
 */
UCLASS(DisplayName = "Cooldown Refund Component", DontCollapseCategories)
class PROJECTER_API UCooldownRefundGEC : public UBaseGEC
{
	GENERATED_BODY()

public:
	UCooldownRefundGEC();

	virtual FSkillTooltipData GetTooltipDescription(int32 Level, TSubclassOf<class USkillBase> AbilityClass) const override;

protected:
	virtual void OnGameplayEffectExecuted(FActiveGameplayEffectsContainer& ActiveGEContainer, FGameplayEffectSpec& GESpec, FPredictionKey& PredictionKey) const override;

public:
	/** 
	 * 감소시킬 쿨타임 태그들. 
	 * 비어있으면, 이 이펙트를 유발한 원본 어빌리티의 쿨타임 태그를 찾아 동적으로 감소시킵니다. 
	 */
	UPROPERTY(EditDefaultsOnly, Category = "Cooldown", meta = (Categories = "Cooldown.Skill"))
	FGameplayTagContainer TargetCooldownTags;

	/** 감소시킬 시간(초) */
	UPROPERTY(EditDefaultsOnly, Category = "Cooldown")
	FScalableFloat RefundAmount;
};
