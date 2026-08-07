// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "SkillSystem/GameAbility/SkillBase.h"
#include "InstantSkill.generated.h"

/**
 * 
 */
UCLASS()
class PROJECTER_API UInstantSkill : public USkillBase
{
	GENERATED_BODY()

public:
	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;

protected:
	virtual void StartIndicatorMode(bool bIsManual) override;
	virtual TSubclassOf<class AGameplayAbilityTargetActor> GetTargetActorClass() const override;
	virtual void OnTargetDataReady(const FGameplayAbilityTargetDataHandle& DataHandle) override;
};
