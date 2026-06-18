// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "WatchTagAbility_Base.h"
#include "WatchTagAbility_Instant.generated.h"

/**
 * 즉발형 패시브 트리거 어빌리티.
 * 타이머와 누적 변수를 사용하지 않으며, 이벤트가 수신될 때마다 태그 쿼리만 통과하면 즉시 발동합니다.
 * 예: "피격 시 즉시 출혈 부여", "적 처치 시 즉시 체력 회복"
 */
UCLASS()
class PROJECTER_API UWatchTagAbility_Instant : public UWatchTagAbility_Base
{
	GENERATED_BODY()

protected:
	/** 이벤트가 수신되는 즉시 true를 반환하여 발동을 트리거합니다. */
	virtual bool ProcessEventAndCheckCondition(const FGameplayEventData& Payload, float& OutEventMagnitude) override;
};
