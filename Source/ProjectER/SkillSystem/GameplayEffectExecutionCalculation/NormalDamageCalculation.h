// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "SkillSystem/GameplayEffectExecutionCalculation/DamageExecutionCalculation.h"
#include "NormalDamageCalculation.generated.h"

/**
 * 일반적인 AOS 방어력 공식을 적용하는 데미지 계산 클래스입니다.
 */
UCLASS()
class PROJECTER_API UNormalDamageCalculation : public UDamageExecutionCalculation
{
	GENERATED_BODY()

protected:
    virtual float CalculateFinalDamage(float RawDamage, float Defense, const FGameplayEffectSpec& Spec) const override;
};
