// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "GameplayEffectTypes.h"
#include "GameplayPrediction.h"
#include "SkillSystem/GameplayCueNotify/ProjectERSummonedActorInterface.h"
#include "BaseMissileActor.generated.h"

class UProjectileMovementComponent;
class USceneComponent;

/**
 * 물리 충돌체 없이 거리 체크로 대상 도달 여부를 판단하는 미사일 액터.
 * Tick에서 대상과의 거리를 계산하여 ReachThreshold 이내이면 적중 처리합니다.
 */
UCLASS()
class PROJECTER_API ABaseMissileActor : public AActor, public IProjectERSummonedActorInterface
{
	GENERATED_BODY()

public:
	ABaseMissileActor();

	// IProjectERSummonedActorInterface interface implementation
	virtual void OnVfxHandshakeCompleted_Implementation(AActor* VfxActor) override;
	
	UFUNCTION()
	void OnRep_InstigatorActor();

	/**
	 * GEC에서 호출하여 미사일의 모든 데이터를 초기화합니다.
	 * FinishSpawning 전에 호출되어야 합니다.
	 */
	void InitializeMissile(
		const TArray<FGameplayEffectSpecHandle>& InEffectSpecHandles,
		AActor* InInstigatorActor,
		AActor* InHomingTarget,
		const FGameplayCueParameters& InHitVfxCueParameters,
		const FGameplayCueParameters& InHitSoundCueParameters,
		float InInitialSpeed,
		float InMaxSpeed,
		float InHomingAcceleration,
		float InReachThreshold,
		bool bInDestroyOnHit,
		const FVector& InInitialDirection = FVector::ForwardVector
	);

	/** 서버에서 시전 시간을 초기화합니다. */
	void SetClientActivationTime(float InTime) { ClientActivationTime = InTime; }

protected:
	virtual void BeginPlay() override;
	virtual void PostNetInit() override;
	virtual void Tick(float DeltaTime) override;

	/** 대상에 도달했을 때 호출. 효과 적용 및 파괴를 수행합니다. */
	virtual void OnReachedTarget();

	/** 타겟에게 효과를 적용합니다. */
	void ApplyEffectsToTarget(AActor* TargetActor);

	/** 적중 효과(VFX, Sound)를 실행합니다. */
	void ExecuteHitCues();

public:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Missile")
	TObjectPtr<USceneComponent> SceneRoot;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Missile")
	TObjectPtr<UProjectileMovementComponent> ProjectileMovement;

protected:
	UPROPERTY()
	TArray<FGameplayEffectSpecHandle> EffectSpecHandles;

	UPROPERTY(ReplicatedUsing = OnRep_InstigatorActor)
	TObjectPtr<AActor> InstigatorActor;

	UPROPERTY()
	TObjectPtr<AActor> HomingTargetActor;

	UPROPERTY()
	FGameplayCueParameters HitVfxCueParameters;

	UPROPERTY()
	FGameplayCueParameters HitSoundCueParameters;

	UPROPERTY()
	float ReachThreshold = 50.0f;

	UPROPERTY()
	bool bDestroyOnHit = true;

	UPROPERTY(Replicated)
	FRotator InitialTargetRotation; // 추가: 엔진에 의한 회전값 왜곡 방지용

	bool bHasReached = false;

protected:
	/** 리플리케이션된 시전 시간 (VFX 핸드셰이크용) */
	UPROPERTY(Replicated)
	float ClientActivationTime;
};
