#include "Monster/GAS/GE/GE_AttackDamage.h"
#include "Monster/GAS/ExecCalc/BaseMonsterDamageExecCalc.h"

UGE_AttackDamage::UGE_AttackDamage()
{
	DurationPolicy = EGameplayEffectDurationType::Instant;

	FGameplayEffectExecutionDefinition ExecutionDef;
	ExecutionDef.CalculationClass = UBaseMonsterDamageExecCalc::StaticClass();
	Executions.Add(ExecutionDef);
	
	bRequireModifierSuccessToTriggerCues = true;
	bSuppressStackingCues = false;
	
	FGameplayEffectCue ParticleCue;
	FGameplayEffectCue SoundCue;
	GameplayCues.Add(ParticleCue);
	GameplayCues.Add(SoundCue);

	StackingType = EGameplayEffectStackingType::None;
}
