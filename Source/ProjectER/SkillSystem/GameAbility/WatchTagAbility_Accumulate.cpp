// Fill out your copyright notice in the Description page of Project Settings.

#include "WatchTagAbility_Accumulate.h"
#include "SkillSystem/SkillConfig/BaseSkillConfig.h"

bool UWatchTagAbility_Accumulate::ProcessEventAndCheckCondition(const FGameplayEventData& Payload)
{
	const UPassiveAccumulateSkillConfig* AccumConfig = Cast<UPassiveAccumulateSkillConfig>(PassiveConfig);
	if (!IsValid(AccumConfig))
	{
		return false;
	}

	// 이벤트 횟수와 Magnitude를 누적합니다.
	CurrentEventCount++;
	CurrentTotalValue += Payload.EventMagnitude;

	// 만료 타이머를 재시작합니다.
	RestartExpirationTimer();

	// 임계치 달성 여부를 확인합니다. 두 조건 모두 설정된 경우 AND 조건으로 작동합니다.
	const bool bCountConditionMet = (AccumConfig->RequiredEventCount <= 0) || (CurrentEventCount >= AccumConfig->RequiredEventCount);
	const bool bValueConditionMet = (AccumConfig->RequiredTotalValue <= 0.0f) || (CurrentTotalValue >= AccumConfig->RequiredTotalValue);

	if (!bCountConditionMet || !bValueConditionMet)
	{
		return false;
	}

	// 임계치를 달성했습니다. 초과분 이월 여부에 따라 누적치를 처리합니다.
	if (AccumConfig->bCarryOverExcess)
	{
		// 초과분만 남기고 임계치만큼 차감합니다.
		CurrentEventCount -= FMath::Max(0, AccumConfig->RequiredEventCount);
		CurrentTotalValue -= FMath::Max(0.0f, AccumConfig->RequiredTotalValue);
	}
	else
	{
		ResetAccumulation();
	}

	return true;
}

void UWatchTagAbility_Accumulate::ResetAccumulation()
{
	CurrentEventCount = 0;
	CurrentTotalValue = 0.0f;

	const UWorld* const World = GetWorld();
	if (World != nullptr)
	{
		World->GetTimerManager().ClearTimer(ExpirationTimerHandle);
	}
}

void UWatchTagAbility_Accumulate::RestartExpirationTimer()
{
	const UPassiveAccumulateSkillConfig* AccumConfig = Cast<UPassiveAccumulateSkillConfig>(PassiveConfig);
	if (!IsValid(AccumConfig) || AccumConfig->ExpirationTime <= 0.0f)
	{
		return;
	}

	UWorld* const World = GetWorld();
	if (World == nullptr)
	{
		return;
	}

	// 기존 타이머를 취소하고 새로 시작합니다.
	World->GetTimerManager().ClearTimer(ExpirationTimerHandle);
	World->GetTimerManager().SetTimer(
		ExpirationTimerHandle,
		this,
		&UWatchTagAbility_Accumulate::ResetAccumulation,
		AccumConfig->ExpirationTime,
		false
	);
}
