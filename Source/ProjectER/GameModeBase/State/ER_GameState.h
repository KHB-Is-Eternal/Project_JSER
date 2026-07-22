#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameStateBase.h"
#include "ER_GameState.generated.h"


class AER_PlayerState;
class UCharacterData;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnPhaseChangedBP, int32, NewPhase);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnPlayerStateChangedSignature, class APlayerState*, InPlayerState);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnGameStartedBP);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnHazardVisualsFinishedBP);

// this is for the mpc update
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnHazardZonesChanged, const TArray<int32>&, NewDangerZoneIDs);

UCLASS()
class PROJECTER_API AER_GameState : public AGameStateBase
{
	GENERATED_BODY()
	
public:
	
	AER_GameState();
	
	virtual void AddPlayerState(APlayerState* PlayerState) override;
	virtual void RemovePlayerState(APlayerState* PlayerState) override;

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	void BuildTeamCache();
	void RemoveTeamCache();

	TArray<FString>& GetTeamArray(int32 TeamIdx);

	// 재접속 호환: UniqueId 문자열로 PlayerState 찾기
	UFUNCTION(BlueprintPure)
	AER_PlayerState* GetPlayerStateByUniqueId(const FString& InUniqueIdStr) const;

	bool GetTeamEliminate(int32 idx);

	int32 GetLastTeamIdx();

	UFUNCTION()
	void OnRep_Phase();

	UFUNCTION()
	void OnRep_GameStarted();

	float GetPhaseRemainingTime() const;

	UFUNCTION(BlueprintCallable)
	int32 GetCurrentPhase() { return CurrentPhase; }

	UFUNCTION(BlueprintCallable)
	void SetCurrentPhase(int32 input) { CurrentPhase = input; }

	// 사용 가능한 캐릭터 데이터 목록 반환
	UFUNCTION(BlueprintPure, Category = "Character Selection")
	const TArray<TSoftObjectPtr<UCharacterData>>& GetAvailableCharacterData() const;

	// MPC Update Reaction purpsoe
	UFUNCTION(NetMulticast, Reliable)
	void Multicast_OnHazardPhaseChanged(const TArray<int32>& NewDangerZoneIDs);

	UFUNCTION(BlueprintImplementableEvent, Category = "Hazard")
	void OnDangerZonesReceived(const TArray<int32>& NewDangerZoneIDs);

	// 구역 색 전환 타임라인(OnDangerZonesReceived 내부)의 Finished 핀에서 호출 — 색 전환 완료 시점 알림용
	UFUNCTION(BlueprintCallable, Category = "Hazard")
	void NotifyHazardVisualsFinished();

	// 위험 구역 강도 설정 (경고=0.5, 위험=1.0 등 자유롭게 지정)
	UFUNCTION(NetMulticast, Reliable)
	void Multicast_SetHazardIntensity(const TArray<int32>& ZoneIDs, float Intensity);

	UFUNCTION(BlueprintImplementableEvent, Category = "Hazard")
	void OnHazardIntensityReceived(const TArray<int32>& ZoneIDs, float Intensity);


public:
	UPROPERTY(BlueprintReadOnly)
	TMap<int32, bool> TeamElimination;

	UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_Phase)
	float PhaseServerTime = 0.f;

	UPROPERTY(BlueprintReadOnly, Replicated)
	float PhaseDuration = 10.f;

	UPROPERTY(BlueprintAssignable)
	FOnPhaseChangedBP OnPhaseChanged;

	UPROPERTY(BlueprintAssignable)
	FOnGameStartedBP OnGameStarted;

	UPROPERTY(ReplicatedUsing = OnRep_GameStarted)
	bool bGameStarted = false;

	UPROPERTY(BlueprintAssignable, Category = "Hazard")
	FOnHazardZonesChanged OnHazardZonesChanged;

	UPROPERTY(BlueprintAssignable, Category = "Hazard")
	FOnHazardVisualsFinishedBP OnHazardVisualsFinished;

	UPROPERTY(BlueprintAssignable, Category = "GameState|Events")
	FOnPlayerStateChangedSignature OnPlayerStateAddedDelegate;

	UPROPERTY(BlueprintAssignable, Category = "GameState|Events")
	FOnPlayerStateChangedSignature OnPlayerStateRemovedDelegate;

protected:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Character Selection")
	TArray<TSoftObjectPtr<UCharacterData>> AvailableCharacterData;


private:
	TArray<TArray<FString>> TeamCache;

	UPROPERTY(ReplicatedUsing = OnRep_Phase)
	int32 CurrentPhase = 0;

// chat
public:
	UFUNCTION(NetMulticast, Reliable)
	void Multicast_BroadcastChatMessage(const FString& Message);

};

