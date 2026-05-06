// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "SkillSystem/GameplayEffect/BaseGameplayEffect.h"
#include "SkillSystem/GameplayEffectComponent/BaseGEC.h"
#include "AdditionalEffectGEC.generated.h"

class USkillNiagaraSpawnConfig;
class USkillSoundSpawnConfig;
 
struct FActiveGameplayEffectsContainer;
struct FActiveGameplayEffect;


/**
 * GEC used to identify and store additional effects in a SkillProc buff.
 */
UCLASS(DontCollapseCategories)
class PROJECTER_API UAdditionalEffectGEC : public UBaseGEC
{
	GENERATED_BODY()

public:
	UAdditionalEffectGEC();

	virtual bool OnActiveGameplayEffectAdded(FActiveGameplayEffectsContainer& ActiveGEContainer, FActiveGameplayEffect& ActiveGE) const override;

	UPROPERTY(EditDefaultsOnly, Category = "Additional|Logic")
	TArray<TSubclassOf<UBaseGameplayEffect>> Bonus;

	/** 효과 적용 후 이 버프를 소모(제거)할지 여부 */
	UPROPERTY(EditDefaultsOnly, Category = "Additional|Logic")
	bool bConsumeBuff = true;

	/** 버프가 활성화되어 있는 동안 재생할 나이아가라 효과 */
	UPROPERTY(EditDefaultsOnly, Category = "Additional|VFX")
	TObjectPtr<USkillNiagaraSpawnConfig> ActiveVfxConfig;

	/** 버프가 활성화되어 있는 동안 재생할 사운드 효과 */
	UPROPERTY(EditDefaultsOnly, Category = "Additional|SFX")
	TObjectPtr<USkillSoundSpawnConfig> ActiveSoundConfig;
};
