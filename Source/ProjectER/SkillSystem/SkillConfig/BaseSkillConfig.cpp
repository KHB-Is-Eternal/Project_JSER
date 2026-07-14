// Fill out your copyright notice in the Description page of Project Settings.

#include "SkillSystem/SkillConfig/BaseSkillConfig.h"
#include "SkillSystem/GameAbility/SkillBase.h"
#include "SkillSystem/GameAbility/MouseTargetSkill.h"
#include "SkillSystem/GameAbility/MouseClickSkill.h"
#include "SkillSystem/GameAbility/InstantSkill.h"
#include "SkillSystem/GameAbility/WatchTagAbility_Instant.h"
#include "SkillSystem/GameAbility/WatchTagAbility_Accumulate.h"
#include "CharacterSystem/GameplayTags/GameplayTags.h"

UBaseSkillConfig::UBaseSkillConfig()
{
	AbilityClass = USkillBase::StaticClass();
}

const TArray<FSkillExecutionPhase>& UBaseSkillConfig::GetExecutionPhases() const
{
	static TArray<FSkillExecutionPhase> EmptyPhases;
	return EmptyPhases;
}

UActiveSkillConfig::UActiveSkillConfig()
{
	AbilityClass = USkillBase::StaticClass();
	bAllowMovementDuringSkill = false;
}

UGameplayEffect* UActiveSkillConfig::CreateCostGameplayEffect(UObject* Outer)
{
    if (SkillCosts.Num() <= 0)
    {
        return nullptr;
    }

    // GE 인스턴스 생성 (Outer를 TransientPackage로 설정하여 관리)
    UGameplayEffect* NewCostGE = NewObject<UGameplayEffect>(Outer);
    NewCostGE->DurationPolicy = EGameplayEffectDurationType::Instant;

    for (const FSkillCostInfo& CostInfo : SkillCosts)
    {
        if (!CostInfo.Attribute.IsValid()) continue;

        FGameplayModifierInfo ModInfo;
        ModInfo.Attribute = CostInfo.Attribute;
        ModInfo.ModifierOp = EGameplayModOp::Additive;
        ModInfo.ModifierMagnitude = FGameplayEffectModifierMagnitude(CostInfo.CostValue);
        NewCostGE->Modifiers.Add(ModInfo);
    }

    return NewCostGE;
}

FText UActiveSkillConfig::BuildCostDescription(float InLevel) const
{
    TArray<FString> CostTerms;

    for (const FSkillCostInfo& SkillCost : SkillCosts)
    {
        if (SkillCost.Attribute.IsValid())
        {
            const float CostValue = SkillCost.CostValue.GetValueAtLevel(InLevel);
            FString Term = FString::Printf(TEXT("%s %.0f"), *SkillCost.Attribute.GetName(), CostValue);
            CostTerms.Add(Term);
        }
    }

    if (CostTerms.Num() > 0)
    {
        FString Combined = TEXT("소모: ") + FString::Join(CostTerms, TEXT(", "));
        return FText::FromString(Combined);
    }

    return FText::GetEmpty();
}

UMouseTargetSkillConfig::UMouseTargetSkillConfig()
{
	AbilityClass = UMouseTargetSkill::StaticClass();
}

UMouseClickSkillConfig::UMouseClickSkillConfig()
{
	AbilityClass = UMouseClickSkill::StaticClass();
}

UInstantSkillConfig::UInstantSkillConfig()
{
    AbilityClass = UInstantSkill::StaticClass();
}

UPassiveSkillConfig::UPassiveSkillConfig()
{
	// 패시브 클래스의 기본값은 별도로 매핑되므로 base에서는 StaticClass만 지정합니다.
	AbilityClass = USkillBase::StaticClass();
}

UPassiveInstantSkillConfig::UPassiveInstantSkillConfig()
{
	AbilityClass = UWatchTagAbility_Instant::StaticClass();
}

UPassiveAccumulateSkillConfig::UPassiveAccumulateSkillConfig()
{
	AbilityClass = UWatchTagAbility_Accumulate::StaticClass();
}
