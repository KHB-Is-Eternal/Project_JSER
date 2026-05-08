#include "SkillSystem/GameplayEffectComponent/VfxSfxGEC.h"

#include "AbilitySystemComponent.h"
#include "AbilitySystemGlobals.h"
#include "GameplayCueManager.h"
#include "GameplayEffect.h"
#include "GameplayPrediction.h"
#include "GameplayEffectTypes.h"
#include "SkillSystem/GameplayCueNotify/Particle/SkillNiagaraSpawnConfig.h"
#include "SkillSystem/GameplayCueNotify/Sound/SkillSoundSpawnConfig.h"

UVfxSfxGEC::UVfxSfxGEC()
{
}

bool UVfxSfxGEC::OnActiveGameplayEffectAdded(FActiveGameplayEffectsContainer& ActiveGEContainer, FActiveGameplayEffect& ActiveGE) const
{
	bool bResult = Super::OnActiveGameplayEffectAdded(ActiveGEContainer, ActiveGE);

	UAbilitySystemComponent* TargetASC = ActiveGEContainer.Owner;
	if (IsValid(TargetASC) && TargetASC->IsOwnerActorAuthoritative())
	{
		// 1. 발동 효과 실행 (Duration/Infinite GE이므로 지속성 효과로 등록)
		TArray<FGameplayTag> OngoingTags;

		auto AddOngoing = [&](const UObject* Config, FGameplayTag Tag)
		{
			if (IsValid(Config) && Tag.IsValid())
			{
				FGameplayCueParameters Params(ActiveGE.Spec);
				Params.SourceObject = const_cast<UObject*>(Config);
				Params.Instigator = ActiveGE.Spec.GetContext().GetInstigator();
				Params.EffectCauser = ActiveGE.Spec.GetContext().GetEffectCauser();

				FScopedPredictionWindow PredictionWindow(TargetASC, !TargetASC->GetPredictionKeyForNewAction().IsValidKey());
				TargetASC->AddGameplayCue(Tag, Params);
				OngoingTags.Add(Tag);
			}
		};

		if (IsValid(TriggerVfx)) AddOngoing(TriggerVfx.Get(), TriggerVfx->CueTag);
		if (IsValid(TriggerSound)) AddOngoing(TriggerSound.Get(), TriggerSound->CueTag);

		// 2. 제거 효과 바인딩 (Ongoing 중단 + Removed 효과 실행)
		if (OngoingTags.Num() > 0 || IsValid(RemovedVfx.Get()) || IsValid(RemovedSound.Get()))
		{
			TWeakObjectPtr<const UVfxSfxGEC> WeakThis(this);
			TWeakObjectPtr<UAbilitySystemComponent> WeakASC(TargetASC);
			FGameplayEffectSpec Spec = ActiveGE.Spec; // 복사본 저장

			ActiveGE.EventSet.OnEffectRemoved.AddLambda([WeakThis, WeakASC, Spec, OngoingTags](const FGameplayEffectRemovalInfo& RemovalInfo)
			{
				if (WeakASC.IsValid())
				{
					UAbilitySystemComponent* ASC = WeakASC.Get();

					// 지속성으로 등록된 발동 효과 제거
					for (const FGameplayTag& Tag : OngoingTags)
					{
						FScopedPredictionWindow PredictionWindow(ASC, !ASC->GetPredictionKeyForNewAction().IsValidKey());
						ASC->RemoveGameplayCue(Tag);
					}

					// 제거 시점의 효과 실행 (Burst)
					if (WeakThis.IsValid())
					{
						WeakThis->ExecuteEffects(ASC, Spec, WeakThis->RemovedVfx.Get(), WeakThis->RemovedSound.Get());
					}
				}
			});
		}
	}

	return bResult;
}

void UVfxSfxGEC::OnGameplayEffectExecuted(FActiveGameplayEffectsContainer& ActiveGEContainer, FGameplayEffectSpec& GESpec, FPredictionKey& PredictionKey) const
{
	Super::OnGameplayEffectExecuted(ActiveGEContainer, GESpec, PredictionKey);

	UAbilitySystemComponent* ASC = ActiveGEContainer.Owner;
	if (IsValid(ASC) && ASC->IsOwnerActorAuthoritative())
	{
		if (GESpec.GetPeriod() > 0.0f)
		{
			// 주기적 틱인 경우
			ExecuteEffects(ASC, GESpec, PeriodicVfx.Get(), PeriodicSound.Get(), PredictionKey);
		}
		else
		{
			// 즉시(Instant) 실행인 경우
			ExecuteEffects(ASC, GESpec, TriggerVfx.Get(), TriggerSound.Get(), PredictionKey);
		}
	}
}

void UVfxSfxGEC::ExecuteEffects(UAbilitySystemComponent* ASC, const FGameplayEffectSpec& GESpec, const USkillNiagaraSpawnConfig* VfxConfig, const USkillSoundSpawnConfig* SoundConfig, FPredictionKey PredictionKey) const
{
	if (!IsValid(ASC)) return;

	const FGameplayEffectContextHandle& Context = GESpec.GetContext();
	
	// 기본 위치 설정: Context에 Origin이 있으면 사용, 없으면 Target(Avatar) 위치 사용
	FVector CueLocation = Context.HasOrigin() ? Context.GetOrigin() : ASC->GetAvatarActor()->GetActorLocation();
	FVector CueDirection = FVector::UpVector;
	if (const FHitResult* Hit = Context.GetHitResult())
	{
		CueDirection = Hit->Normal;
	}

	if (!PredictionKey.IsValidKey()) PredictionKey = ASC->ScopedPredictionKey;
	UGameplayCueManager* CueManager = UAbilitySystemGlobals::Get().GetGameplayCueManager();
	if (!IsValid(CueManager)) return;

	// 1. VFX 실행
	if (IsValid(VfxConfig) && VfxConfig->CueTag.IsValid())
	{
		FGameplayCueParameters Params(GESpec);
		Params.Location = CueLocation;
		Params.Normal = CueDirection;
		Params.SourceObject = const_cast<USkillNiagaraSpawnConfig*>(VfxConfig);
		Params.Instigator = Context.GetInstigator();
		Params.EffectCauser = Context.GetEffectCauser();

		CueManager->InvokeGameplayCueExecuted_WithParams(ASC, VfxConfig->CueTag, PredictionKey, Params);
	}

	// 2. SFX 실행
	if (IsValid(SoundConfig) && SoundConfig->CueTag.IsValid())
	{
		FGameplayCueParameters Params(GESpec);
		Params.Location = CueLocation;
		Params.Normal = CueDirection;
		Params.SourceObject = const_cast<USkillSoundSpawnConfig*>(SoundConfig);
		Params.Instigator = Context.GetInstigator();
		Params.EffectCauser = Context.GetEffectCauser();

		CueManager->InvokeGameplayCueExecuted_WithParams(ASC, SoundConfig->CueTag, PredictionKey, Params);
	}
}
