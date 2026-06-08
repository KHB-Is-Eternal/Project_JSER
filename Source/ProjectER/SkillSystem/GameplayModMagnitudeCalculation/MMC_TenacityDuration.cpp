// Fill out your copyright notice in the Description page of Project Settings.

#include "SkillSystem/GameplayModMagnitudeCalculation/MMC_TenacityDuration.h"
#include "CharacterSystem/GAS/AttributeSet/BaseAttributeSet.h"
#include "GameplayEffect.h"

UMMC_TenacityDuration::UMMC_TenacityDuration()
{
    // 타겟의 Tenacity 속성을 캡처하도록 설정
    TenacityDef.AttributeToCapture = UBaseAttributeSet::GetTenacityAttribute();
    TenacityDef.AttributeSource = EGameplayEffectAttributeCaptureSource::Target;
    TenacityDef.bSnapshot = false;

    RelevantAttributesToCapture.Add(TenacityDef);
}

float UMMC_TenacityDuration::CalculateBaseMagnitude_Implementation(const FGameplayEffectSpec& Spec) const
{
    // 1. 강인함 수치 획득 (0.0 ~ 1.0)
    FAggregatorEvaluateParameters EvaluationParameters;
    EvaluationParameters.SourceTags = Spec.CapturedSourceTags.GetAggregatedTags();
    EvaluationParameters.TargetTags = Spec.CapturedTargetTags.GetAggregatedTags();

    float TenacityValue = 0.0f;
    GetCapturedAttributeMagnitude(TenacityDef, Spec, EvaluationParameters, TenacityValue);
    TenacityValue = FMath::Clamp<float>(TenacityValue, 0.0f, 1.0f);

    // 2. 기본 배율 (1.0 - 강인함)
    float Multiplier = (1.0f - TenacityValue);

    // 3. 최소 보장 비율 적용 (0.2 미만으로 떨어지지 않게 함)
    Multiplier = FMath::Max<float>(Multiplier, MinDurationPercent);

    return FMath::Clamp<float>(Multiplier, 0.0f, 1.0f);
}
