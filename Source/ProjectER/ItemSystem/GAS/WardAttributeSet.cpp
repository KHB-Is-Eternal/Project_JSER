#include "ItemSystem/GAS/WardAttributeSet.h"
#include "CharacterSystem/GameplayTags/GameplayTags.h"
#include "GameplayEffectExtension.h"

void UWardAttributeSet::PostGameplayEffectExecute(const FGameplayEffectModCallbackData& Data)
{
	// 와드는 IncomingDamage만 처리한다. 캐릭터용 Super 로직(다운/사망/XP)은 호출하지 않는다.
	if (Data.EvaluatedData.Attribute != GetIncomingDamageAttribute())
	{
		return;
	}

	// 메타 어트리뷰트 리셋(누적 방지)
	SetIncomingDamage(0.0f);

	// 평타일 때만 히트로 카운트. 스킬 등 비평타는 무시.
	// 평타 마커는 두 가지가 쓰인다(BaseAttributeSet과 동일 판정):
	//  - 스펙 부여 태그 Event.Action.Hit.BasicAttack (근거리 평타가 AddGrantedTag로 부착)
	//  - 어빌리티 소스 태그 Ability.Action.AutoAttack
	FGameplayTagContainer CombinedTags = Data.EffectSpec.CapturedSourceTags.GetSpecTags();
	CombinedTags.AppendTags(Data.EffectSpec.DynamicGrantedTags);

	static const FGameplayTag BasicAttackTag = FGameplayTag::RequestGameplayTag(FName("Event.Action.Hit.BasicAttack"));
	const bool bIsBasicAttack = CombinedTags.HasTag(BasicAttackTag)
		|| CombinedTags.HasTag(ProjectER::Ability::Action::AutoAttack);

	if (bIsBasicAttack)
	{
		OnWardAutoAttackHit.Broadcast();
	}
}
