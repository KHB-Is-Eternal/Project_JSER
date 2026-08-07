// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Abilities/GameplayAbilityTargetActor.h"
#include "SkillSystem/SkillConfig/BaseSkillConfig.h"
#include "TargetActor.generated.h"

/**
 * 
 */
UCLASS()
class PROJECTER_API ATargetActor : public AGameplayAbilityTargetActor
{
	GENERATED_BODY()
public:
	ATargetActor();
	virtual void ConfirmTargetingAndContinue() override;
	bool TryConfirmMouseTarget();
	bool SubmitExternalTarget(AActor* InTargetActor);

	/** 최대 물리 사거리 주입 */
	void Setup(float InMaxRange);

protected:
	virtual void StartTargeting(UGameplayAbility* Ability) override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void Tick(float DeltaSeconds) override;

protected:
	float MaxRange = 0.f;
};
