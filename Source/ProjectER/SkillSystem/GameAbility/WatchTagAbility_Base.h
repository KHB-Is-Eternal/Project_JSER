// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "SkillSystem/GameAbility/SkillBase.h"
#include "WatchTagAbility_Base.generated.h"

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
	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;

protected:
	/** 이벤트 수신 시 호출. 쿨타임 및 태그 쿼리 필터링 후 ProcessEventAndCheckCondition()을 위임합니다. */
	UFUNCTION()
	void OnEventReceived(FGameplayEventData Payload);

	/**
	 * [Pure Virtual] 자식 클래스에서 구현. 이벤트를 처리하고 발동 조건이 충족되면 true를 반환합니다.
	 * 즉발형은 항상 true, 누적형은 누적치가 임계치에 도달했을 때 true를 반환합니다.
	 */
	virtual bool ProcessEventAndCheckCondition(const FGameplayEventData& Payload) PURE_VIRTUAL(UWatchTagAbility_Base::ProcessEventAndCheckCondition, return false;);

private:
	/** 태그 쿼리 검사를 실행할 대상 액터를 PassiveConfig의 QueryTarget 설정에 따라 Payload에서 찾아 반환합니다. */
	AActor* ResolveQueryTargetActor(const FGameplayEventData& Payload) const;

	/** 발동 조건이 충족되었을 때, TriggerAbility 실행 또는 TriggerEffects 적용을 수행합니다. */
	void ExecuteTriggerAction(AActor* TargetActor);

	/** TargetActor의 ASC에 TriggerEffects를 적용합니다. */
	void ApplyTriggerEffects(AActor* TargetActor);

protected:
	UPROPERTY(Transient, BlueprintReadOnly, Category = "Config")
	TObjectPtr<const UPassiveSkillConfig> PassiveConfig;
};
