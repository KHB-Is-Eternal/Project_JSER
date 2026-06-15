// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "SkillSystem/GameAbility/SkillBase.h"
#include "WatchTagAbility_Base.generated.h"

enum class EPassiveQueryTarget : uint8;
class UPassiveSkillConfig;
class UBaseGameplayEffect;

/**
 * 패시브 트리거 어빌리티의 추상 부모 클래스.
 * 특정 게임플레이 이벤트 태그를 무한 감시하고, 쿨타임 차단 및 태그 쿼리 필터링,
 * 발동 액션(이펙트 적용 / 어빌리티 실행) 등 공통 로직을 수행합니다.
 * 자식 클래스는 ProcessEventAndCheckCondition()을 구현하여 조건 달성 여부를 반환합니다.
 */
UCLASS(Abstract)
class PROJECTER_API UWatchTagAbility_Base : public USkillBase
{
	GENERATED_BODY()

public:
	UWatchTagAbility_Base();

	virtual void OnGiveAbility(const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilitySpec& Spec) override;
	virtual void OnRemoveAbility(const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilitySpec& Spec) override;
	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;

protected:
	/** 이벤트 수신 시 호출. 쿨타임 및 태그 쿼리 필터링 후 ProcessEventAndCheckCondition()을 위임합니다. */
	UFUNCTION()
	void OnEventReceived(FGameplayEventData Payload);

	/**
	 * [Pure Virtual] 자식 클래스에서 구현. 이벤트를 처리하고 발동 조건이 충족되면 true를 반환합니다.
	 * 즉발형은 항상 true, 누적형은 누적치가 임계치에 도달했을 때 true를 반환합니다.
	 */
	virtual bool ProcessEventAndCheckCondition(const FGameplayEventData& Payload, float& OutEventMagnitude) PURE_VIRTUAL(UWatchTagAbility_Base::ProcessEventAndCheckCondition, return false;);

	/** 대상 액터를 지정된 Target 타입에 따라 Payload에서 찾아 반환합니다. */
	AActor* ResolveQueryTargetActor(const FGameplayEventData& Payload, EPassiveQueryTarget TargetType) const;

	/** 지정된 모든 스탯(Attribute) 발동 조건을 만족하는지 검사합니다. */
	bool CheckAttributeConditions(const FGameplayEventData& Payload) const;

	/** 발동 조건이 충족되었을 때, TriggerAbility 실행 또는 TriggerEffects 적용을 수행합니다. */
	void ExecuteTriggerAction(AActor* TargetActor, float EventMagnitude);

	/** TargetActor의 ASC에 TriggerEffects를 적용합니다. */
	void ApplyTriggerEffects(AActor* TargetActor, float EventMagnitude);

	/** 사망 태그 변경 감지 콜백 (부활 시 패시브 재활성화를 위함) */
	UFUNCTION()
	void OnDeathTagChanged(const FGameplayTag Tag, int32 NewCount, FGameplayAbilitySpecHandle SpecHandle);

	// --- Lifecycle Hook ---
	virtual void OnEffectSpecCreated(FGameplayEffectSpecHandle& SpecHandle) const override;

protected:
	UPROPERTY(Transient, BlueprintReadOnly, Category = "Config")
	TObjectPtr<const UPassiveSkillConfig> PassiveConfig;

	/** 동적으로 부여된 트리거 어빌리티의 스펙 핸들 */
	FGameplayAbilitySpecHandle GrantedTriggerAbilityHandle;

	/** 훅(Hook)을 통해 스펙에 주입하기 위해 보관하는 이벤트 매그니튜드 값 */
	mutable float CurrentEventMagnitude = 0.0f;
};
