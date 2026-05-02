// Fill out your copyright notice in the Description page of Project Settings.

#include "SkillSystem/GameplayEffectExecutionCalculation/NormalDamageCalculation.h"

float UNormalDamageCalculation::CalculateFinalDamage(float RawDamage, float Defense, const FGameplayEffectSpec& Spec) const
{


    // AOS 공식: 데미지 * (100 / (100 + 방어력))
    float DamageMultiplier = 100.0f / (100.0f + Defense);
    float CalculatedResult = RawDamage * DamageMultiplier;



    return CalculatedResult;
}
