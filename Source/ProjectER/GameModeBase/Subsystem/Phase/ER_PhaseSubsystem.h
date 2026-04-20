// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "ER_PhaseSubsystem.generated.h"

class AER_GameState;
class UGameplayEffect;
class ULevelAreaGameModeComponent;
class ULevelAreaTrackerComponent;
class APawn;
UCLASS()
class PROJECTER_API UER_PhaseSubsystem : public UWorldSubsystem
{
	GENERATED_BODY()
	

public:
	void StartPhaseTimer(AER_GameState& GS, float Duration);
	void StartNoticeTimer(float Duration);
	void ClearPhaseTimer();

	void OnPhaseTimeUp();
	void OnNoticeTimeUp();

private:
	void OnPeriodicCheckTick();
	UGameplayEffect* GetOrCreateHazardDamageEffect();

private:
	FTimerHandle PhaseTimer;
	FTimerHandle NoticeTimer;
	FTimerHandle PeriodicCheckTimer;
	
	TWeakObjectPtr<class AER_GameState> CachedGameState;

	TWeakObjectPtr<ULevelAreaGameModeComponent> CachedAreaGameModeComp;

	// 캐싱할 타이머 기반 트래커들 (메모리 릭 방지를 위해 TWeakObjectPtr 사용)
	TMap<TWeakObjectPtr<APawn>, TWeakObjectPtr<ULevelAreaTrackerComponent>> CachedTrackers;

	UPROPERTY(Transient)
	TObjectPtr<UGameplayEffect> CachedHazardDamageEffect = nullptr;
};
