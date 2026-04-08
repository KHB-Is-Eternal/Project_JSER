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
	 * GEC가 보유한 CueTag가 설정된 Config 객체들을 수집합니다.
	 * UProjectERASC::FindCueConfig()에서 호출하여 CueTag별 SourceObject를 자동 매칭합니다.
	 * 파생 클래스에서 오버라이드하여 자신의 USkillNiagaraSpawnConfig / USkillSoundSpawnConfig를 등록하세요.
	 */
	virtual void CollectCueConfigs(TArray<const UObject*>& OutConfigs) const {}

	friend class UProjectERASC;
protected:

private:

public:

protected:

private:

};

