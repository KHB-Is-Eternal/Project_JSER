// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "SkillSystem/GameplayEffectExecutionCalculation/DamageExecutionCalculation.h"
#include "TrueDamageCalculation.generated.h"

/**
 * 방어력을 무시하고 원본 데미지를 그대로 입히는 고정 데미지 계산 클래스입니다.
 */
UCLASS()
class PROJECTER_API UTrueDamageCalculation : public UDamageExecutionCalculation
{
	GENERATED_BODY()

protected:
    virtual float CalculateFinalDamage(float RawDamage, float Defense, const FGameplayEffectSpec& Spec) const override;
};
