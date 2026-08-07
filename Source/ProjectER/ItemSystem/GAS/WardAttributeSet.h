#pragma once

#include "CoreMinimal.h"
#include "CharacterSystem/GAS/AttributeSet/BaseAttributeSet.h"
#include "WardAttributeSet.generated.h"

// 평타(Ability.Action.AutoAttack) 피격 시 서버에서 브로드캐스트. 와드가 구독해 히트 카운트를 감소시킨다.
DECLARE_MULTICAST_DELEGATE(FOnWardAutoAttackHit);

/**
 * 와드 전용 어트리뷰트셋.
 * UBaseAttributeSet를 상속하되, IncomingDamage 수신 시 캐릭터용 다운/사망/XP 로직(Super)은 타지 않고
 * "평타 여부"만 판정해 히트를 통지한다. 스킬 등 비평타 데미지는 무시(값을 0으로 리셋만).
 */
UCLASS()
class PROJECTER_API UWardAttributeSet : public UBaseAttributeSet
{
	GENERATED_BODY()

public:
	// 서버에서 평타 피격 시 브로드캐스트
	FOnWardAutoAttackHit OnWardAutoAttackHit;

	virtual void PostGameplayEffectExecute(const FGameplayEffectModCallbackData& Data) override;
};
