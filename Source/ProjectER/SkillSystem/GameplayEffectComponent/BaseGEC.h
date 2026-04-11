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

public:


	/**
	 * [Phase 1 - 준비]
	 * GE가 적용되기 전에 호출되어 보정된 좌표(Lag 보상 등)를 Context에 기록합니다.
	 * 서버/클라이언트(예측) 양측에서 호출됩니다.
	 */
	virtual void PreApplyEffect(UAbilitySystemComponent* ASC, const FGameplayEffectContextHandle& ContextHandle, const FGameplayEffectSpecHandle& SpecHandle) const {}

	/**
	 * [Phase 2 - 비주얼 실행]
	 * 기록된 Context(Origin 등)를 기반으로 로컬 예측 및 서버 브로드캐스트용 GameplayCue를 실행합니다.
	 */
	virtual void OnExecutePredictive(UAbilitySystemComponent* ASC, const FGameplayEffectContextHandle& ContextHandle, const FGameplayEffectSpecHandle& SpecHandle) const {}

protected:

private:

public:

protected:

private:

};

