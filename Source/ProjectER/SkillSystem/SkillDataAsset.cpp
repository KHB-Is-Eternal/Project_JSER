// Fill out your copyright notice in the Description page of Project Settings.


#include "SkillSystem/SkillDataAsset.h"
#include "GameplayEffect.h"
#include "AbilitySystemComponent.h"
#include "GameplayAbilitySpec.h"
#include "GameAbility/SkillBase.h"
#include "SkillSystem/GameAbility/MouseClickSkill.h"
#include "SkillSystem/GameAbility/MouseTargetSkill.h"
#include "SkillSystem/GameAbility/InstantSkill.h"
#include "SkillConfig/BaseSkillConfig.h"
#include "SkillSystem/GameplayEffectComponent/BaseGEC.h"

FGameplayAbilitySpec USkillDataAsset::MakeSpec()
{
	TSubclassOf<USkillBase> AbilityClass = SkillConfig->AbilityClass;
	TSubclassOf<UGameplayAbility> ClassToUse = AbilityClass ? AbilityClass : TSubclassOf<UGameplayAbility>(USkillBase::StaticClass());

	FGameplayAbilitySpec Spec(ClassToUse, 1);

	Spec.SourceObject = this;

	Spec.GetDynamicSpecSourceTags().AddTag(SkillConfig->GetInputKeyTag());

    return Spec;
}

FString USkillDataAsset::GetTargetingStyleText(TSubclassOf<class USkillBase> AbilityClass)
{
	if (AbilityClass)
	{
		if (AbilityClass->IsChildOf(UMouseClickSkill::StaticClass()))
		{
			return TEXT("지정된 위치에 사용 시, ");
		}
		else if (AbilityClass->IsChildOf(UMouseTargetSkill::StaticClass()))
		{
			return TEXT("지정된 대상에게 적중 시, ");
		}
		else if (AbilityClass->IsChildOf(UInstantSkill::StaticClass()))
		{
			return TEXT("즉시 ");
		}
	}
	return TEXT("즉시 "); // Default fallback
}

FSkillTooltipData USkillDataAsset::GetSkillTooltipData(int32 InLevel) const
{
	FSkillTooltipData Result;

	if (!IsValid(SkillConfig))
	{
		return Result;
	}

	// 레벨 최소치 보장 (1레벨 이상)
	const int32 Level = FMath::Max(1, InLevel);

	Result.SkillName = SkillName;
	Result.DetailedDescription = DetailedDescription;

	TArray<FString> GECShorts;
	TArray<FString> EffectDescriptions;
	TSubclassOf<USkillBase> AbilityClass = SkillConfig->AbilityClass;

	TArray<TSubclassOf<UBaseGameplayEffect>> AllEffects;

	for (const FSkillExecutionPhase& Phase : SkillConfig->GetExecutionPhases())
	{
		AllEffects.Append(Phase.Effects);
	}

	if (const UMouseTargetSkillConfig* TargetConfig = Cast<UMouseTargetSkillConfig>(SkillConfig))
	{
		for (const FTargetExecutionPhase& TargetPhase : TargetConfig->GetTargetPhases())
		{
			AllEffects.Append(TargetPhase.TargetEffects);
		}
	}

	for (const TSubclassOf<UBaseGameplayEffect>& EffectClass : AllEffects)
	{
		if (!IsValid(EffectClass))
		{
			continue;
		}

		const UBaseGameplayEffect* EffectDef = EffectClass->GetDefaultObject<UBaseGameplayEffect>();
		if (!EffectDef) continue;

		for (const UGameplayEffectComponent* GEC : EffectDef->GetGEComponents())
		{
			if (const UBaseGEC* BaseGEC = Cast<UBaseGEC>(GEC))
			{
				FSkillTooltipData GECTooltip = BaseGEC->GetTooltipDescription(Level, AbilityClass);
				
				if (!GECTooltip.ShortDescription.IsEmpty())
				{
					GECShorts.Add(GECTooltip.ShortDescription.ToString());
				}
				
				if (!GECTooltip.DetailedDescription.IsEmpty())
				{
					EffectDescriptions.Add(GECTooltip.DetailedDescription.ToString());
				}
			}
		}
	}

	FText GEEffectsText = UBaseGEC::FormatAppliedEffects(AllEffects, Level);
	if (!GEEffectsText.IsEmpty())
	{
		EffectDescriptions.Add(GEEffectsText.ToString());
	}

	// 1. 발동 조건(조준 방식) 텍스트 가져오기
	FString TriggerPrefix = GetTargetingStyleText(AbilityClass);

	// 2. GEC들에서 수집한 짧은 설명들 조립
	FString CombinedGECShort = FString::Join(GECShorts, TEXT(", "));

	// 3. 최종 ShortDescription 조립 (발동 조건 + 효과 설명)
	FString FinalShort;
	if (!CombinedGECShort.IsEmpty())
	{
		FinalShort = TriggerPrefix + CombinedGECShort;
	}

	// 에셋에 기획자가 직접 입력한 기본 짧은 설명이 있다면 합쳐줌
	if (!ShortDescription.IsEmpty())
	{
		if (!FinalShort.IsEmpty())
		{
			Result.ShortDescription = FText::FromString(FString::Printf(TEXT("%s (%s)"), *ShortDescription.ToString(), *FinalShort));
		}
		else
		{
			Result.ShortDescription = ShortDescription;
		}
	}
	else
	{
		Result.ShortDescription = FText::FromString(FinalShort);
	}

	Result.SkillEffectDescription = FText::FromString(FString::Join(EffectDescriptions, TEXT("\n\n")));

	if (!Result.SkillEffectDescription.IsEmpty())
	{
		if (!Result.DetailedDescription.IsEmpty())
		{
			Result.DetailedDescription = FText::FromString(FString::Printf(TEXT("%s\n\n%s"), *Result.DetailedDescription.ToString(), *Result.SkillEffectDescription.ToString()));
		}
		else
		{
			Result.DetailedDescription = Result.SkillEffectDescription;
		}
	}

	Result.CooldownSeconds = SkillConfig->GetBaseCooldownDuration(Level);
	Result.CostDescription = SkillConfig->BuildCostDescription(Level);
	Result.SKillIcon = SKillIcon;

	return Result;
}