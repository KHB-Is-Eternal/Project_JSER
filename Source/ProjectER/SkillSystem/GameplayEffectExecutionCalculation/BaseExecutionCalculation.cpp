// Fill out your copyright notice in the Description page of Project Settings.


#include "SkillSystem/GameplayEffectExecutionCalculation/BaseExecutionCalculation.h"
#include "SkillSystem/SkillDataAsset.h"
#include "CharacterSystem/GAS/AttributeSet/BaseAttributeSet.h"

#define ATTRIBUTE_CLASS UBaseAttributeSet

#define DECLARE_ST_CAPTUREDEF(AttributeName) \
    DECLARE_ATTRIBUTE_CAPTUREDEF(AttributeName##Source); \
    DECLARE_ATTRIBUTE_CAPTUREDEF(AttributeName##Target);

#define DEFINE_ST_CAPTUREDEF(AttributeName) \
    AttributeName##SourceDef = FGameplayEffectAttributeCaptureDefinition(ATTRIBUTE_CLASS::Get##AttributeName##Attribute(), EGameplayEffectAttributeCaptureSource::Source, false); \
    AttributeName##TargetDef = FGameplayEffectAttributeCaptureDefinition(ATTRIBUTE_CLASS::Get##AttributeName##Attribute(), EGameplayEffectAttributeCaptureSource::Target, false); \
    SourceAttributeMap.Add(ATTRIBUTE_CLASS::Get##AttributeName##Attribute(), AttributeName##SourceDef); \
    TargetAttributeMap.Add(ATTRIBUTE_CLASS::Get##AttributeName##Attribute(), AttributeName##TargetDef);

struct FAttributeStatics
{
    DECLARE_ST_CAPTUREDEF(Level);
    DECLARE_ST_CAPTUREDEF(MaxLevel);
    DECLARE_ST_CAPTUREDEF(XP);
    DECLARE_ST_CAPTUREDEF(MaxXP);
    DECLARE_ST_CAPTUREDEF(Health);
    DECLARE_ST_CAPTUREDEF(MaxHealth);
    DECLARE_ST_CAPTUREDEF(HealthRegen);
    DECLARE_ST_CAPTUREDEF(Stamina);
    DECLARE_ST_CAPTUREDEF(MaxStamina);
    DECLARE_ST_CAPTUREDEF(StaminaRegen);

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

    DECLARE_ATTRIBUTE_CAPTUREDEF(IncomingDamage); // 메타 속성 (기존 방식)

    TMap<FGameplayAttribute, FGameplayEffectAttributeCaptureDefinition> SourceAttributeMap;
    TMap<FGameplayAttribute, FGameplayEffectAttributeCaptureDefinition> TargetAttributeMap;

    FAttributeStatics()
    {
        // --- 2. 정의 및 맵 등록 (헬퍼 하나로 4가지 작업 동시 수행) ---
        //DEFINE_ST_CAPTUREDEF(Level);
        //DEFINE_ST_CAPTUREDEF(MaxLevel);
        //DEFINE_ST_CAPTUREDEF(XP);
        //DEFINE_ST_CAPTUREDEF(MaxXP);
        DEFINE_ST_CAPTUREDEF(Health);
        DEFINE_ST_CAPTUREDEF(MaxHealth);
        DEFINE_ST_CAPTUREDEF(HealthRegen);
        DEFINE_ST_CAPTUREDEF(Stamina);
        DEFINE_ST_CAPTUREDEF(MaxStamina);
        DEFINE_ST_CAPTUREDEF(StaminaRegen);

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

        // IncomingDamage는 Target 전용이므로 별도 등록
        DEFINE_ATTRIBUTE_CAPTUREDEF(ATTRIBUTE_CLASS, IncomingDamage, Target, false);
        TargetAttributeMap.Add(ATTRIBUTE_CLASS::GetIncomingDamageAttribute(), IncomingDamageDef);
    }
};

static const FAttributeStatics& AttributeStatics()
{
    static FAttributeStatics Statics;
    return Statics;
}

UBaseExecutionCalculation::UBaseExecutionCalculation()
{
    // 생성자에서 맵에 등록된 모든 Def를 엔진에 알립니다.
    // 이 작업이 있어야 AttemptCalculate... 함수가 0이 아닌 실제 값을 반환합니다.
    for (auto& Pair : AttributeStatics().SourceAttributeMap)
    {
        RelevantAttributesToCapture.Add(Pair.Value);
    }

    for (auto& Pair : AttributeStatics().TargetAttributeMap)
    {
        RelevantAttributesToCapture.Add(Pair.Value);
    }
}

void UBaseExecutionCalculation::Execute_Implementation(const FGameplayEffectCustomExecutionParameters& ExecutionParams, FGameplayEffectCustomExecutionOutput& OutExecutionOutput) const
{
	Super::Execute_Implementation(ExecutionParams, OutExecutionOutput);

    // TODO: Implement calculation logic using native GE Modifiers and captured attributes
    // FindValueByAttribute(ExecutionParams, UBaseAttributeSet::GetDefenseAttribute(), AttributeStatics().TargetAttributeMap);
}

float UBaseExecutionCalculation::FindValueByAttribute(const FGameplayEffectCustomExecutionParameters& ExecutionParams, const FGameplayAttribute& Attribute, const TMap<FGameplayAttribute, FGameplayEffectAttributeCaptureDefinition>& TargetMap) const
{
    float FoundValue = 0.f;

    // 인자로 넘어온 맵에서 정의를 찾습니다.
    if (const FGameplayEffectAttributeCaptureDefinition* FoundDef = TargetMap.Find(Attribute))
    {
        ExecutionParams.AttemptCalculateCapturedAttributeMagnitude(*FoundDef, FAggregatorEvaluateParameters(), FoundValue);
    }
    else
    {
        UE_LOG(LogTemp, Warning, TEXT("ExecutionCalculation: 선택된 맵에 [%s] 속성이 등록되지 않았습니다!"), *Attribute.GetName());
    }

    return FoundValue;
}

