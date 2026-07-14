// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "SkillSystem/GameplayEffectComponent/MoveBaseGEC.h"
#include "MoveToDynamicForceGEC.generated.h"

/**
 * UMoveToDynamicForceGEC
 * 목적지 또는 타겟 액터를 향해 이동하며, 타겟이 움직일 경우 실시간으로 추적할 수 있는 이동기 GEC입니다.
 */
UCLASS(DontCollapseCategories)
class PROJECTER_API UMoveToDynamicForceGEC : public UMoveBaseGEC
{
	GENERATED_BODY()

public:
	UMoveToDynamicForceGEC();

	virtual FSkillTooltipData GetTooltipDescription(int32 Level, TSubclassOf<class USkillBase> AbilityClass) const override;

	virtual float CalculateMoveDuration(const FGameplayEffectSpec& GESpec, const AActor* Instigator, const FVector& Direction) const override;

protected:
	virtual void Execute(AActor* Instigator, const FVector& Direction, const FGameplayEffectSpec& GESpec, FPredictionKey PredictionKey) const override;

public:
	// --- Dynamic Move Settings ---
	
	// [이동 속도 설정]
	// 현재 재생 중인 몽타주의 남은 시간을 이동 지속 시간으로 사용할지 여부
	UPROPERTY(EditDefaultsOnly, Category = "Move|Dynamic|Speed")
	bool bUseMontageDuration = false;

	// 이동 소요 시간 (초) (bUseMontageDuration이 꺼져 있을 때 사용)
	UPROPERTY(EditDefaultsOnly, Category = "Move|Dynamic|Speed", meta = (EditCondition = "!bUseMontageDuration"))
	float Duration = 0.5f;

	// 예상 속도 초과 방지 (타겟 추격 시 급가속 방지)
	UPROPERTY(EditDefaultsOnly, Category = "Move|Dynamic|Speed")
	bool bRestrictSpeedToExpected = true;

	// [추격 설정]
	// 이동 중에도 타겟 액터의 위치를 실시간으로 추적할지 여부 (TowardTarget일 때만 활성화)
	UPROPERTY(EditDefaultsOnly, Category = "Move|Dynamic|Tracking", meta = (EditCondition = "DirectionSource == EMoveDirectionSource::TowardTarget"))
	bool bTrackTargetActor = true;

	// [종료 판정 설정]
	// 목적지 도달 판정 거리 (cm) (TowardTarget 및 추격 활성화 시에만 사용)
	UPROPERTY(EditDefaultsOnly, Category = "Move|Dynamic|Stopping", meta = (EditCondition = "DirectionSource == EMoveDirectionSource::TowardTarget && bTrackTargetActor"))
	float ReachedDestinationDistance = 50.0f;

	// [비주얼 설정]
	// 이동 경로 오프셋 커브 (선택)
	UPROPERTY(EditDefaultsOnly, Category = "Move|Dynamic|Visuals")
	TObjectPtr<UCurveVector> PathOffsetCurve;
};
