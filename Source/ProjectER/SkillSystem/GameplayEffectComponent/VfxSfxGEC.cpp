#include "SkillSystem/GameplayEffectComponent/VfxSfxGEC.h"

#include "AbilitySystemComponent.h"
#include "AbilitySystemGlobals.h"
#include "GameplayCueManager.h"
#include "GameplayEffect.h"
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
		// 1. 발동 효과 실행 (Trigger)
		ExecuteEffects(TargetASC, ActiveGE.Spec, TriggerVfx.Get(), TriggerSound.Get());

		// 2. 제거 효과 바인딩 (Removal)
		if (IsValid(RemovedVfx.Get()) || IsValid(RemovedSound.Get()))
		{
			// Weak Object Pointer를 사용하여 해제된 객체 접근 방지
			TWeakObjectPtr<const UVfxSfxGEC> WeakThis(this);
			TWeakObjectPtr<UAbilitySystemComponent> WeakASC(TargetASC);
			FGameplayEffectSpec Spec = ActiveGE.Spec; // 복사본 저장 (제거 시점에 Spec이 유효하지 않을 수 있음)

			ActiveGE.EventSet.OnEffectRemoved.AddLambda([WeakThis, WeakASC, Spec](const FGameplayEffectRemovalInfo& RemovalInfo)
			{
				if (WeakThis.IsValid() && WeakASC.IsValid())
				{
					WeakThis->ExecuteEffects(WeakASC.Get(), Spec, WeakThis->RemovedVfx.Get(), WeakThis->RemovedSound.Get());
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
