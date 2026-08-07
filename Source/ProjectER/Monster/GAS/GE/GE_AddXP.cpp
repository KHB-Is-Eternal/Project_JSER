#include "Monster/GAS/GE/GE_AddXP.h"
#include "CharacterSystem/GAS/AttributeSet/BaseAttributeSet.h"

UGE_AddXP::UGE_AddXP()
{
	DurationPolicy = EGameplayEffectDurationType::Instant;
	PeriodicInhibitionPolicy = EGameplayEffectPeriodInhibitionRemovedPolicy::NeverReset;

	FGameplayModifierInfo Modifier;
	Modifier.Attribute = UBaseAttributeSet::GetIncomingXPAttribute();
	Modifier.ModifierOp = EGameplayModOp::Additive;
	FSetByCallerFloat Caller;
	Caller.DataTag = FGameplayTag::RequestGameplayTag("Status.XP");
	Modifier.ModifierMagnitude = FGameplayEffectModifierMagnitude(Caller);

	Modifiers.Add(Modifier);
} 
