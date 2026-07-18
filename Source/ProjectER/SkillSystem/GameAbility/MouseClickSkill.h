// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "SkillSystem/GameAbility/SkillBase.h"
#include "MouseClickSkill.generated.h"

class AMouseLocationTargetActor;

/**
 * 
 */
UCLASS()
class PROJECTER_API UMouseClickSkill : public USkillBase
{
	GENERATED_BODY()
public:
	UMouseClickSkill();

	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;
	virtual bool ShouldAbilityRespondToEvent(const FGameplayAbilityActorInfo* ActorInfo, const FGameplayEventData* Payload) const override;
	bool TryGetMouseLocationInRange(FVector& OutLocation) const;
	bool ConsumePendingExternalTargetLocation(FVector& OutLocation);
	bool IsTargetLocationInRange(const FVector& InLocation) const;
	FVector GetMouseLocation() const;
	virtual void EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled) override;

protected:
	virtual void ExecuteSmartCast(const FGameplayEventData& EventData) override;
	virtual void StartIndicatorMode(bool bIsManual) override;

	virtual void ApplyExecutionEffects() override;
	virtual void OnCancelAbility() override;
	void ClearRangeIndicator();
	bool IsInRange(const FVector& Location) const;
	void RotateToLocation(const FVector& Location);
	void SetWaitTargetTask();
	void SetWaitExternalTargetEventTask();
	void SubmitExternalTargetLocation(const FVector& InLocation);
	UFUNCTION()
	void OnTargetDataReady(const FGameplayAbilityTargetDataHandle& DataHandle);
	UFUNCTION()
	void OnTargetCancelled(const FGameplayAbilityTargetDataHandle& DataHandle);
	UFUNCTION()
	void OnExternalTargetLocationReceived(FGameplayEventData Payload);
private:

public:

protected:
	TWeakObjectPtr<AMouseLocationTargetActor> CurrentMouseLocationTargetActor;
	TOptional<FVector> PendingExternalTargetLocation;
	FGameplayEffectContextHandle TargetLocationEffectContext;
	FGameplayTag ExternalTargetLocationEventTag;

	UPROPERTY()
	TWeakObjectPtr<class UGroundIndicatorComponent> ActiveRangeIndicatorComp;

private:
};
