// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "SkillSystem/GameplayEffectComponent/BaseGEC.h"
#include "VfxSfxGEC.generated.h"

class USkillNiagaraSpawnConfig;
class USkillSoundSpawnConfig;

/**
 * GameplayEffect의 생명주기(적용, 실행, 제거) 시점에 맞춰 
 * 지정된 VFX와 SFX를 실행하는 컴포넌트입니다.
 */
UCLASS(DontCollapseCategories)
class PROJECTER_API UVfxSfxGEC : public UBaseGEC
{
	GENERATED_BODY()

public:
	UVfxSfxGEC();

protected:
	/** GE가 처음 적용될 때 호출됩니다. (Trigger 효과 실행 및 Removal 바인딩용) */
	virtual bool OnActiveGameplayEffectAdded(FActiveGameplayEffectsContainer& ActiveGEContainer, FActiveGameplayEffect& ActiveGE) const override;

	/** GE가 실행(Instant)되거나 주기적(Periodic)으로 틱이 발생할 때 호출됩니다. */
	virtual void OnGameplayEffectExecuted(FActiveGameplayEffectsContainer& ActiveGEContainer, FGameplayEffectSpec& GESpec, FPredictionKey& PredictionKey) const override;

	/** 공통 큐 실행 로직 */
	void ExecuteEffects(UAbilitySystemComponent* ASC, const FGameplayEffectSpec& GESpec, const USkillNiagaraSpawnConfig* VfxConfig, const USkillSoundSpawnConfig* SoundConfig, FPredictionKey PredictionKey = FPredictionKey()) const;

public:
	/** 발동(Trigger) 시점에 실행할 VFX (Instant 실행 또는 Duration 최초 적용 시) */
	UPROPERTY(EditDefaultsOnly, Instanced, Category = "VfxSfx|VFX")
	TObjectPtr<USkillNiagaraSpawnConfig> TriggerVfx;

	/** 발동(Trigger) 시점에 실행할 사운드 (Instant 실행 또는 Duration 최초 적용 시) */
	UPROPERTY(EditDefaultsOnly, Instanced, Category = "VfxSfx|SFX")
	TObjectPtr<USkillSoundSpawnConfig> TriggerSound;

	/** 주기적(Periodic) 틱마다 실행할 VFX */
	UPROPERTY(EditDefaultsOnly, Instanced, Category = "VfxSfx|VFX")
	TObjectPtr<USkillNiagaraSpawnConfig> PeriodicVfx;

	/** 주기적(Periodic) 틱마다 실행할 사운드 */
	UPROPERTY(EditDefaultsOnly, Instanced, Category = "VfxSfx|SFX")
	TObjectPtr<USkillSoundSpawnConfig> PeriodicSound;

	/** 제거(Removed) 시점에 실행할 VFX */
	UPROPERTY(EditDefaultsOnly, Instanced, Category = "VfxSfx|VFX")
	TObjectPtr<USkillNiagaraSpawnConfig> RemovedVfx;

	/** 제거(Removed) 시점에 실행할 사운드 */
	UPROPERTY(EditDefaultsOnly, Instanced, Category = "VfxSfx|SFX")
	TObjectPtr<USkillSoundSpawnConfig> RemovedSound;
};
