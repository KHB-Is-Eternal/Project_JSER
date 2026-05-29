#include "SkillSystem/GameplayEffectComponent/AdditionalEffectGEC.h"

#include "SkillSystem/GameplayCueNotify/Particle/SkillNiagaraSpawnConfig.h"
#include "SkillSystem/GameplayCueNotify/Sound/SkillSoundSpawnConfig.h"
#include "AbilitySystemComponent.h"
#include "GameplayEffect.h"

//////////////////////////////////////////////////////////////////////////
// UAdditionalEffectGEC

UAdditionalEffectGEC::UAdditionalEffectGEC()
{
}

bool UAdditionalEffectGEC::OnActiveGameplayEffectAdded(FActiveGameplayEffectsContainer& ActiveGEContainer, FActiveGameplayEffect& ActiveGE) const
{
	bool bResult = Super::OnActiveGameplayEffectAdded(ActiveGEContainer, ActiveGE);

	// 0. 다이나믹 에셋 태그 설정 (보너스 효과 식별용)
	static const FGameplayTag SkillProcTag = FGameplayTag::RequestGameplayTag(FName("Skill.Data.Augments"));
	ActiveGE.Spec.AddDynamicAssetTag(SkillProcTag);

	UAbilitySystemComponent* TargetASC = ActiveGEContainer.Owner;
	if (IsValid(TargetASC))
	{
		// 1. Niagara VFX 처리
		if (IsValid(this->ActiveVfxConfig.Get()) && this->ActiveVfxConfig->CueTag.IsValid())
		{
			FGameplayCueParameters Params(ActiveGE.Spec);
			Params.SourceObject = this->ActiveVfxConfig.Get();
			Params.Instigator = ActiveGE.Spec.GetContext().GetInstigator();
			Params.EffectCauser = ActiveGE.Spec.GetContext().GetEffectCauser();

			{
				FScopedPredictionWindow PredictionWindow(TargetASC, !TargetASC->GetPredictionKeyForNewAction().IsValidKey());
				TargetASC->AddGameplayCue(this->ActiveVfxConfig->CueTag, Params);
			}

				FGameplayTag CueTag = this->ActiveVfxConfig->CueTag;
				ActiveGE.EventSet.OnEffectRemoved.AddLambda([TargetASC, CueTag](const FGameplayEffectRemovalInfo& RemovalInfo)
				{
					if (IsValid(TargetASC))
					{
						FScopedPredictionWindow PredictionWindow(TargetASC, !TargetASC->GetPredictionKeyForNewAction().IsValidKey());
						TargetASC->RemoveGameplayCue(CueTag);
					}
				});
			}

		// 2. Sound 처리
		if (IsValid(this->ActiveSoundConfig.Get()) && this->ActiveSoundConfig->CueTag.IsValid())
		{
			FGameplayCueParameters Params(ActiveGE.Spec);
			Params.SourceObject = this->ActiveSoundConfig.Get();
			Params.Instigator = ActiveGE.Spec.GetContext().GetInstigator();
			Params.EffectCauser = ActiveGE.Spec.GetContext().GetEffectCauser();

			{
				FScopedPredictionWindow PredictionWindow(TargetASC, !TargetASC->GetPredictionKeyForNewAction().IsValidKey());
				TargetASC->AddGameplayCue(this->ActiveSoundConfig->CueTag, Params);
			}

			FGameplayTag CueTag = this->ActiveSoundConfig->CueTag;
				ActiveGE.EventSet.OnEffectRemoved.AddLambda([TargetASC, CueTag](const FGameplayEffectRemovalInfo& RemovalInfo)
				{
					if (IsValid(TargetASC))
					{
						FScopedPredictionWindow PredictionWindow(TargetASC, !TargetASC->GetPredictionKeyForNewAction().IsValidKey());
						TargetASC->RemoveGameplayCue(CueTag);
					}
				});
		}
	}

	return bResult;
}

FSkillTooltipData UAdditionalEffectGEC::GetTooltipDescription(int32 Level, TSubclassOf<class USkillBase> AbilityClass) const
{
	FSkillTooltipData Data;
	Data.ShortDescription = FText::FromString(TEXT("추가 효과를 준비합니다."));

	FString DetailStr = TEXT("추가 효과 : 버프 활성화 중 다음 적중 시 추가 효과를 적용합니다.");
	FText BonusText = FormatAppliedEffects(Bonus, Level);
	if (!BonusText.IsEmpty())
	{
		DetailStr += TEXT("\n") + BonusText.ToString();
	}

	Data.DetailedDescription = FText::FromString(DetailStr);
	return Data;
}
