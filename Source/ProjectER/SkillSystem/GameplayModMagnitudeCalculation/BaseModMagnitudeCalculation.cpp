// Fill out your copyright notice in the Description page of Project Settings.


#include "SkillSystem/GameplayModMagnitudeCalculation/BaseModMagnitudeCalculation.h"
#include "SkillSystem/SkillDataAsset.h"
#include "CharacterSystem/GAS/AttributeSet/BaseAttributeSet.h"
#include "GameplayEffectExecutionCalculation.h" // GAS Attribute Capture macros (DECLARE_ATTRIBUTE_CAPTUREDEF, etc.)

#define ATTRIBUTE_CLASS UBaseAttributeSet


#define DECLARE_ST_CAPTUREDEF(AttributeName) \
    DECLARE_ATTRIBUTE_CAPTUREDEF(AttributeName##Source); \
    DECLARE_ATTRIBUTE_CAPTUREDEF(AttributeName##Target);

#define DEFINE_ST_CAPTUREDEF(AttributeName) \
    AttributeName##SourceDef = FGameplayEffectAttributeCaptureDefinition(ATTRIBUTE_CLASS::Get##AttributeName##Attribute(), EGameplayEffectAttributeCaptureSource::Source, true); \
    AttributeName##TargetDef = FGameplayEffectAttributeCaptureDefinition(ATTRIBUTE_CLASS::Get##AttributeName##Attribute(), EGameplayEffectAttributeCaptureSource::Target, true); \
    SourceAttributeMap.Add(ATTRIBUTE_CLASS::Get##AttributeName##Attribute(), AttributeName##SourceDef); \
    TargetAttributeMap.Add(ATTRIBUTE_CLASS::Get##AttributeName##Attribute(), AttributeName##TargetDef);

struct FMMCAttributeStatics
{
    // Vital
    DECLARE_ST_CAPTUREDEF(Health);
    DECLARE_ST_CAPTUREDEF(MaxHealth);
    DECLARE_ST_CAPTUREDEF(MaxStamina);

    // Combat (핵심 전투 스탯)
    DECLARE_ST_CAPTUREDEF(AttackPower);
    DECLARE_ST_CAPTUREDEF(AttackSpeed);
    DECLARE_ST_CAPTUREDEF(AttackRange);
    DECLARE_ST_CAPTUREDEF(SkillAmp);
    DECLARE_ST_CAPTUREDEF(CriticalChance);
    DECLARE_ST_CAPTUREDEF(CriticalDamage);
    DECLARE_ST_CAPTUREDEF(Defense);
    DECLARE_ST_CAPTUREDEF(MoveSpeed);
    DECLARE_ST_CAPTUREDEF(CooldownReduction);
    DECLARE_ST_CAPTUREDEF(Tenacity);

    TMap<FGameplayAttribute, FGameplayEffectAttributeCaptureDefinition> SourceAttributeMap;
    TMap<FGameplayAttribute, FGameplayEffectAttributeCaptureDefinition> TargetAttributeMap;

    FMMCAttributeStatics()
    {
        // Vital
        DEFINE_ST_CAPTUREDEF(Health);
        DEFINE_ST_CAPTUREDEF(MaxHealth);
        DEFINE_ST_CAPTUREDEF(MaxStamina);

        // Combat
        DEFINE_ST_CAPTUREDEF(AttackPower);
        DEFINE_ST_CAPTUREDEF(AttackSpeed);
        DEFINE_ST_CAPTUREDEF(AttackRange);
        DEFINE_ST_CAPTUREDEF(SkillAmp);
        DEFINE_ST_CAPTUREDEF(CriticalChance);
        DEFINE_ST_CAPTUREDEF(CriticalDamage);
        DEFINE_ST_CAPTUREDEF(Defense);
        DEFINE_ST_CAPTUREDEF(MoveSpeed);
        DEFINE_ST_CAPTUREDEF(CooldownReduction);
        DEFINE_ST_CAPTUREDEF(Tenacity);
    }
};

static const FMMCAttributeStatics& MMCAttributeStatics()
{
    static FMMCAttributeStatics Statics;
    return Statics;
}

UBaseModMagnitudeCalculation::UBaseModMagnitudeCalculation()
{
    for (auto& Pair : MMCAttributeStatics().SourceAttributeMap)
    {
        RelevantAttributesToCapture.Add(Pair.Value);
    }

    for (auto& Pair : MMCAttributeStatics().TargetAttributeMap)
    {
        RelevantAttributesToCapture.Add(Pair.Value);
    }
}

float UBaseModMagnitudeCalculation::CalculateBaseMagnitude_Implementation(const FGameplayEffectSpec& Spec) const
{
    // TODO: Implement calculation logic using native GE Modifiers and captured attributes
    // e.g., float BaseValue = Spec.GetSetByCallerMagnitude(...);
    return 0.f;
}



float UBaseModMagnitudeCalculation::FindValueByAttribute(const FGameplayEffectSpec& Spec, const FGameplayAttribute& Attribute, const TMap<FGameplayAttribute, FGameplayEffectAttributeCaptureDefinition>& TargetMap) const
{
    float FoundValue = 0.f;

    if (const FGameplayEffectAttributeCaptureDefinition* FoundDef = TargetMap.Find(Attribute))
    {
        // MMC에서는 GetCapturedAttributeMagnitude를 사용합니다.
        GetCapturedAttributeMagnitude(*FoundDef, Spec, FAggregatorEvaluateParameters(), FoundValue);
    }

    return FoundValue;
}

