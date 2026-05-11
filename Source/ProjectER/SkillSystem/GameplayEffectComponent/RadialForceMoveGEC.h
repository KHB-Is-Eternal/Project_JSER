// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "SkillSystem/GameplayEffectComponent/MoveBaseGEC.h"
#include "RadialForceMoveGEC.generated.h"

/**
 * URadialForceMoveGEC
 * 특정 지점을 중심으로 밀어내거나(Push) 당기는(Pull) 광역 이동 효과를 구현하는 GEC입니다.
 */
UCLASS(DontCollapseCategories)
class PROJECTER_API URadialForceMoveGEC : public UMoveBaseGEC
{
	GENERATED_BODY()

public:
	URadialForceMoveGEC();

	virtual float CalculateMoveDuration(const FGameplayEffectSpec& GESpec, const AActor* Instigator, const FVector& Direction) const override;

protected:
	virtual void Execute(AActor* Instigator, const FVector& Direction, const FGameplayEffectSpec& GESpec, FPredictionKey PredictionKey) const override;

public:
	// --- Radial Force Settings ---

	// 이동 시간 (초)
	UPROPERTY(EditDefaultsOnly, Category = "Move|Radial")
	float Duration = 0.3f;

	// 힘의 세기 (cm/s)
	UPROPERTY(EditDefaultsOnly, Category = "Move|Radial")
	float Strength = 1000.0f;

	// 영향 반경 (cm)
	UPROPERTY(EditDefaultsOnly, Category = "Move|Radial")
	float Radius = 500.0f;

	// true: 중심에서 밀어냄 (Knockback), false: 중심으로 당김 (Pull)
	UPROPERTY(EditDefaultsOnly, Category = "Move|Radial")
	bool bIsPush = true;

	// true: 수직 방향 힘 무시 (지면 넉백 전용)
	UPROPERTY(EditDefaultsOnly, Category = "Move|Radial")
	bool bNoZForce = true;

	// 중심에서의 거리에 따른 힘의 감쇠 커브 (선택)
	UPROPERTY(EditDefaultsOnly, Category = "Move|Radial")
	TObjectPtr<UCurveFloat> StrengthDistanceFalloff;
};
