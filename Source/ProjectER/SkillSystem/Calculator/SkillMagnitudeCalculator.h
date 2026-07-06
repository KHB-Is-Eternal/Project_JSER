// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "AttributeSet.h"
#include "GameplayTagContainer.h"
#include "SkillMagnitudeCalculator.generated.h"

class UAbilitySystemComponent;
class USkillMagnitudeCalculator;
class UGameplayEffect;


/** 피연산자(값) 타입 */
UENUM(BlueprintType)
enum class ECalcOperandType : uint8
{
	Constant			UMETA(DisplayName = "상수 (Constant)"),
	SourceStat			UMETA(DisplayName = "시전자 스탯 (Source Stat)"),
	TargetStat			UMETA(DisplayName = "타겟 스탯 (Target Stat)"),
	SourceTagStack		UMETA(DisplayName = "시전자 태그 스택 (Source Tag Stack)"),
	TargetTagStack		UMETA(DisplayName = "타겟 태그 스택 (Target Tag Stack)"),
	SubFormula			UMETA(DisplayName = "하위 수식 (Sub Formula / 괄호)"),
	SetByCaller			UMETA(DisplayName = "SetByCaller 값 (SetByCaller)"),
	SourceEffectStack   UMETA(DisplayName = "시전자 GE 스택 (Source GE Stack)"),
	TargetEffectStack   UMETA(DisplayName = "타겟 GE 스택 (Target GE Stack)")
};

/** 연산자 타입 */
UENUM(BlueprintType)
enum class ECalcOperator : uint8
{
	Add			UMETA(DisplayName = "더하기 (+)"),
	Subtract	UMETA(DisplayName = "빼기 (-)"),
	Multiply	UMETA(DisplayName = "곱하기 (*)"),
	Divide		UMETA(DisplayName = "나누기 (/)")
};

/**
 * 계산기 내부의 단일 연산 스텝입니다.
 * (예: "더하기" "타겟 태그 스택" "Status.Poison")
 */
USTRUCT(BlueprintType)
struct PROJECTER_API FCalcStep
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Calculation")
	ECalcOperator Operator = ECalcOperator::Add;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Calculation")
	ECalcOperandType OperandType = ECalcOperandType::Constant;

	// ECalcOperandType::Constant 일 때 사용
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Calculation", meta=(EditCondition="OperandType==ECalcOperandType::Constant", EditConditionHides))
	float ConstantValue = 0.0f;

	// ECalcOperandType::SourceStat 또는 TargetStat 일 때 사용
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Calculation", meta=(EditCondition="OperandType==ECalcOperandType::SourceStat || OperandType==ECalcOperandType::TargetStat", EditConditionHides))
	FGameplayAttribute StatAttribute;

	// ECalcOperandType::SourceTagStack, TargetTagStack, SetByCaller 일 때 사용
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Calculation", meta=(EditCondition="OperandType==ECalcOperandType::SourceTagStack || OperandType==ECalcOperandType::TargetTagStack || OperandType==ECalcOperandType::SetByCaller", EditConditionHides))
	FGameplayTag Tag;

	// ECalcOperandType::SubFormula 일 때 사용
	UPROPERTY(EditAnywhere, Instanced, BlueprintReadWrite, Category = "Calculation", meta=(EditCondition="OperandType==ECalcOperandType::SubFormula", EditConditionHides))
	TObjectPtr<USkillMagnitudeCalculator> SubFormula;

	// ECalcOperandType::SourceEffectStack 또는 TargetEffectStack 일 때 사용
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Calculation", meta=(EditCondition="OperandType==ECalcOperandType::SourceEffectStack || OperandType==ECalcOperandType::TargetEffectStack", EditConditionHides))
	TSubclassOf<UGameplayEffect> EffectClass;
};

/**
 * 데이터 주도형 스킬 연산기(Parser) 클래스입니다.
 * SkillDataAsset의 디테일 패널에서 직접 구조체 배열로 수식을 조립할 수 있습니다.
 */
UCLASS(BlueprintType, EditInlineNew, DefaultToInstanced)
class PROJECTER_API USkillMagnitudeCalculator : public UObject
{
	GENERATED_BODY()

public:
	// 연산 시작 기준값
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Calculation")
	float InitialValue = 0.0f;

	// 기획자가 순차적으로 조립하는 연산자/피연산자 레고 배열
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Calculation")
	TArray<FCalcStep> FormulaSteps;

	/** 
	 * 최종 연산을 수행합니다. 
	 * 타겟 ASC가 널(Null)이더라도 안전하게 동작(관련 값 0 처리)합니다.
	 */
	UFUNCTION(BlueprintCallable, Category = "Calculation")
	float CalculateValue(UAbilitySystemComponent* SourceASC, UAbilitySystemComponent* TargetASC) const;

	// C++ 전용 오버로드 (UFUNCTION 없음, FGameplayEffectSpec 포인터 전달 가능)
	float CalculateValue(UAbilitySystemComponent* SourceASC, UAbilitySystemComponent* TargetASC, const struct FGameplayEffectSpec* Spec) const;

protected:
	float GetOperandValue(const FCalcStep& Step, UAbilitySystemComponent* SourceASC, UAbilitySystemComponent* TargetASC, const struct FGameplayEffectSpec* Spec) const;
};
