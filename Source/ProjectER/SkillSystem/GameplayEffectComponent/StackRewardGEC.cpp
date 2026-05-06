// Fill out your copyright notice in the Description page of Project Settings.

#include "StackRewardGEC.h"
#include "AbilitySystemComponent.h"
#include "GameplayEffect.h"
#include "SkillSystem/GameplayCueNotify/Particle/SkillNiagaraSpawnConfig.h"
#include "SkillSystem/GameplayCueNotify/Sound/SkillSoundSpawnConfig.h"
#include "SkillSystem/GameplayEffect/BaseGameplayEffect.h"

UStackRewardGEC::UStackRewardGEC()
{
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

    // 람다 바인딩: 스택이 변경될 때마다 호출
    ActiveGE.EventSet.OnStackChanged.AddLambda(
        [this, TargetASC](FActiveGameplayEffectHandle InHandle, int32 NewStack, int32 OldStack)
        { 
			ProcessStackRewards(TargetASC, InHandle, NewStack);
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

						{
							FScopedPredictionWindow PredictionWindow(SourceASC, !SourceASC->GetPredictionKeyForNewAction().IsValidKey());
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
						
						{
							FScopedPredictionWindow PredictionWindow(SourceASC, !SourceASC->GetPredictionKeyForNewAction().IsValidKey());
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

						{
							FScopedPredictionWindow PredictionWindow(SourceASC, !SourceASC->GetPredictionKeyForNewAction().IsValidKey());
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

						{
							FScopedPredictionWindow PredictionWindow(SourceASC, !SourceASC->GetPredictionKeyForNewAction().IsValidKey());
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