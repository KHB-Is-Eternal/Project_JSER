// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Abilities/GameplayAbilityTargetActor.h"
#include "SkillSystem/SkillDataAsset.h"
#include "SkillSystem/SkillConfig/BaseSkillConfig.h"
#include "MouseLocationTargetActor.generated.h"

class ASkillIndicatorActor;

UCLASS()
class PROJECTER_API AMouseLocationTargetActor : public AGameplayAbilityTargetActor
{
	GENERATED_BODY()
	
public:
	AMouseLocationTargetActor();

	virtual void StartTargeting(UGameplayAbility* Ability) override;
	virtual void ConfirmTargetingAndContinue() override;
	virtual void Tick(float DeltaSeconds) override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	bool TryConfirmMouseLocation();
	bool SubmitExternalLocation(const FVector& InLocation);

	/** 방향 지시선 설정 및 최대 물리 사거리 주입 */
	void Setup(const FSkillIndicatorConfig& InIndicatorConfig, float InMaxRange);

protected:
	UPROPERTY()
	TObjectPtr<ASkillIndicatorActor> SpawnedIndicator;

	FSkillIndicatorConfig IndicatorConfig;
	float MaxRange = 0.f;
};
