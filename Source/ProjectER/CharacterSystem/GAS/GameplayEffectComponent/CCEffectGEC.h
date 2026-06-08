#pragma once

#include "CoreMinimal.h"
#include "GameplayEffectComponent.h"
#include "GameplayTagContainer.h"
#include "CCEffectGEC.generated.h"

class UAbilitySystemComponent;

UCLASS()
class PROJECTER_API UCCEffectGEC : public UGameplayEffectComponent
{
	GENERATED_BODY()
	
public:
	UCCEffectGEC();
	
protected:
	virtual void OnGameplayEffectApplied(FActiveGameplayEffectsContainer& ActiveGEContainer, FGameplayEffectSpec& GESpec, FPredictionKey& PredictionKey) const override;
	
private:
	// Tenacity + DR 보정된 Duration을 GESpec에 재설정
	void AdjustDuration(FGameplayEffectSpec& GESpec, const UAbilitySystemComponent* TargetASC, AActor* TargetActor) const;
	
	// CC 부가 동작 실행 (이동 중지, 어빌리티 캔슬, 에어본)
	void ExecuteCCBehavior(AActor* TargetActor, FGameplayEffectSpec& GESpec) const;
	
	// Slow 중첩 체크: 기존 Slow보다 약하면 true 반환 (적용 차단용)
	bool ShouldBlockWeakerSlow(const UAbilitySystemComponent* TargetASC, const FGameplayEffectSpec& IncomingSpec) const;
	
	// 기존의 약한 Slow GE를 제거
	void RemoveWeakerSlowEffects(UAbilitySystemComponent* TargetASC, const FGameplayEffectSpec& IncomingSpec) const;
	
public:
	// 에어본 도달 높이 (cm) — 0이면 에어본 아님
	// LaunchCharacter 속도는 이 높이와 중력으로부터 자동 역산됨
	UPROPERTY(EditDefaultsOnly, Category = "CC|Airborne")
	float DesiredAirborneHeight = 0.0f;
	
	// Tenacity 적용 여부 (Airborne은 false로 설정)
	UPROPERTY(EditDefaultsOnly, Category = "CC")
	bool bAffectedByTenacity = true;
	
	// 현재 진행 중인 어빌리티를 캔슬할지 (Hard CC: true)
	UPROPERTY(EditDefaultsOnly, Category = "CC")
	bool bCancelCurrentAbilities = false;
	
	// 이동을 즉시 중지할지 (Root, Stun, Airborne: true)
	UPROPERTY(EditDefaultsOnly, Category = "CC")
	bool bStopMovement = false;
	
	// Slow 전용: 이 GE가 Slow CC인지 여부
	// true이면 기존 Slow보다 약한 경우 적용을 차단
	UPROPERTY(EditDefaultsOnly, Category = "CC|Slow")
	bool bIsSlowEffect = false;
	
	// CC 타입 태그 (DR 추적용) — 예: State.Debuff.Hard.Stun
	UPROPERTY(EditDefaultsOnly, Category = "CC|DR", meta = (Categories = "State.Debuff"))
	FGameplayTag CCTypeTag;
	
	// Diminishing Returns 적용 여부
	UPROPERTY(EditDefaultsOnly, Category = "CC|DR")
	bool bApplyDiminishingReturns = true;
};
