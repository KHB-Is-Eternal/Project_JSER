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
	bool IsInRange(const FVector& Location) const;
	FVector GetMouseLocation() const;
	virtual void EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled) override;

protected:
	virtual void ExecuteSmartCast(const FGameplayEventData& EventData) override;
	virtual void StartIndicatorMode(bool bIsManual) override;

	virtual void ExecuteSkill() override;
	virtual void CompleteFinishSkill() override;
	virtual void OnCancelAbility() override;

	virtual TSubclassOf<class AGameplayAbilityTargetActor> GetTargetActorClass() const override;

	virtual void ApplyExecutionEffects() override;
	void RotateToLocation(const FVector& Location);
	void SetWaitExternalTargetEventTask();
	void SubmitExternalTargetLocation(const FVector& InLocation);
	void CleanUpSkill();

	virtual void OnTargetDataReady(const FGameplayAbilityTargetDataHandle& DataHandle) override;
	virtual void OnTargetCancelled(const FGameplayAbilityTargetDataHandle& DataHandle) override;
	UFUNCTION()
	void OnExternalTargetLocationReceived(FGameplayEventData Payload);
private:

public:

protected:
	TWeakObjectPtr<AActor> AffectedActor;
	FVector MouseLocation;
	TOptional<FVector> PendingExternalTargetLocation;
	FGameplayEffectContextHandle TargetLocationEffectContext;
	FGameplayTag ExternalTargetLocationEventTag;

private:
};
