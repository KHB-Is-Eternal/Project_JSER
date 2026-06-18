// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "SkillSystem/GameplayEffectComponent/MoveBaseGEC.h"
#include "JumpForceMoveGEC.generated.h"

/**
 * UJumpForceMoveGEC
 * 기존 LaunchMoveGEC를 대체하며, 엔진의 JumpForce 루트 모션을 사용하여 안정적인 도약을 구현합니다.
 */
UCLASS(DontCollapseCategories)
class PROJECTER_API UJumpForceMoveGEC : public UMoveBaseGEC
{
	GENERATED_BODY()

public:
	UJumpForceMoveGEC();

	virtual FSkillTooltipData GetTooltipDescription(int32 Level, TSubclassOf<class USkillBase> AbilityClass) const override;

	virtual float CalculateMoveDuration(const FGameplayEffectSpec& GESpec, const AActor* Instigator, const FVector& Direction) const override;

protected:
	virtual void Execute(AActor* Instigator, const FVector& Direction, const FGameplayEffectSpec& GESpec, FPredictionKey PredictionKey) const override;

public:
	// --- Jump Settings ---
	
	// 점프 최고 높이 (cm)
	UPROPERTY(EditDefaultsOnly, Category = "Move|Jump")
	float JumpHeight = 300.0f;

	// 체공 시간 (초). 0이면 중력에 따라 자동 계산됩니다.
	UPROPERTY(EditDefaultsOnly, Category = "Move|Jump")
	float JumpDuration = 0.5f;

	// 점프 궤적 커브 오프셋 (선택)
	UPROPERTY(EditDefaultsOnly, Category = "Move|Jump")
	TObjectPtr<UCurveVector> PathOffsetCurve;
};
