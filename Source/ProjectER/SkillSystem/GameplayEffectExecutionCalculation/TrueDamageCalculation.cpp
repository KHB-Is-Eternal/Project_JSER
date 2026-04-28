// Fill out your copyright notice in the Description page of Project Settings.

#include "SkillSystem/GameplayEffectExecutionCalculation/TrueDamageCalculation.h"

float UTrueDamageCalculation::CalculateFinalDamage(float RawDamage, float Defense, const FGameplayEffectSpec& Spec) const
{
    // 고정 데미지는 방어력 요소를 무시하고 RawDamage를 그대로 반환합니다.
    return RawDamage;
}
