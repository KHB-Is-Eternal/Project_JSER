// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "SkillSystem/GameplayEffectComponent/BaseGEC.h"
#include "GameplayTagContainer.h"
#include "LaunchHomingMissile.generated.h"

class ABaseMissileActor;
class UBaseGameplayEffect;
class USkillNiagaraSpawnConfig;
class USkillSoundSpawnConfig;
struct FGameplayEffectSpec;
struct FGameplayEffectContextHandle;
struct FActiveGameplayEffectsContainer;
struct FPredictionKey;



/**
 * 유도 미사일을 발사하는 GameplayEffectComponent.
 * UBaseGEC를 직접 상속하여 SummonRange 계열 종속성을 제거합니다.
 */
UCLASS()
class PROJECTER_API ULaunchHomingMissile : public UBaseGEC
{
	GENERATED_BODY()

public:
	ULaunchHomingMissile();



	/** Phase 1: 준비 - 렉 보정 및 소환 위치를 계산하여 Context에 기록합니다. */
	virtual void PreApplyEffect(UAbilitySystemComponent* ASC, const FGameplayEffectContextHandle& ContextHandle, const FGameplayEffectSpec& GESpec) const override;

	/** Phase 2: 비주얼 실행 - 기록된 위치에서 즉시 로컬 이펙트를 실행합니다. */
	virtual void OnExecutePredictive(UAbilitySystemComponent* ASC, const FGameplayEffectContextHandle& ContextHandle, const FGameplayEffectSpec& GESpec) const override;

	/** Phase 2.5: VFX 브로드캐스트 - 서버에서 관전자들에게 VFX를 전송합니다. */
	virtual void OnExecuteVFXCue(UAbilitySystemComponent* ASC, const FGameplayEffectContextHandle& ContextHandle, const FGameplayEffectSpec& GESpec) const override;

protected:
	virtual void OnGameplayEffectApplied(FActiveGameplayEffectsContainer& ActiveGEContainer, FGameplayEffectSpec& GESpec, FPredictionKey& PredictionKey) const override;

	FTransform CalculateSpawnTransform(const AActor* Instigator, const AActor* TargetActor) const;
	AActor* GetTargetActorFromContainer(FActiveGameplayEffectsContainer& ActiveGEContainer) const;
	void ExecuteVfx(const FGameplayEffectSpec& GESpec, const FGameplayEffectContextHandle& ContextHandle, AActor* Instigator, ABaseMissileActor* MissileActor) const;
	void ExecuteSound(const FGameplayEffectSpec& GESpec, const FGameplayEffectContextHandle& ContextHandle, AActor* Instigator, ABaseMissileActor* MissileActor) const;

public:
	//--- 미사일 액터 클래스 ---
	UPROPERTY(EditDefaultsOnly, Category = "Missile|Base")
	TSubclassOf<ABaseMissileActor> MissileActorClass;

	//--- Movement ---
	UPROPERTY(EditDefaultsOnly, Category = "Missile|Movement")
	float InitialSpeed = 1500.0f;

	UPROPERTY(EditDefaultsOnly, Category = "Missile|Movement")
	float MaxSpeed = 2000.0f;

	//--- Homing ---
	UPROPERTY(EditDefaultsOnly, Category = "Missile|Homing")
	float HomingAccelerationMagnitude = 30000.0f;

	UPROPERTY(EditDefaultsOnly, Category = "Missile|Homing", meta = (ToolTip = "이 거리 이내로 접근하면 적중으로 판정"))
	float ReachThreshold = 50.0f;

	//--- Gameplay ---
	UPROPERTY(EditDefaultsOnly, Category = "Missile|Gameplay")
	float LifeSpan = 5.0f;

	UPROPERTY(EditDefaultsOnly, Category = "Missile|Gameplay")
	bool bDestroyOnHit = true;

	UPROPERTY(EditDefaultsOnly, Category = "Missile|Gameplay")
	FName BoneName;

	UPROPERTY(EditDefaultsOnly, Category = "Missile|Effect")
	TArray<TSubclassOf<UBaseGameplayEffect>> Applied;

	//--- Niagara VFX ---
	UPROPERTY(EditDefaultsOnly, Instanced, Category = "Missile|Niagara")
	TObjectPtr<USkillNiagaraSpawnConfig> MissileVfx;

	UPROPERTY(EditDefaultsOnly, Instanced, Category = "Missile|Niagara")
	TObjectPtr<USkillNiagaraSpawnConfig> ImpactVfx;

	//--- Sound ---
	UPROPERTY(EditDefaultsOnly, Instanced, Category = "Missile|Sound")
	TObjectPtr<USkillSoundSpawnConfig> MissileSound;

	UPROPERTY(EditDefaultsOnly, Instanced, Category = "Missile|Sound")
	TObjectPtr<USkillSoundSpawnConfig> ImpactSound;
};
