// Fill out your copyright notice in the Description page of Project Settings.

#include "SkillSystem/Calculator/SkillMagnitudeCalculator.h"
#include "AbilitySystemComponent.h"

float USkillMagnitudeCalculator::CalculateValue(UAbilitySystemComponent* SourceASC, UAbilitySystemComponent* TargetASC) const
{
	float Result = InitialValue;

	for (const FCalcStep& Step : FormulaSteps)
	{
		float OperandValue = GetOperandValue(Step, SourceASC, TargetASC);

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
				UE_LOG(LogTemp, Warning, TEXT("USkillMagnitudeCalculator: 0으로 나누려고 시도하여 무시되었습니다."));
			}
			break;
		}
	}

	return Result;
}

float USkillMagnitudeCalculator::GetOperandValue(const FCalcStep& Step, UAbilitySystemComponent* SourceASC, UAbilitySystemComponent* TargetASC) const
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
			UE_LOG(LogTemp, Warning, TEXT("USkillMagnitudeCalculator: TargetASC가 존재하지 않지만 TargetStat을 참조하려고 했습니다. 0을 반환합니다."));
		}
		break;

	case ECalcOperandType::SourceTagStack:
		if (SourceASC && Step.StackTag.IsValid())
		{
			return static_cast<float>(SourceASC->GetTagCount(Step.StackTag));
		}
		break;

	case ECalcOperandType::TargetTagStack:
		if (TargetASC && Step.StackTag.IsValid())
		{
			return static_cast<float>(TargetASC->GetTagCount(Step.StackTag));
		}
		else if (!TargetASC)
		{
			UE_LOG(LogTemp, Warning, TEXT("USkillMagnitudeCalculator: TargetASC가 존재하지 않지만 TargetTagStack을 참조하려고 했습니다. 0을 반환합니다."));
		}
		break;

	case ECalcOperandType::SubFormula:
		if (Step.SubFormula)
		{
			return Step.SubFormula->CalculateValue(SourceASC, TargetASC);
		}
		else
		{
			UE_LOG(LogTemp, Warning, TEXT("USkillMagnitudeCalculator: SubFormula가 nullptr입니다. 0을 반환합니다."));
		}
		break;
	}

	return 0.0f;
}
