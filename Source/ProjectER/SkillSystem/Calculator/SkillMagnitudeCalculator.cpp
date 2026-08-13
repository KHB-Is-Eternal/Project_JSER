// Fill out your copyright notice in the Description page of Project Settings.

#include "SkillSystem/Calculator/SkillMagnitudeCalculator.h"
#include "AbilitySystemComponent.h"
#include "GameplayEffect.h"


float USkillMagnitudeCalculator::CalculateValue(UAbilitySystemComponent* SourceASC, UAbilitySystemComponent* TargetASC) const
{
	return CalculateValue(SourceASC, TargetASC, nullptr, 0);
}

float USkillMagnitudeCalculator::CalculateValue(UAbilitySystemComponent* SourceASC, UAbilitySystemComponent* TargetASC, const FGameplayEffectSpec* Spec) const
{
	return CalculateValue(SourceASC, TargetASC, Spec, 0);
}

float USkillMagnitudeCalculator::CalculateValue(UAbilitySystemComponent* SourceASC, UAbilitySystemComponent* TargetASC, const FGameplayEffectSpec* Spec, int32 RecursionDepth) const
{
	if (RecursionDepth > 16)
	{
		UE_LOG(LogTemp, Warning, TEXT("USkillMagnitudeCalculator: Maximum recursion depth exceeded (> 16). Circular reference detected. Returning 0."));
		return 0.0f;
	}

	float Result = InitialValue;

	for (const FCalcStep& Step : FormulaSteps)
	{
		float OperandValue = GetOperandValue(Step, SourceASC, TargetASC, Spec, RecursionDepth);

		switch (Step.Operator)
		{
		case ECalcOperator::Add:
			Result += OperandValue;
			break;
		case ECalcOperator::Subtract:
			Result -= OperandValue;
			break;
		case ECalcOperator::Multiply:
			Result *= OperandValue;
			break;
		case ECalcOperator::Divide:
			if (OperandValue != 0.0f)
			{
				Result /= OperandValue;
			}
			else
			{
				UE_LOG(LogTemp, Log, TEXT("USkillMagnitudeCalculator: Attempted to divide by zero. Ignored."));
			}
			break;
		}
	}

	return Result;
}

float USkillMagnitudeCalculator::GetOperandValue(const FCalcStep& Step, UAbilitySystemComponent* SourceASC, UAbilitySystemComponent* TargetASC, const FGameplayEffectSpec* Spec, int32 RecursionDepth) const
{
	switch (Step.OperandType)
	{
	case ECalcOperandType::Constant:
		return Step.ConstantValue;

	case ECalcOperandType::SourceStat:
		if (SourceASC)
		{
			bool bFound = false;
			float Value = SourceASC->GetNumericAttribute(Step.StatAttribute);
			return Value; // 값이 없으면(혹은 어트리뷰트가 유효하지 않으면) 기본적으로 0 반환
		}
		break;

	case ECalcOperandType::TargetStat:
		if (TargetASC)
		{
			return TargetASC->GetNumericAttribute(Step.StatAttribute);
		}
		else
		{
			UE_LOG(LogTemp, Log, TEXT("USkillMagnitudeCalculator: TargetASC is null but attempted to reference TargetStat. Returning 0."));
		}
		break;

	case ECalcOperandType::SourceTagStack:
		if (SourceASC && Step.Tag.IsValid())
		{
			return static_cast<float>(SourceASC->GetTagCount(Step.Tag));
		}
		break;

	case ECalcOperandType::TargetTagStack:
		if (TargetASC && Step.Tag.IsValid())
		{
			return static_cast<float>(TargetASC->GetTagCount(Step.Tag));
		}
		else if (!TargetASC)
		{
			UE_LOG(LogTemp, Log, TEXT("USkillMagnitudeCalculator: TargetASC is null but attempted to reference TargetTagStack. Returning 0."));
		}
		break;

	case ECalcOperandType::SubFormula:
		if (Step.SubFormula)
		{
			return Step.SubFormula->CalculateValue(SourceASC, TargetASC, Spec, RecursionDepth + 1);
		}
		else
		{
			UE_LOG(LogTemp, Log, TEXT("USkillMagnitudeCalculator: SubFormula is null. Returning 0."));
		}
		break;

	case ECalcOperandType::SetByCaller:
		if (Spec && Step.Tag.IsValid())
		{
			if (Spec->SetByCallerTagMagnitudes.Contains(Step.Tag))
			{
				return Spec->SetByCallerTagMagnitudes.FindRef(Step.Tag);
			}
		}
		break;

	case ECalcOperandType::SourceEffectStack:
		if (SourceASC && Step.EffectClass)
		{
			FGameplayEffectQuery Query;
			Query.EffectDefinition = Step.EffectClass;

			int32 TotalStack = 0;
			TArray<FActiveGameplayEffectHandle> ActiveEffects = SourceASC->GetActiveEffects(Query);
			for (const FActiveGameplayEffectHandle& Handle : ActiveEffects)
			{
				const FActiveGameplayEffect* ActiveGE = SourceASC->GetActiveGameplayEffect(Handle);
				if (ActiveGE)
				{
					TotalStack += ActiveGE->Spec.GetStackCount();
				}
			}
			return static_cast<float>(TotalStack);
		}
		break;

	case ECalcOperandType::TargetEffectStack:
		if (TargetASC && Step.EffectClass)
		{
			FGameplayEffectQuery Query;
			Query.EffectDefinition = Step.EffectClass;

			int32 TotalStack = 0;
			TArray<FActiveGameplayEffectHandle> ActiveEffects = TargetASC->GetActiveEffects(Query);
			for (const FActiveGameplayEffectHandle& Handle : ActiveEffects)
			{
				const FActiveGameplayEffect* ActiveGE = TargetASC->GetActiveGameplayEffect(Handle);
				if (ActiveGE)
				{
					TotalStack += ActiveGE->Spec.GetStackCount();
				}
			}
			return static_cast<float>(TotalStack);
		}
		else if (!TargetASC)
		{
			UE_LOG(LogTemp, Log, TEXT("USkillMagnitudeCalculator: TargetASC is null but attempted to reference TargetEffectStack. Returning 0."));
		}
		break;
	}

	return 0.0f;
}
