// Fill out your copyright notice in the Description page of Project Settings.


#include "SkillSystem/GameplayEffectExecutionCalculation/DamageExecutionCalculation.h"
#include "SkillSystem/SkillDataAsset.h"
#include "CharacterSystem/GAS/AttributeSet/BaseAttributeSet.h"
#include "CharacterSystem/GameplayTags/GameplayTags.h"


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
    DECLARE_ATTRIBUTE_CAPTUREDEF(Defense);
    DECLARE_ATTRIBUTE_CAPTUREDEF(IncomingDamage);

    TMap<FGameplayAttribute, FGameplayEffectAttributeCaptureDefinition> TargetAttributeMap;

    FAttributeStatics()
    {
        // 최적화: 데미지 계산에 필요한 방어력과 누적 데미지를 캡처합니다.
        DEFINE_ATTRIBUTE_CAPTUREDEF(ATTRIBUTE_CLASS, Defense, Target, false);
        DEFINE_ATTRIBUTE_CAPTUREDEF(ATTRIBUTE_CLASS, IncomingDamage, Target, false);

        TargetAttributeMap.Add(ATTRIBUTE_CLASS::GetDefenseAttribute(), DefenseDef);
        TargetAttributeMap.Add(ATTRIBUTE_CLASS::GetIncomingDamageAttribute(), IncomingDamageDef);
    }
};

static const FAttributeStatics& AttributeStatics()
{
    static FAttributeStatics Statics;
    return Statics;
}

UDamageExecutionCalculation::UDamageExecutionCalculation()
{
    // RelevantAttributesToCapture에 등록된 속성만 캡처가 수행됩니다.
    RelevantAttributesToCapture.Add(AttributeStatics().DefenseDef);
    RelevantAttributesToCapture.Add(AttributeStatics().IncomingDamageDef);
}

void UDamageExecutionCalculation::Execute_Implementation(const FGameplayEffectCustomExecutionParameters& ExecutionParams, FGameplayEffectCustomExecutionOutput& OutExecutionOutput) const
{

	Super::Execute_Implementation(ExecutionParams, OutExecutionOutput);



    const FGameplayEffectSpec& Spec = ExecutionParams.GetOwningSpec();

    // 1. 캡처한 IncomingDamage 어트리뷰트에서 기초 데미지(Raw Damage) 가져오기
    float BaseDamage = FindValueByAttribute(ExecutionParams, UBaseAttributeSet::GetIncomingDamageAttribute(), AttributeStatics().TargetAttributeMap);



    // 2. 방어력 캡처
    float TargetDefense = FindValueByAttribute(ExecutionParams, UBaseAttributeSet::GetDefenseAttribute(), AttributeStatics().TargetAttributeMap);
    TargetDefense = FMath::Max<float>(TargetDefense, 0.0f);


    if (BaseDamage <= 0.0f)
    {
        return;
    }

    // 3. 가상 함수를 통해 자식 클래스의 특수 계산식 실행
    float FinalDamage = CalculateFinalDamage(BaseDamage, TargetDefense, Spec);



    // 4. 최종 산출된 데미지를 IncomingDamage 어트리뷰트에 주입 (+)
    if (FinalDamage > 0.0f)
    {
        OutExecutionOutput.AddOutputModifier(FGameplayModifierEvaluatedData(UBaseAttributeSet::GetIncomingDamageAttribute(), EGameplayModOp::Additive, FinalDamage));
    }

}

float UDamageExecutionCalculation::FindValueByAttribute(const FGameplayEffectCustomExecutionParameters& ExecutionParams, const FGameplayAttribute& Attribute, const TMap<FGameplayAttribute, FGameplayEffectAttributeCaptureDefinition>& TargetMap) const
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


#undef ATTRIBUTE_CLASS
#undef DECLARE_ST_CAPTUREDEF
#undef DEFINE_ST_CAPTUREDEF
