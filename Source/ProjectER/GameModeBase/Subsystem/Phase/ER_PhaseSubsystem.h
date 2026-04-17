// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "ER_PhaseSubsystem.generated.h"

class AER_GameState;
class UGameplayEffect;

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

	UPROPERTY(Transient)
	TObjectPtr<UGameplayEffect> CachedHazardDamageEffect = nullptr;
};
