// Fill out your copyright notice in the Description page of Project Settings.

#include "StackRewardGEC.h"
#include "SkillSystem/GameplayEffectComponent/BaseGEC.h"
#include "AbilitySystemComponent.h"
#include "GameplayEffect.h"
#include "SkillSystem/GameplayCueNotify/Particle/SkillNiagaraSpawnConfig.h"
#include "SkillSystem/GameplayCueNotify/Sound/SkillSoundSpawnConfig.h"
#include "SkillSystem/GameplayEffect/BaseGameplayEffect.h"

UStackRewardGEC::UStackRewardGEC()
{
}

FSkillTooltipData UStackRewardGEC::GetTooltipDescription(int32 Level, TSubclassOf<class USkillBase> AbilityClass) const
{
	FSkillTooltipData Data;
	Data.ShortDescription = FText::FromString(TEXT("스택에 따라 보상을 획득합니다."));

	FString DetailStr = TEXT("스택 보상 : 특정 스택 도달 시 보상 효과가 발동됩니다.");
	for (const FStackRewardInfo& Reward : Rewards)
	{
		DetailStr += FString::Printf(TEXT("\n\n[%d 스택 달성 시]"), Reward.StackCount);
		
		TArray<TSubclassOf<UBaseGameplayEffect>> RewardEffects;
		RewardEffects.Add(Reward.AppliedEffect);
		
		FText RewardText = FormatAppliedEffects(RewardEffects, Level);
		if (!RewardText.IsEmpty())
		{
			DetailStr += TEXT("\n") + RewardText.ToString();
		}
	}

	Data.DetailedDescription = FText::FromString(DetailStr);
	return Data;
}

bool UStackRewardGEC::OnActiveGameplayEffectAdded(FActiveGameplayEffectsContainer &ActiveGEContainer, FActiveGameplayEffect &ActiveGE) const
{
    bool bResult = Super::OnActiveGameplayEffectAdded(ActiveGEContainer, ActiveGE);

    // 1. Config 유효성 및 보상 목록 개수 체크
    if (this->Rewards.Num() <= 0)
    {
        return bResult;
    }

    // 2. Target ASC 유효성 체크
    UAbilitySystemComponent *TargetASC = ActiveGEContainer.Owner;
    if (!IsValid(TargetASC))
    {
        return bResult;
    }

    // AddWeakLambda를 사용하여 this(GEC) 또는 TargetASC가 소멸해도 크래시를 방지합니다.
    TWeakObjectPtr<UAbilitySystemComponent> WeakASC = TargetASC;
    ActiveGE.EventSet.OnStackChanged.AddWeakLambda(this,
        [this, WeakASC](FActiveGameplayEffectHandle InHandle, int32 NewStack, int32 OldStack)
        { 
			if (!WeakASC.IsValid()) return;
			ProcessStackRewards(WeakASC.Get(), InHandle, NewStack);
        });

    // 3. 최초 부여 시점(1스택) 체크 로그
    int32 InitialStack = ActiveGE.Spec.GetStackCount();
    ProcessStackRewards(TargetASC, ActiveGE.Handle, InitialStack);

    return bResult;
}

void UStackRewardGEC::ProcessStackRewards(UAbilitySystemComponent* TargetASC, FActiveGameplayEffectHandle InHandle, int32 CurrentStack) const
{
	for (const FStackRewardInfo& RewardInfo : this->Rewards)
	{
		if (CurrentStack == RewardInfo.StackCount)
		{
			const FActiveGameplayEffect* Effect = TargetASC->GetActiveGameplayEffect(InHandle);
			if (!Effect) continue;
			UAbilitySystemComponent* SourceASC = Effect->Spec.GetContext().GetInstigatorAbilitySystemComponent();
			if (IsValid(SourceASC))
			{
				if (IsValid(RewardInfo.AppliedEffect))
				{
					FGameplayEffectContextHandle EffectContext = Effect->Spec.GetContext();
					FGameplayEffectSpecHandle SpecHandle = SourceASC->MakeOutgoingSpec(RewardInfo.AppliedEffect, Effect->Spec.GetLevel(), EffectContext);
					UBaseGEC::InheritHitTags(Effect->Spec, SpecHandle);
					
					if (SpecHandle.IsValid())
					{
						if (RewardInfo.bApplyToTarget)
						{
							SourceASC->ApplyGameplayEffectSpecToTarget(*SpecHandle.Data.Get(), TargetASC);
						}
						else
						{
							SourceASC->ApplyGameplayEffectSpecToSelf(*SpecHandle.Data.Get());
						}
					}
				}

				{
					// --- VFX 발동 로직 ---
					// 1. 시전자(Instigator) 피드백 VFX
					if (IsValid(RewardInfo.InstigatorVfxConfig.Get()) && RewardInfo.InstigatorVfxConfig->CueTag.IsValid())
					{
						FGameplayCueParameters Params(Effect->Spec);
						Params.SourceObject = RewardInfo.InstigatorVfxConfig.Get();

						if (SourceASC->IsOwnerActorAuthoritative() || SourceASC->ScopedPredictionKey.IsLocalClientKey())
						{
							SourceASC->ExecuteGameplayCue(RewardInfo.InstigatorVfxConfig->CueTag, Params);
						}
					}
					// 2. 발동 대상(Target) 피드백 VFX
					if (IsValid(RewardInfo.TargetVfxConfig.Get()) && RewardInfo.TargetVfxConfig->CueTag.IsValid())
					{
						FGameplayCueParameters Params(Effect->Spec);
						Params.SourceObject = RewardInfo.TargetVfxConfig.Get();
						
						// 대상 액터 본체에 어태치
						if (AActor *TargetAvatar = TargetASC->GetAvatarActor())
						{
							Params.TargetAttachComponent = TargetAvatar->GetRootComponent();
						}
						
						if (SourceASC->IsOwnerActorAuthoritative() || SourceASC->ScopedPredictionKey.IsLocalClientKey())
						{
							SourceASC->ExecuteGameplayCue(RewardInfo.TargetVfxConfig->CueTag, Params);
						}
					}
				}

				{
					// --- Sound 발동 로직 ---
					if (IsValid(RewardInfo.InstigatorSoundConfig.Get()) && RewardInfo.InstigatorSoundConfig->CueTag.IsValid())
					{
						FGameplayCueParameters Params(Effect->Spec);
						Params.SourceObject = RewardInfo.InstigatorSoundConfig.Get();

						if (SourceASC->IsOwnerActorAuthoritative() || SourceASC->ScopedPredictionKey.IsLocalClientKey())
						{
							SourceASC->ExecuteGameplayCue(RewardInfo.InstigatorSoundConfig->CueTag, Params);
						}
					}

					if (IsValid(RewardInfo.TargetSoundConfig.Get()) && RewardInfo.TargetSoundConfig->CueTag.IsValid())
					{
						FGameplayCueParameters Params(Effect->Spec);
						Params.SourceObject = RewardInfo.TargetSoundConfig.Get();

						if (AActor* TargetAvatar = TargetASC->GetAvatarActor())
						{
							Params.TargetAttachComponent = TargetAvatar->GetRootComponent();
						}

						if (SourceASC->IsOwnerActorAuthoritative() || SourceASC->ScopedPredictionKey.IsLocalClientKey())
						{
							SourceASC->ExecuteGameplayCue(RewardInfo.TargetSoundConfig->CueTag, Params);
						}
					}
				}
			}
			if (RewardInfo.bClearStack)
			{
				TargetASC->RemoveActiveGameplayEffect(InHandle, -1);
			}
		}
	}
}