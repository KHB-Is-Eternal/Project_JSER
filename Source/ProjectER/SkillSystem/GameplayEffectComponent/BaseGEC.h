// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameplayEffectComponent.h"
#include "GameplayPrediction.h"
#include "SkillSystem/Interfaces/SkillVisualDataProvider.h"
#include "BaseGEC.generated.h"

/**
 *
 */


class UAbilitySystemComponent;
class UGameplayAbility;
struct FGameplayEffectContextHandle;
struct FGameplayEffectSpecHandle;
struct FGameplayEffectSpec;

UCLASS(DontCollapseCategories)
class PROJECTER_API UBaseGEC : public UGameplayEffectComponent, public ISkillVisualDataProvider
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
	virtual void PreApplyEffect(UAbilitySystemComponent* ASC, const FGameplayEffectContextHandle& ContextHandle, const FGameplayEffectSpec& GESpec) const {}

	/**
	 * [Phase 2 - 비주얼 실행]
	 * 기록된 Context(Origin 등)를 기반으로 로컬 예측용 GameplayCue를 실행합니다.
	 * [V7.3] 예측 실행은 시전자 클라이언트에서만 호출되도록 변경되었습니다. (SkillBase 처리)
	 */
	virtual void OnExecutePredictive(UAbilitySystemComponent* ASC, const FGameplayEffectContextHandle& ContextHandle, const FGameplayEffectSpec& GESpec) const {}

	/**
	 * [Phase 2.5 - VFX 브로드캐스트]
	 * 서버에서 GameplayEffectが 적용된 후 호출되어 관전자들에게 VFX를 브로드캐스트합니다.
	 * OnExecutePredictive와 달리, 이 함수는 서버 권한으로만 실행됩니다.
	 */
	virtual void OnExecuteVFXCue(UAbilitySystemComponent* ASC, const FGameplayEffectContextHandle& ContextHandle, const FGameplayEffectSpec& GESpec, FPredictionKey PredictionKey = FPredictionKey()) const {}

	/** GCN 액터(AGCN_SummonedActor) 초기화를 위한 데이터 제공 함수들 */
	virtual class USkillNiagaraSpawnConfig* GetAGCN_NiagaraConfig() const override { return nullptr; }
	virtual class USkillSoundSpawnConfig* GetAGCN_SoundConfig() const override { return nullptr; }
	virtual void SetupMovement(class UProjectileMovementComponent* Movement) const {}

protected:

private:

public:

protected:

private:

};

