// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "SkillSystem/GameAbility/SkillBase.h"
#include "MouseTargetSkill.generated.h"

/**
 * 
 */

class ATargetActor;
class UBaseGameplayEffect;

UCLASS()
class PROJECTER_API UMouseTargetSkill : public USkillBase
{
	GENERATED_BODY()
public:
	UMouseTargetSkill();

	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;
	virtual bool ShouldAbilityRespondToEvent(const FGameplayAbilityActorInfo* ActorInfo, const FGameplayEventData* Payload) const override;
	AActor* GetTargetUnderCursorInRange();
	bool IsTargetActorInRange(AActor* InTargetActor) const;
protected:
	virtual void ExecuteSmartCast(const FGameplayEventData& EventData) override;
	virtual void StartIndicatorMode(bool bIsManual) override;

	virtual void ExecuteSkill() override;
	virtual void CompleteFinishSkill() override;
	virtual void OnCancelAbility() override;
	void SetWaitTargetTask();
	void SetWaitExternalTargetEventTask();
	void SubmitExternalTargetActor(AActor* InTargetActor);
	bool ConsumePendingExternalTargetActor(AActor*& OutTargetActor);
	AActor* GetTargetUnderCursor();
	bool IsInRange(AActor* Actor) const;
	void RotateToTarget(AActor* Actor);
	void ApplyEffectsTarget(AActor* TargetActor, const TArray<TSubclassOf<UBaseGameplayEffect>>& SkillEffectDataAssets, const TArray<FSkillMagnitudeCalculation>& Calculators);
	void CleanUpSkill();
	void ClearRangeIndicator();

	UFUNCTION()
	void OnTargetDataReady(const FGameplayAbilityTargetDataHandle& DataHandle);
	UFUNCTION()
	void OnTargetCancelled(const FGameplayAbilityTargetDataHandle& DataHandle);
	UFUNCTION()
	void OnExternalTargetActorReceived(FGameplayEventData Payload);
	virtual void EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled) override;

private:

public:

protected:
	TWeakObjectPtr<AActor> AffectedActor;
	TWeakObjectPtr<ATargetActor> CurrentTargetActor;
	TWeakObjectPtr<AActor> PendingExternalTargetActor;
	FGameplayTag ExternalTargetActorEventTag;

	UPROPERTY()
	TWeakObjectPtr<class UGroundIndicatorComponent> ActiveRangeIndicatorComp;

private:
};
