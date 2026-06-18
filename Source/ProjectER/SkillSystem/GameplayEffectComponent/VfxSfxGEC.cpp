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
		// 1. 발동 효과 실행 (Duration/Infinite GE이므로 지속성 효과로 등록)
		TArray<TPair<FGameplayTag, TWeakObjectPtr<const UObject>>> OngoingCues;

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
				OngoingCues.Add(TPair<FGameplayTag, TWeakObjectPtr<const UObject>>(Tag, Config));
			}
		};

		if (IsValid(TriggerVfx)) AddOngoing(TriggerVfx.Get(), TriggerVfx->CueTag);
		if (IsValid(TriggerSound)) AddOngoing(TriggerSound.Get(), TriggerSound->CueTag);

		// 1.1 최초 적용 시점 스택 수치가 최대 스택인지 검사 및 상태 저장
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

				FScopedPredictionWindow PredictionWindow(TargetASC, !TargetASC->GetPredictionKeyForNewAction().IsValidKey());
				TargetASC->AddGameplayCue(MaxStackVfx->CueTag, Params);
				*bMaxStackCueActive = true;
			}
			if (IsValid(MaxStackSound.Get()) && MaxStackSound->CueTag.IsValid())
			{
				FGameplayCueParameters Params(ActiveGE.Spec);
				Params.SourceObject = MaxStackSound.Get();
				Params.Instigator = ActiveGE.Spec.GetContext().GetInstigator();
				Params.EffectCauser = ActiveGE.Spec.GetContext().GetEffectCauser();

				FScopedPredictionWindow PredictionWindow(TargetASC, !TargetASC->GetPredictionKeyForNewAction().IsValidKey());
				TargetASC->AddGameplayCue(MaxStackSound->CueTag, Params);
			}
		}

		// 2. 제거 효과 바인딩 (Ongoing 중단 + Removed 효과 실행)
		if (OngoingCues.Num() > 0 || IsValid(RemovedVfx.Get()) || IsValid(RemovedSound.Get()) || IsValid(MaxStackVfx.Get()) || IsValid(MaxStackSound.Get()))
		{
			TWeakObjectPtr<const UVfxSfxGEC> WeakThis(this);
			TWeakObjectPtr<UAbilitySystemComponent> WeakASC(TargetASC);
			FGameplayEffectSpec Spec = ActiveGE.Spec; // 복사본 저장

			ActiveGE.EventSet.OnEffectRemoved.AddLambda([WeakThis, WeakASC, Spec, OngoingCues, bMaxStackCueActive](const FGameplayEffectRemovalInfo& RemovalInfo)
			{
				if (WeakASC.IsValid())
				{
					UProjectERASC* CustomASC = Cast<UProjectERASC>(WeakASC.Get());
					ensureMsgf(CustomASC != nullptr, TEXT("OnEffectRemoved: ASC is not UProjectERASC! Check Blueprint CDO or reparent the BP."));
					
					if (CustomASC)
					{
						// 지속성으로 등록된 발동 효과 제거
						for (const auto& CuePair : OngoingCues)
						{
							FScopedPredictionWindow PredictionWindow(CustomASC, !CustomASC->GetPredictionKeyForNewAction().IsValidKey());
							CustomASC->RemoveGameplayCueBySource(CuePair.Key, CuePair.Value.Get());
						}

						// 최대 스택 효과 제거
						if (*bMaxStackCueActive && WeakThis.IsValid())
						{
							if (WeakThis->MaxStackVfx.Get() && WeakThis->MaxStackVfx->CueTag.IsValid())
							{
								FScopedPredictionWindow PredictionWindow(CustomASC, !CustomASC->GetPredictionKeyForNewAction().IsValidKey());
								CustomASC->RemoveGameplayCueBySource(WeakThis->MaxStackVfx->CueTag, WeakThis->MaxStackVfx.Get());
							}
							if (WeakThis->MaxStackSound.Get() && WeakThis->MaxStackSound->CueTag.IsValid())
							{
								FScopedPredictionWindow PredictionWindow(CustomASC, !CustomASC->GetPredictionKeyForNewAction().IsValidKey());
								CustomASC->RemoveGameplayCueBySource(WeakThis->MaxStackSound->CueTag, WeakThis->MaxStackSound.Get());
							}
							*bMaxStackCueActive = false;
						}
					}

					// 제거 시점의 효과 실행 (Burst)
					if (WeakThis.IsValid())
					{
						WeakThis->ExecuteEffects(WeakASC.Get(), Spec, WeakThis->RemovedVfx.Get(), WeakThis->RemovedSound.Get());
					}
				}
			});
		}

		// 3. 스택 변경 시 발동 효과 재실행 (스택 증가 및 최대 스택 도달 연출)
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
							// 스택 증가 이펙트 실행
							WeakThis->ExecuteEffects(ASC, Effect->Spec, WeakThis->TriggerVfx.Get(), WeakThis->TriggerSound.Get());

							// 최대 스택 도달 시 이펙트 (최초 도달 순간에만 실행)
							if (LimitCount > 0 && NewStack >= LimitCount && !(*bMaxStackCueActive))
							{
								if (WeakThis->MaxStackVfx.Get() && WeakThis->MaxStackVfx->CueTag.IsValid())
								{
									FGameplayCueParameters Params(Effect->Spec);
									Params.SourceObject = WeakThis->MaxStackVfx.Get();
									Params.Instigator = Effect->Spec.GetContext().GetInstigator();
									Params.EffectCauser = Effect->Spec.GetContext().GetEffectCauser();

									FScopedPredictionWindow PredictionWindow(ASC, !ASC->GetPredictionKeyForNewAction().IsValidKey());
									ASC->AddGameplayCue(WeakThis->MaxStackVfx->CueTag, Params);
								}
								if (WeakThis->MaxStackSound.Get() && WeakThis->MaxStackSound->CueTag.IsValid())
								{
									FGameplayCueParameters Params(Effect->Spec);
									Params.SourceObject = WeakThis->MaxStackSound.Get();
									Params.Instigator = Effect->Spec.GetContext().GetInstigator();
									Params.EffectCauser = Effect->Spec.GetContext().GetEffectCauser();

									FScopedPredictionWindow PredictionWindow(ASC, !ASC->GetPredictionKeyForNewAction().IsValidKey());
									ASC->AddGameplayCue(WeakThis->MaxStackSound->CueTag, Params);
								}
								*bMaxStackCueActive = true;
							}
						}
						else if (NewStack < OldStack)
						{
							// 스택 감소 시 최대 스택 미만으로 내려가면 이펙트 제거
							if (LimitCount > 0 && NewStack < LimitCount && *bMaxStackCueActive)
							{
								if (UProjectERASC* CustomASC = Cast<UProjectERASC>(ASC))
								{
									if (WeakThis->MaxStackVfx.Get() && WeakThis->MaxStackVfx->CueTag.IsValid())
									{
										FScopedPredictionWindow PredictionWindow(CustomASC, !CustomASC->GetPredictionKeyForNewAction().IsValidKey());
										CustomASC->RemoveGameplayCueBySource(WeakThis->MaxStackVfx->CueTag, WeakThis->MaxStackVfx.Get());
									}
									if (WeakThis->MaxStackSound.Get() && WeakThis->MaxStackSound->CueTag.IsValid())
									{
										FScopedPredictionWindow PredictionWindow(CustomASC, !CustomASC->GetPredictionKeyForNewAction().IsValidKey());
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

	// 1. VFX 실행
	if (IsValid(VfxConfig) && VfxConfig->CueTag.IsValid())
	{
		FGameplayCueParameters Params(GESpec);
		Params.Location = CueLocation;
		Params.Normal = CueDirection;
		Params.SourceObject = const_cast<USkillNiagaraSpawnConfig*>(VfxConfig);
		Params.Instigator = Context.GetInstigator();
		Params.EffectCauser = Context.GetEffectCauser();

		FScopedPredictionWindow PredictionWindow(ASC, !ASC->GetPredictionKeyForNewAction().IsValidKey());
		ASC->ExecuteGameplayCue(VfxConfig->CueTag, Params);
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

		FScopedPredictionWindow PredictionWindow(ASC, !ASC->GetPredictionKeyForNewAction().IsValidKey());
		ASC->ExecuteGameplayCue(SoundConfig->CueTag, Params);
	}
}
