// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameplayEffectComponent.h"
#include "BaseGEC.generated.h"

/**
 *
 */


class UAbilitySystemComponent;
class UGameplayAbility;
struct FGameplayEffectContextHandle;
struct FGameplayEffectSpecHandle;
struct FGameplayEffectSpec;

UCLASS()
class PROJECTER_API UBaseGEC : public UGameplayEffectComponent
{
	GENERATED_BODY()

public:
	UBaseGEC();



protected:
	virtual void OnGameplayEffectExecuted(FActiveGameplayEffectsContainer& ActiveGEContainer, FGameplayEffectSpec& GESpec, FPredictionKey& PredictionKey) const override;



	static void GetSkillProcEffects(UAbilitySystemComponent* InstigatorASC, UGameplayAbility* InstigatorSkill,  AActor* InEffectCauser,  const FGameplayEffectContextHandle& CurrentContext,  TArray<FGameplayEffectSpecHandle>& OutSpecs, bool bDefaultConsume = true);
private:

public:

protected:

private:

};

