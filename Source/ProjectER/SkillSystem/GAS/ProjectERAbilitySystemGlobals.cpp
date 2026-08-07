// Fill out your copyright notice in the Description page of Project Settings.

#include "SkillSystem/GAS/ProjectERAbilitySystemGlobals.h"
#include "SkillSystem/GAS/ProjectERGameplayEffectContext.h"

FGameplayEffectContext* UProjectERAbilitySystemGlobals::AllocGameplayEffectContext() const
{
	// 전용 커스텀 컨텍스트를 할당하여 반환
	return new FProjectERGameplayEffectContext();
}
