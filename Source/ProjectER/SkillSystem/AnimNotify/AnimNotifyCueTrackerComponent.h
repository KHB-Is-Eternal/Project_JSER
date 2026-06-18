// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "AnimNotifyCueTrackerComponent.generated.h"

class UNiagaraComponent;
class UAudioComponent;
class UAnimMontage;
class USkeletalMeshComponent;
class USkillNiagaraSpawnConfig;
class USkillSoundSpawnConfig;

/**
 * 실제로 추적 중인 개별 Niagara / Audio 이펙트 컴포넌트 정보
 */
USTRUCT(BlueprintType)
struct FTrackedCueComponent
{
	GENERATED_BODY()

#if WITH_EDITORONLY_DATA
	UPROPERTY()
	TWeakObjectPtr<UAnimMontage> TargetMontage = nullptr;

	UPROPERTY()
	TWeakObjectPtr<USkeletalMeshComponent> MeshComponent = nullptr;

	UPROPERTY()
	TWeakObjectPtr<UNiagaraComponent> NiagaraComponent = nullptr;

	UPROPERTY()
	TWeakObjectPtr<UAudioComponent> AudioComponent = nullptr;

	UPROPERTY()
	bool bWasPaused = false;
#endif // WITH_EDITORONLY_DATA
};

/**
 * 아직 월드상에 스폰되지 않았거나 감시를 대기 중인 설정 정보
 */
USTRUCT(BlueprintType)
struct FPendingTrackedCue
{
	GENERATED_BODY()

#if WITH_EDITORONLY_DATA
	UPROPERTY()
	TWeakObjectPtr<UAnimMontage> TargetMontage = nullptr;

	UPROPERTY()
	TWeakObjectPtr<USkeletalMeshComponent> MeshComponent = nullptr;

	UPROPERTY()
	TWeakObjectPtr<UObject> TargetAsset = nullptr; // UNiagaraSystem 혹은 USoundBase
#endif // WITH_EDITORONLY_DATA
};

/**
 * 몽타주 일시정지 상태와 연동하여 GameplayCue로 생성된 파티클/사운드를 Pause/Resume 시켜주는 트래커 컴포넌트
 */
UCLASS()
class PROJECTER_API UAnimNotifyCueTrackerComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UAnimNotifyCueTrackerComponent();

	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

#if WITH_EDITOR
	/** 대상 액터에서 트래커 컴포넌트를 찾거나 없으면 새로 생성하여 부착합니다. */
	static UAnimNotifyCueTrackerComponent* GetOrCreateTracker(AActor* Owner);

	/** Niagara 이펙트 추적을 등록합니다. */
	void RegisterNiagaraCue(USkeletalMeshComponent* MeshComp, UAnimMontage* Montage, USkillNiagaraSpawnConfig* SpawnConfig);

	/** 사운드 이펙트 추적을 등록합니다. */
	void RegisterSoundCue(USkeletalMeshComponent* MeshComp, UAnimMontage* Montage, USkillSoundSpawnConfig* SpawnConfig);

	/** 특정 에셋에 대해 추적 및 대기 목록에서 해제합니다. (NotifyEnd 시점에 호출) */
	void UnregisterCue(USkeletalMeshComponent* MeshComp, UObject* TargetAsset);
#endif // WITH_EDITOR

private:
#if WITH_EDITORONLY_DATA
	UPROPERTY()
	TArray<FTrackedCueComponent> TrackedComponents;

	UPROPERTY()
	TArray<FPendingTrackedCue> PendingCues;
#endif // WITH_EDITORONLY_DATA
};
