// Fill out your copyright notice in the Description page of Project Settings.

#include "WatchTagAbility_Instant.h"

bool UWatchTagAbility_Instant::ProcessEventAndCheckCondition(const FGameplayEventData& Payload, float& OutEventMagnitude)
{
	OutEventMagnitude = Payload.EventMagnitude;
	// 쿨타임 및 태그 쿼리는 부모 클래스(OnEventReceived)에서 이미 통과한 상태이므로 즉시 발동합니다.
	return true;
}
