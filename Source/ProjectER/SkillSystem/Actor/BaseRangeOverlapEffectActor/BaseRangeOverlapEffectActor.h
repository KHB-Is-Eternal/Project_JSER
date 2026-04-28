// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "GameplayEffectTypes.h"
#include "GameplayPrediction.h"
#include "SkillSystem/GameplayCueNotify/Particle/SkillNiagaraSpawnSettings.h"
#include "SkillSystem/Interfaces/SkillSummonedActor.h"
#include "BaseRangeOverlapEffectActor.generated.h"

class UShapeComponent;
class USphereComponent;
class UBoxComponent;
class UCapsuleComponent;
class UAreaPeriodicEffectComponent;

UCLASS()
class PROJECTER_API ABaseRangeOverlapEffectActor : public AActor, public ISkillSummonedActor {
	GENERATED_BODY()

public:
  // Sets default values for this actor's properties
  ABaseRangeOverlapEffectActor();

  // ISkillSummonedActor interface implementation
  virtual void OnVfxHandshakeCompleted_Implementation(AActor* VfxActor) override;

  UFUNCTION()
  void OnRep_InstigatorActor();

  void InitializeEffectData(
      const TArray<FGameplayEffectSpecHandle> &InEffectSpecHandles,
      AActor *InInstigatorActor, const FVector &InCollisionSize,
      bool bInHitOncePerTarget, const UObject *InHitTargetCueSourceObject,
      const FGameplayCueParameters &InHitTargetVfxCueParameters,
      const FGameplayCueParameters &InHitTargetSoundCueParameters);

  /** 컴포넌트를 이 액터의 도트 관리자로 설정 (GEC에서 호출) */
  void SetAreaPeriodicComponent(UAreaPeriodicEffectComponent* InComponent);

  void InitializePeriodicCues(const FGameplayCueParameters& InPeriodicVfxCueParameters, const FGameplayCueParameters& InPeriodicSoundCueParameters);

  /** 서버에서 시전 시간을 초기화합니다. */
  void SetClientActivationTime(float InTime) { ClientActivationTime = InTime; }

protected:
  virtual void BeginPlay() override;
  virtual void PostNetInit() override;
  virtual void ApplyCollisionSize(const FVector &InCollisionSize);
  void SetCollisionComponent(UShapeComponent *InCollisionComponent);

  UFUNCTION()
  virtual void OnShapeBeginOverlap(UPrimitiveComponent *OverlappedComp, AActor *OtherActor, UPrimitiveComponent *OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult &SweepResult);

  UFUNCTION()
  virtual void OnShapeEndOverlap(UPrimitiveComponent* OverlappedComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex);

  /** 주기적 트리거 발생 시 호출되는 로직 */
  UFUNCTION()
  virtual void OnAreaPeriodicTrigger(const TArray<AActor*>& Targets);

  /** 타겟들에게 효과 적용 */
  void ApplyEffectsToTargets(const TArray<AActor*>& Targets);
  void ApplyEffectsToTarget(AActor* TargetActor);

public:
  UPROPERTY()
  bool bDestroyOnOverlap = false;

protected:
  UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Collision")
  TObjectPtr<UShapeComponent> CollisionComponent;

  UPROPERTY()
  TArray<FGameplayEffectSpecHandle> EffectSpecHandles;

  UPROPERTY(ReplicatedUsing = OnRep_InstigatorActor)
  TObjectPtr<AActor> InstigatorActor;

  UPROPERTY()
  bool bHitOncePerTarget = true;

  UPROPERTY()
  FVector PendingCollisionSize = FVector::ZeroVector;

  UPROPERTY()
  bool bHasPendingCollisionSize = false;

  UPROPERTY()
  TSet<TObjectPtr<AActor>> HitActors;

  UPROPERTY()
  TObjectPtr<const UObject> HitTargetCueSourceObject;

  UPROPERTY()
  FGameplayCueParameters HitTargetVfxCueParameters;

  UPROPERTY()
  FGameplayCueParameters HitTargetSoundCueParameters;

  /** 도트 로직용 컴포넌트 */
  UPROPERTY()
  TObjectPtr<UAreaPeriodicEffectComponent> AreaPeriodicComponent;

  UPROPERTY()
  FGameplayCueParameters PeriodicVfxCueParameters;

  UPROPERTY()
  FGameplayCueParameters PeriodicSoundCueParameters;

protected:
  /** 리플리케이션된 시전 시간 */
  UPROPERTY(Replicated)
  float ClientActivationTime;
};
