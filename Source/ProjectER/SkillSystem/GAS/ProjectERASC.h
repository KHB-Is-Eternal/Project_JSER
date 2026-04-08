#pragma once

#include "CoreMinimal.h"
#include "AbilitySystemComponent.h"
#include "ProjectERASC.generated.h"

/**
 * ProjectER 전용 Ability System Component
 * GameplayCue 실행 시 SourceObject를 자동 주입하는 기능을 제공합니다.
 */
UCLASS()
class PROJECTER_API UProjectERASC : public UAbilitySystemComponent
{
	GENERATED_BODY()

public:
	UProjectERASC();

	// IAbilitySystemReplicationProxyInterface 머서드 오버라이드 (UE 5.7 GameplayCue 가로채기 지점)
	virtual void Call_InvokeGameplayCueExecuted_FromSpec(const FGameplayEffectSpecForRPC Spec, FPredictionKey PredictionKey) override;
	virtual void Call_InvokeGameplayCueExecuted_WithParams(const FGameplayTag GameplayCueTag, FPredictionKey PredictionKey, FGameplayCueParameters GameplayCueParameters) override;
	virtual void Call_InvokeGameplayCueAdded_WithParams(const FGameplayTag GameplayCueTag, FPredictionKey PredictionKey, FGameplayCueParameters Parameters) override;
	virtual void Call_InvokeGameplayCueAddedAndWhileActive_WithParams(const FGameplayTag GameplayCueTag, FPredictionKey PredictionKey, FGameplayCueParameters GameplayCueParameters) override;

protected:
	/** CueTag에 맞는 Config 객체를 BaseGE의 GEC들로부터 찾아 SourceObject에 주입합니다. 리플렉션 없이 안전하게 접근합니다. */
	void InjectConfigsIntoParameters(const struct FGameplayTag& CueTag, FGameplayCueParameters& Parameters, const class UBaseGameplayEffect* BaseGE);
};
