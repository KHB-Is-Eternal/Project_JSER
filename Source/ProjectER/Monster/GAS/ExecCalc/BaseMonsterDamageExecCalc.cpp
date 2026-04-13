#include "Monster/GAS/ExecCalc/BaseMonsterDamageExecCalc.h"
#include "CharacterSystem/GAS/AttributeSet/BaseAttributeSet.h"

struct FMonsterDamageCapture
{
	DECLARE_ATTRIBUTE_CAPTUREDEF(Defense);
	DECLARE_ATTRIBUTE_CAPTUREDEF(AttackPower);

	FMonsterDamageCapture()
	{
		DEFINE_ATTRIBUTE_CAPTUREDEF(UBaseAttributeSet, Defense, Target, false);
		DEFINE_ATTRIBUTE_CAPTUREDEF(UBaseAttributeSet, AttackPower, Source, true);
	}
};

static FMonsterDamageCapture& MonsterDamageCapture()
{
	static FMonsterDamageCapture MonsterDamageCapture;
	return MonsterDamageCapture;
}


UBaseMonsterDamageExecCalc::UBaseMonsterDamageExecCalc()
{
	bRequiresPassedInTags = false;

	RelevantAttributesToCapture.Add(MonsterDamageCapture().DefenseDef);
	RelevantAttributesToCapture.Add(MonsterDamageCapture().AttackPowerDef);
}

void UBaseMonsterDamageExecCalc::Execute_Implementation(const FGameplayEffectCustomExecutionParameters& ExecutionParams, FGameplayEffectCustomExecutionOutput& OutExecutionOutput) const
{
	Super::Execute_Implementation(ExecutionParams, OutExecutionOutput);

	UAbilitySystemComponent* TargetASC = ExecutionParams.GetTargetAbilitySystemComponent();
	UAbilitySystemComponent* SourceASC = ExecutionParams.GetSourceAbilitySystemComponent();

	const FGameplayEffectSpec& Spec = ExecutionParams.GetOwningSpec();
	const FGameplayTagContainer* SourceTags = Spec.CapturedSourceTags.GetAggregatedTags();
	const FGameplayTagContainer* TargetTags = Spec.CapturedTargetTags.GetAggregatedTags();

	FAggregatorEvaluateParameters EvaluationParameters;
	EvaluationParameters.SourceTags = SourceTags;
	EvaluationParameters.TargetTags = TargetTags;

	float Defense = 0.f;
	ExecutionParams.AttemptCalculateCapturedAttributeMagnitude(MonsterDamageCapture().DefenseDef, EvaluationParameters, Defense);
	Defense = FMath::Max<float>(Defense, 0.0f);

	float AttackPower = 0.f;
	ExecutionParams.AttemptCalculateCapturedAttributeMagnitude(MonsterDamageCapture().AttackPowerDef, EvaluationParameters, AttackPower);
	AttackPower = FMath::Max<float>(AttackPower, 0.0f);

	float Mitigation = 100.0f / (100.0f + Defense);
	float FinalDamage = AttackPower * Mitigation;

	if (FinalDamage > 0.f)
	{
		OutExecutionOutput.AddOutputModifier(FGameplayModifierEvaluatedData(UBaseAttributeSet::GetIncomingDamageAttribute(), EGameplayModOp::Additive, FinalDamage));
	}
}
