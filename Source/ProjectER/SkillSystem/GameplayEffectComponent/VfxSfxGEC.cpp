#include "SkillSystem/GameplayEffectComponent/VfxSfxGEC.h"

#include "AbilitySystemComponent.h"
#include "AbilitySystemGlobals.h"
#include "GameplayCueManager.h"
#include "GameplayEffect.h"
#include "GameplayPrediction.h"
#include "GameplayEffectTypes.h"
#include "SkillSystem/GameplayCueNotify/Particle/SkillNiagaraSpawnConfig.h"
#include "SkillSystem/GameplayCueNotify/Sound/SkillSoundSpawnConfig.h"
#include "CharacterSystem/GAS/ProjectERASC.h"

UVfxSfxGEC::UVfxSfxGEC()
{
}

bool UVfxSfxGEC::OnActiveGameplayEffectAdded(FActiveGameplayEffectsContainer& ActiveGEContainer, FActiveGameplayEffect& ActiveGE) const
{
	bool bResult = Super::OnActiveGameplayEffectAdded(ActiveGEContainer, ActiveGE);

	UAbilitySystemComponent* TargetASC = ActiveGEContainer.Owner;
	if (IsValid(TargetASC))
	{
		// 1. Î∞úÎèô ?®Í≥º ?§Ìñâ (Duration/Infinite GE?¥Î?Î°?ÏßÄ?çÏÑ± ?®Í≥ºÎ°??±Î°ù)
		TArray<TPair<FGameplayTag, TWeakObjectPtr<const UObject>>> OngoingCues;

		auto AddOngoing = [&](const UObject* Config, FGameplayTag Tag)
		{
			if (IsValid(Config) && Tag.IsValid())
			{
				FGameplayCueParameters Params(ActiveGE.Spec);
				Params.SourceObject = const_cast<UObject*>(Config);
				Params.Instigator = ActiveGE.Spec.GetContext().GetInstigator();
				Params.EffectCauser = ActiveGE.Spec.GetContext().GetEffectCauser();

				if (TargetASC->IsOwnerActorAuthoritative() || TargetASC->ScopedPredictionKey.IsLocalClientKey())
				TargetASC->AddGameplayCue(Tag, Params);
				OngoingCues.Add(TPair<FGameplayTag, TWeakObjectPtr<const UObject>>(Tag, Config));
			}
		};

		if (IsValid(TriggerVfx)) AddOngoing(TriggerVfx.Get(), TriggerVfx->CueTag);
		if (IsValid(TriggerSound)) AddOngoing(TriggerSound.Get(), TriggerSound->CueTag);

		// 1.1 ÏµúÏ¥à ?ÅÏö© ?úÏ†ê ?§ÌÉù ?òÏπòÍ∞Ä ÏµúÎ? ?§ÌÉù?∏Ï? Í≤Ä??Î∞??ÅÌÉú ?Ä??
		int32 InitialStack = ActiveGE.Spec.GetStackCount();
		int32 MaxStack = ActiveGE.Spec.Def ? ActiveGE.Spec.Def->GetStackLimitCount() : 0;
		
		TSharedPtr<bool> bMaxStackCueActive = MakeShared<bool>(false);

		if (MaxStack > 0 && InitialStack >= MaxStack)
		{
			if (IsValid(MaxStackVfx.Get()) && MaxStackVfx->CueTag.IsValid())
			{
				FGameplayCueParameters Params(ActiveGE.Spec);
				Params.SourceObject = MaxStackVfx.Get();
				Params.Instigator = ActiveGE.Spec.GetContext().GetInstigator();
				Params.EffectCauser = ActiveGE.Spec.GetContext().GetEffectCauser();

				if (TargetASC->IsOwnerActorAuthoritative() || TargetASC->ScopedPredictionKey.IsLocalClientKey())
				TargetASC->AddGameplayCue(MaxStackVfx->CueTag, Params);
				*bMaxStackCueActive = true;
			}
			if (IsValid(MaxStackSound.Get()) && MaxStackSound->CueTag.IsValid())
			{
				FGameplayCueParameters Params(ActiveGE.Spec);
				Params.SourceObject = MaxStackSound.Get();
				Params.Instigator = ActiveGE.Spec.GetContext().GetInstigator();
				Params.EffectCauser = ActiveGE.Spec.GetContext().GetEffectCauser();

				if (TargetASC->IsOwnerActorAuthoritative() || TargetASC->ScopedPredictionKey.IsLocalClientKey())
				TargetASC->AddGameplayCue(MaxStackSound->CueTag, Params);
			}
		}

		// 2. ?úÍ±∞ ?®Í≥º Î∞îÏù∏??(Ongoing Ï§ëÎã® + Removed ?®Í≥º ?§Ìñâ)
		if (OngoingCues.Num() > 0 || IsValid(RemovedVfx.Get()) || IsValid(RemovedSound.Get()) || IsValid(MaxStackVfx.Get()) || IsValid(MaxStackSound.Get()))
		{
			TWeakObjectPtr<const UVfxSfxGEC> WeakThis(this);
			TWeakObjectPtr<UAbilitySystemComponent> WeakASC(TargetASC);
			FGameplayEffectSpec Spec = ActiveGE.Spec; // Î≥µÏÇ¨Î≥??Ä??

			ActiveGE.EventSet.OnEffectRemoved.AddLambda([WeakThis, WeakASC, Spec, OngoingCues, bMaxStackCueActive](const FGameplayEffectRemovalInfo& RemovalInfo)
			{
				if (WeakASC.IsValid())
				{
					UProjectERASC* CustomASC = Cast<UProjectERASC>(WeakASC.Get());
					ensureMsgf(CustomASC != nullptr, TEXT("OnEffectRemoved: ASC is not UProjectERASC! Check Blueprint CDO or reparent the BP."));
					
					if (CustomASC)
					{
						// ÏßÄ?çÏÑ±?ºÎ°ú ?±Î°ù??Î∞úÎèô ?®Í≥º ?úÍ±∞
						for (const auto& CuePair : OngoingCues)
						{
							if (CustomASC->IsOwnerActorAuthoritative() || CustomASC->ScopedPredictionKey.IsLocalClientKey())
							CustomASC->RemoveGameplayCueBySource(CuePair.Key, CuePair.Value.Get());
						}

						// ÏµúÎ? ?§ÌÉù ?®Í≥º ?úÍ±∞
						if (*bMaxStackCueActive && WeakThis.IsValid())
						{
							if (WeakThis->MaxStackVfx.Get() && WeakThis->MaxStackVfx->CueTag.IsValid())
							{
								if (CustomASC->IsOwnerActorAuthoritative() || CustomASC->ScopedPredictionKey.IsLocalClientKey())
								CustomASC->RemoveGameplayCueBySource(WeakThis->MaxStackVfx->CueTag, WeakThis->MaxStackVfx.Get());
							}
							if (WeakThis->MaxStackSound.Get() && WeakThis->MaxStackSound->CueTag.IsValid())
							{
								if (CustomASC->IsOwnerActorAuthoritative() || CustomASC->ScopedPredictionKey.IsLocalClientKey())
								CustomASC->RemoveGameplayCueBySource(WeakThis->MaxStackSound->CueTag, WeakThis->MaxStackSound.Get());
							}
							*bMaxStackCueActive = false;
						}
					}

					// ?úÍ±∞ ?úÏ†ê???®Í≥º ?§Ìñâ (Burst)
					if (WeakThis.IsValid())
					{
						WeakThis->ExecuteEffects(WeakASC.Get(), Spec, WeakThis->RemovedVfx.Get(), WeakThis->RemovedSound.Get());
					}
				}
			});
		}

		// 3. ?§ÌÉù Î≥ÄÍ≤???Î∞úÎèô ?®Í≥º ?¨Ïã§??(?§ÌÉù Ï¶ùÍ? Î∞?ÏµúÎ? ?§ÌÉù ?ÑÎã¨ ?∞Ï∂ú)
		if (IsValid(TriggerVfx.Get()) || IsValid(TriggerSound.Get()) || IsValid(MaxStackVfx.Get()) || IsValid(MaxStackSound.Get()))
		{
			TWeakObjectPtr<const UVfxSfxGEC> WeakThis(this);
			TWeakObjectPtr<UAbilitySystemComponent> WeakASC(TargetASC);
			ActiveGE.EventSet.OnStackChanged.AddLambda([WeakThis, WeakASC, bMaxStackCueActive](FActiveGameplayEffectHandle InHandle, int32 NewStack, int32 OldStack)
			{
				if (WeakThis.IsValid() && WeakASC.IsValid())
				{
					UAbilitySystemComponent* ASC = WeakASC.Get();
					const FActiveGameplayEffect* Effect = ASC->GetActiveGameplayEffect(InHandle);
					if (Effect)
					{
						int32 LimitCount = Effect->Spec.Def ? Effect->Spec.Def->GetStackLimitCount() : 0;

						if (NewStack > OldStack)
						{
							// ?§ÌÉù Ï¶ùÍ? ?¥Ìéô???§Ìñâ
							WeakThis->ExecuteEffects(ASC, Effect->Spec, WeakThis->TriggerVfx.Get(), WeakThis->TriggerSound.Get());

							// ÏµúÎ? ?§ÌÉù ?ÑÎã¨ ???¥Ìéô??(ÏµúÏ¥à ?ÑÎã¨ ?úÍ∞Ñ?êÎßå ?§Ìñâ)
							if (LimitCount > 0 && NewStack >= LimitCount && !(*bMaxStackCueActive))
							{
								if (WeakThis->MaxStackVfx.Get() && WeakThis->MaxStackVfx->CueTag.IsValid())
								{
									FGameplayCueParameters Params(Effect->Spec);
									Params.SourceObject = WeakThis->MaxStackVfx.Get();
									Params.Instigator = Effect->Spec.GetContext().GetInstigator();
									Params.EffectCauser = Effect->Spec.GetContext().GetEffectCauser();

									if (ASC->IsOwnerActorAuthoritative() || ASC->ScopedPredictionKey.IsLocalClientKey())
									ASC->AddGameplayCue(WeakThis->MaxStackVfx->CueTag, Params);
								}
								if (WeakThis->MaxStackSound.Get() && WeakThis->MaxStackSound->CueTag.IsValid())
								{
									FGameplayCueParameters Params(Effect->Spec);
									Params.SourceObject = WeakThis->MaxStackSound.Get();
									Params.Instigator = Effect->Spec.GetContext().GetInstigator();
									Params.EffectCauser = Effect->Spec.GetContext().GetEffectCauser();

									if (ASC->IsOwnerActorAuthoritative() || ASC->ScopedPredictionKey.IsLocalClientKey())
									ASC->AddGameplayCue(WeakThis->MaxStackSound->CueTag, Params);
								}
								*bMaxStackCueActive = true;
							}
						}
						else if (NewStack < OldStack)
						{
							// ?§ÌÉù Í∞êÏÜå ??ÏµúÎ? ?§ÌÉù ÎØ∏Îßå?ºÎ°ú ?¥Î†§Í∞ÄÎ©??¥Ìéô???úÍ±∞
							if (LimitCount > 0 && NewStack < LimitCount && *bMaxStackCueActive)
							{
								if (UProjectERASC* CustomASC = Cast<UProjectERASC>(ASC))
								{
									if (WeakThis->MaxStackVfx.Get() && WeakThis->MaxStackVfx->CueTag.IsValid())
									{
										if (CustomASC->IsOwnerActorAuthoritative() || CustomASC->ScopedPredictionKey.IsLocalClientKey())
										CustomASC->RemoveGameplayCueBySource(WeakThis->MaxStackVfx->CueTag, WeakThis->MaxStackVfx.Get());
									}
									if (WeakThis->MaxStackSound.Get() && WeakThis->MaxStackSound->CueTag.IsValid())
									{
										if (CustomASC->IsOwnerActorAuthoritative() || CustomASC->ScopedPredictionKey.IsLocalClientKey())
										CustomASC->RemoveGameplayCueBySource(WeakThis->MaxStackSound->CueTag, WeakThis->MaxStackSound.Get());
									}
								}
								*bMaxStackCueActive = false;
							}
						}
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
	if (IsValid(ASC))
	{
		if (GESpec.GetPeriod() > 0.0f)
		{
			// Ï£ºÍ∏∞???±Ïù∏ Í≤ΩÏö∞
			ExecuteEffects(ASC, GESpec, PeriodicVfx.Get(), PeriodicSound.Get(), PredictionKey);
		}
		else
		{
			// Ï¶âÏãú(Instant) ?§Ìñâ??Í≤ΩÏö∞
			ExecuteEffects(ASC, GESpec, TriggerVfx.Get(), TriggerSound.Get(), PredictionKey);
		}
	}
}

void UVfxSfxGEC::ExecuteEffects(UAbilitySystemComponent* ASC, const FGameplayEffectSpec& GESpec, const USkillNiagaraSpawnConfig* VfxConfig, const USkillSoundSpawnConfig* SoundConfig, FPredictionKey PredictionKey) const
{
	if (!IsValid(ASC)) return;

	const FGameplayEffectContextHandle& Context = GESpec.GetContext();
	
	// Í∏∞Î≥∏ ?ÑÏπò ?§Ï†ï: Context??Origin???àÏúºÎ©??¨Ïö©, ?ÜÏúºÎ©?Target(Avatar) ?ÑÏπò ?¨Ïö©
	FVector CueLocation = Context.HasOrigin() ? Context.GetOrigin() : ASC->GetAvatarActor()->GetActorLocation();
	FVector CueDirection = FVector::UpVector;
	if (const FHitResult* Hit = Context.GetHitResult())
	{
		CueDirection = Hit->Normal;
	}

	// 1. VFX ?§Ìñâ
	if (IsValid(VfxConfig) && VfxConfig->CueTag.IsValid())
	{
		FGameplayCueParameters Params(GESpec);
		Params.Location = CueLocation;
		Params.Normal = CueDirection;
		Params.SourceObject = const_cast<USkillNiagaraSpawnConfig*>(VfxConfig);
		Params.Instigator = Context.GetInstigator();
		Params.EffectCauser = Context.GetEffectCauser();

		if (ASC->IsOwnerActorAuthoritative() || ASC->ScopedPredictionKey.IsLocalClientKey())
		ASC->ExecuteGameplayCue(VfxConfig->CueTag, Params);
	}

	// 2. SFX ?§Ìñâ
	if (IsValid(SoundConfig) && SoundConfig->CueTag.IsValid())
	{
		FGameplayCueParameters Params(GESpec);
		Params.Location = CueLocation;
		Params.Normal = CueDirection;
		Params.SourceObject = const_cast<USkillSoundSpawnConfig*>(SoundConfig);
		Params.Instigator = Context.GetInstigator();
		Params.EffectCauser = Context.GetEffectCauser();

		if (ASC->IsOwnerActorAuthoritative() || ASC->ScopedPredictionKey.IsLocalClientKey())
		ASC->ExecuteGameplayCue(SoundConfig->CueTag, Params);
	}
}

void UVfxSfxGEC::CollectNiagaraPaths(TArray<FSoftObjectPath>& OutPaths) const
{
	Super::CollectNiagaraPaths(OutPaths);
	if (TriggerVfx && !TriggerVfx->NiagaraSystem.IsNull())
	{
		OutPaths.AddUnique(TriggerVfx->NiagaraSystem.ToSoftObjectPath());
	}
	if (PeriodicVfx && !PeriodicVfx->NiagaraSystem.IsNull())
	{
		OutPaths.AddUnique(PeriodicVfx->NiagaraSystem.ToSoftObjectPath());
	}
	if (RemovedVfx && !RemovedVfx->NiagaraSystem.IsNull())
	{
		OutPaths.AddUnique(RemovedVfx->NiagaraSystem.ToSoftObjectPath());
	}
	if (MaxStackVfx && !MaxStackVfx->NiagaraSystem.IsNull())
	{
		OutPaths.AddUnique(MaxStackVfx->NiagaraSystem.ToSoftObjectPath());
	}
}
