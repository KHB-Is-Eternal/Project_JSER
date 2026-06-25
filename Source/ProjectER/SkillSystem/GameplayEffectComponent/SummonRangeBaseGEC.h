// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "SkillSystem/GameplayEffectComponent/BaseGEC.h"
#include "GameplayTagContainer.h"
#include "UObject/Object.h"
#include "SummonRangeBaseGEC.generated.h"

/**
 * 
 */
class ABaseRangeOverlapEffectActor;
class UBaseGameplayEffect;
class USkillNiagaraSpawnConfig;
class USkillSoundSpawnConfig;
struct FGameplayTag;
struct FGameplayEffectSpec;
struct FGameplayCueParameters;
struct FGameplayEffectContextHandle;
struct FActiveGameplayEffectsContainer;
struct FPredictionKey;


UCLASS(Abstract, DontCollapseCategories)
class PROJECTER_API USummonRangeBaseGEC : public UBaseGEC
{
	GENERATED_BODY()

public:
	virtual FSkillTooltipData GetTooltipDescription(int32 Level, TSubclassOf<class USkillBase> AbilityClass) const override;


	/** Phase 1: 준비 - 소환 위치를 계산하여 Context에 기록합니다. */
	virtual void PreApplyEffect(UAbilitySystemComponent* ASC, const FGameplayEffectContextHandle& ContextHandle, const FGameplayEffectSpec& GESpec) const override;

	/** Phase 2: 비주얼 실행 - 기록된 위치에서 즉시 로컬 이펙트를 실행합니다. */
	virtual void OnExecutePredictive(UAbilitySystemComponent* ASC, const FGameplayEffectContextHandle& ContextHandle, const FGameplayEffectSpec& GESpec) const override;

	/** Phase 2.5: VFX 브로드캐스트 - 서버에서 관전자들에게 VFX를 전송합니다. */
	virtual void OnExecuteVFXCue(UAbilitySystemComponent* ASC, const FGameplayEffectContextHandle& ContextHandle, const FGameplayEffectSpec& GESpec, FPredictionKey PredictionKey = FPredictionKey()) const override;

	/** GCN 액터 초기화 데이터 제공 */
	virtual class USkillNiagaraSpawnConfig* GetAGCN_NiagaraConfig() const override { return RangeSpawnVfx.Get(); }
	virtual class USkillSoundSpawnConfig* GetAGCN_SoundConfig() const override { return RangeSpawnSound.Get(); }

protected:
	virtual void OnGameplayEffectApplied(FActiveGameplayEffectsContainer& ActiveGEContainer, FGameplayEffectSpec& GESpec, FPredictionKey& PredictionKey) const override;
	virtual FTransform CalculateSpawnTransform(const FGameplayEffectSpec& GESpec, const AActor* Instigator, const AActor* TargetActor) const;
	virtual FTransform CalculateOriginTransform(const FGameplayEffectSpec& GESpec, const AActor* Instigator, const AActor* TargetActor) const;


	virtual AActor* GetTargetActorFromContainer(FActiveGameplayEffectsContainer& ActiveGEContainer) const;

	/** OnGameplayEffectApplied 세분화 함수들 */
	virtual FTransform GetInitialTransform(const FGameplayEffectContextHandle& ContextHandle, FActiveGameplayEffectsContainer& ActiveGEContainer, const FGameplayEffectSpec& GESpec, AActor* Instigator) const;
	virtual ABaseRangeOverlapEffectActor* SpawnDeferredActor(UWorld* World, TSubclassOf<ABaseRangeOverlapEffectActor> ActorClass, const FTransform& Transform, AActor* Instigator) const;
	virtual void InitializeActorData(ABaseRangeOverlapEffectActor* Actor, const FGameplayEffectContextHandle& ContextHandle, const FGameplayEffectSpec& GESpec, const FTransform& Transform) const;
	virtual void ApplyLagCompensation(ABaseRangeOverlapEffectActor* Actor, const FGameplayEffectContextHandle& ContextHandle) const;

	FGameplayCueParameters BuildNiagaraCueParameters(const FGameplayEffectSpec& GESpec, const FGameplayTag& OriginalTag, const FGameplayEffectContextHandle& EffectContext, AActor* EffectCauser, const FVector& CueLocation, const UObject* SourceObject, const FVector& CueNormal = FVector::UpVector) const;
	virtual void InitializeRangeActor(ABaseRangeOverlapEffectActor* RangeActor, AActor* Instigator, const FGameplayEffectContextHandle& Context, const FGameplayCueParameters& HitTargetVfxCueParameters, const FGameplayCueParameters& HitTargetSoundCueParameters, const FGameplayEffectSpec& ParentSpec) const;
	virtual void SnapLocationToGround(FVector& InOutLocation, const AActor* Instigator) const;
	virtual void ApplyCommonSpawnOptions(FVector& InOutLocation, FRotator& InOutRotation, const AActor* Instigator) const;
	virtual FTransform ApplyCommonSpawnOptionsToTransform(const FTransform& InOriginTransform, const AActor* Instigator) const;

public:
	UPROPERTY(EditDefaultsOnly, Category = "Summon|Base")
	TSubclassOf<ABaseRangeOverlapEffectActor> RangeActorClass;

	UPROPERTY(EditDefaultsOnly, Category = "Summon|Base")
	float LifeSpan = 1.0f;

	UPROPERTY(EditDefaultsOnly, Category = "Summon|Base")
	FVector CollisionRadius = FVector(100.0f);

	UPROPERTY(EditDefaultsOnly, Category = "Summon|Base")
	FVector LocationOffset = FVector::ZeroVector;

	UPROPERTY(EditDefaultsOnly, Category = "Summon|Rotation")
	FRotator RotationOffset = FRotator::ZeroRotator;

	UPROPERTY(EditDefaultsOnly, Category = "Summon|Snap")
	bool bSnapToGround = true;

	UPROPERTY(EditDefaultsOnly, Category = "Summon|Snap", meta = (EditCondition = "bSnapToGround"))
	float FloatingHeight = 2.0f;

	UPROPERTY(EditDefaultsOnly, Category = "Summon|Snap", meta = (EditCondition = "bSnapToGround"))
	bool bUseBoxExtentOffset = true;

	UPROPERTY(EditDefaultsOnly, Category = "Summon|Snap", meta = (EditCondition = "bSnapToGround"))
	TEnumAsByte<ECollisionChannel> GroundTraceChannel = ECC_GameTraceChannel9;

	UPROPERTY(EditDefaultsOnly, Category = "Summon|Effect")
	bool bHitOncePerTarget = true;

	UPROPERTY(EditDefaultsOnly, Instanced, Category = "Summon|VFX")
	TObjectPtr<USkillNiagaraSpawnConfig> RangeSpawnVfx;

	UPROPERTY(EditDefaultsOnly, Instanced, Category = "Summon|VFX")
	TObjectPtr<USkillNiagaraSpawnConfig> HitTargetVfx;

	UPROPERTY(EditDefaultsOnly, Instanced, Category = "Summon|SFX")
	TObjectPtr<USkillSoundSpawnConfig> RangeSpawnSound;

	UPROPERTY(EditDefaultsOnly, Instanced, Category = "Summon|SFX")
	TObjectPtr<USkillSoundSpawnConfig> HitTargetSound;

	UPROPERTY(EditDefaultsOnly, Category = "Summon|Effect")
	TArray<TSubclassOf<UBaseGameplayEffect>> Applied;

public:
	virtual void CollectNiagaraPaths(TArray<FSoftObjectPath>& OutPaths) const override;
};
