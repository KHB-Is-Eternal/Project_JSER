// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "WatchTagAbility_Base.h"
#include "WatchTagAbility_Accumulate.generated.h"

/**
 * 누적형 패시브 트리거 어빌리티.
 * 이벤트 발생 횟수(RequiredEventCount)와 이벤트 Magnitude 총합(RequiredTotalValue)을 누적하여
 * 임계치를 달성했을 때 발동합니다. 시간 만료 초기화 및 초과분 이월(Carry Over) 기능을 지원합니다.
 * 예: "5회 피격 시 반격", "총 1000 데미지 누적 수신 시 광폭화"
 */
UCLASS()
class PROJECTER_API UWatchTagAbility_Accumulate : public UWatchTagAbility_Base
{
	GENERATED_BODY()

protected:
	/** 이벤트를 누적하고 임계치 달성 여부를 반환합니다. */
	virtual bool ProcessEventAndCheckCondition(const FGameplayEventData& Payload) override;

private:
	/** 누적치를 0으로 초기화하고 만료 타이머를 정지합니다. */
	void ResetAccumulation();

	/** 만료 타이머를 (재)시작합니다. ExpirationTime이 0보다 클 때만 동작합니다. */
	void RestartExpirationTimer();

private:
	/** 현재까지 수신된 이벤트 누적 횟수 */
	int32 CurrentEventCount = 0;

	/** 현재까지 수신된 이벤트 Magnitude 누적 총합 */
	float CurrentTotalValue = 0.0f;

	/** 시간 만료 초기화를 처리하는 타이머 핸들 */
	FTimerHandle ExpirationTimerHandle;
};
