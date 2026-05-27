#include "SkillSystem/GameplayEffectComponent/BaseGEC.h"
#include "SkillSystem/GameplayEffectComponent/AdditionalEffectGEC.h"
#include "AbilitySystemComponent.h"
#include "GameplayEffect.h"
#include "SkillSystem/GameplayEffect/BaseGameplayEffect.h"

UBaseGEC::UBaseGEC()
{
}

void UBaseGEC::OnGameplayEffectExecuted(FActiveGameplayEffectsContainer& ActiveGEContainer, FGameplayEffectSpec& GESpec, FPredictionKey& PredictionKey) const
{
	Super::OnGameplayEffectExecuted(ActiveGEContainer, GESpec, PredictionKey);
}


void UBaseGEC::InheritHitTags(const FGameplayEffectSpec& ParentSpec, FGameplayEffectSpecHandle& ChildSpecHandle)
{
	if (!ChildSpecHandle.IsValid()) return;

	static const FGameplayTag HitBaseTag = FGameplayTag::RequestGameplayTag(FName("Event.Action.Hit"));

	// 부모 Spec의 DynamicGrantedTags에서 Event.Action.Hit 하위 태그들을 찾아 자식 Spec에 주입
	for (const FGameplayTag& Tag : ParentSpec.DynamicGrantedTags)
	{
		if (Tag.MatchesTag(HitBaseTag))
		{
			ChildSpecHandle.Data.Get()->DynamicGrantedTags.AddTag(Tag);
		}
	}
}

void UBaseGEC::GetSkillProcEffects(UAbilitySystemComponent* InstigatorASC, UGameplayAbility* InstigatorSkill, AActor* InEffectCauser, const FGameplayEffectContextHandle& CurrentContext, TArray<FGameplayEffectSpecHandle>& OutSpecs, bool bDefaultConsume, const FGameplayEffectSpec* ParentSpec)
{
	if (!IsValid(InstigatorASC) || !IsValid(InstigatorSkill))
	{
		return;
	}

	// 1. 버프 태그
	static const FGameplayTag SkillProcTag = FGameplayTag::RequestGameplayTag(FName("Skill.Data.Augments"));

	// 2. 버프 검색
	FGameplayEffectQuery Query = FGameplayEffectQuery::MakeQuery_MatchAnyOwningTags(FGameplayTagContainer(SkillProcTag));
	TArray<FActiveGameplayEffectHandle> FoundHandles = InstigatorASC->GetActiveEffects(Query);

	if (FoundHandles.Num() > 0)
	{
		const FActiveGameplayEffectHandle& Handle = FoundHandles[0];
		const FActiveGameplayEffect* ActiveGE = InstigatorASC->GetActiveGameplayEffect(Handle);
		
		if (ActiveGE)
		{
			bool bShouldConsume = bDefaultConsume;

			// 3. AdditionalEffectGEC 컴포넌트 추출
			const UAdditionalEffectGEC* ExtraGEC = ActiveGE->Spec.Def->FindComponent<UAdditionalEffectGEC>();
			if (IsValid(ExtraGEC))
			{
				// 4. 추가 효과들로부터 스펙 생성
				for (const TSubclassOf<UBaseGameplayEffect>& EffectClass : ExtraGEC->Bonus)
				{
					if (IsValid(EffectClass))
					{
						FGameplayEffectSpecHandle NewSpecHandle = InstigatorASC->MakeOutgoingSpec(EffectClass, InstigatorSkill->GetAbilityLevel(), CurrentContext);
						if (ParentSpec)
						{
							InheritHitTags(*ParentSpec, NewSpecHandle);
						}
						OutSpecs.Add(NewSpecHandle);
					}
				}

				// 5. 서버 설정(Config)에서 소모 여부 결정
				bShouldConsume = ExtraGEC->bConsumeBuff;
			}

			// 6. 버프 소모 처리
			if (bShouldConsume)
			{
				InstigatorASC->RemoveActiveGameplayEffect(Handle);
			}
		}
	}
}
