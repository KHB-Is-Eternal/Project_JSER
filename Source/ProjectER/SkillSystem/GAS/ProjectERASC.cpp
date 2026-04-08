#include "SkillSystem/GAS/ProjectERASC.h"
#include "SkillSystem/GameplayEffectComponent/BaseGEC.h"
#include "GameplayEffect.h"
#include "GameplayEffectComponent.h"
#include "GameplayEffectComponent.h"
#include "SkillSystem/GameplayEffect/BaseGameplayEffect.h"
#include "SkillSystem/GameplayCueNotify/Particle/SkillNiagaraSpawnConfig.h"
#include "SkillSystem/GameplayCueNotify/Sound/SkillSoundSpawnConfig.h"

UProjectERASC::UProjectERASC()
{
	UE_LOG(LogTemp, Warning, TEXT("[ProjectERASC] UProjectERASC Instance Created! (%s)"), *GetPathName());
}

void UProjectERASC::Call_InvokeGameplayCueExecuted_FromSpec(const FGameplayEffectSpecForRPC Spec, FPredictionKey PredictionKey)
{
	if (Spec.Def)
	{
		FGameplayCueParameters Parameters(Spec);
		
		// GE에 정의된 모든 GameplayCue들에 대해 SourceObject 주입 시도
		for (const FGameplayEffectCue& CueInfo : Spec.Def->GameplayCues)
		{
			for (auto It = CueInfo.GameplayCueTags.CreateConstIterator(); It; ++It)
			{
				InjectConfigsIntoParameters(*It, Parameters, Cast<UBaseGameplayEffect>(Spec.Def));
				Super::Call_InvokeGameplayCueExecuted_WithParams(*It, PredictionKey, Parameters);
			}
		}
	}
	else
	{
		Super::Call_InvokeGameplayCueExecuted_FromSpec(Spec, PredictionKey);
	}
}

void UProjectERASC::Call_InvokeGameplayCueExecuted_WithParams(const FGameplayTag GameplayCueTag, FPredictionKey PredictionKey, FGameplayCueParameters GameplayCueParameters)
{
	if (const UGameplayEffect* GE = GameplayCueParameters.EffectContext.GetInstigatorAbilitySystemComponent() ? nullptr : nullptr) // Placeholder
	{
		// Context를 통해 GE를 찾을 수 있는 경우 주입 로직 실행 (필요 시 확장)
	}

	InjectConfigsIntoParameters(GameplayCueTag, GameplayCueParameters, nullptr);
	Super::Call_InvokeGameplayCueExecuted_WithParams(GameplayCueTag, PredictionKey, GameplayCueParameters);
}

void UProjectERASC::Call_InvokeGameplayCueAdded_WithParams(const FGameplayTag GameplayCueTag, FPredictionKey PredictionKey, FGameplayCueParameters Parameters)
{
	InjectConfigsIntoParameters(GameplayCueTag, Parameters, nullptr);
	Super::Call_InvokeGameplayCueAdded_WithParams(GameplayCueTag, PredictionKey, Parameters);
}

void UProjectERASC::Call_InvokeGameplayCueAddedAndWhileActive_WithParams(const FGameplayTag GameplayCueTag, FPredictionKey PredictionKey, FGameplayCueParameters GameplayCueParameters)
{
	InjectConfigsIntoParameters(GameplayCueTag, GameplayCueParameters, nullptr);
	Super::Call_InvokeGameplayCueAddedAndWhileActive_WithParams(GameplayCueTag, PredictionKey, GameplayCueParameters);
}

void UProjectERASC::InjectConfigsIntoParameters(const FGameplayTag& CueTag, FGameplayCueParameters& Parameters, const UBaseGameplayEffect* BaseGE)
{
	// 기존에 SourceObject가 이미 지정되어 있다면 스킵
	if (Parameters.SourceObject.IsValid())
	{
		return;
	}

	if (!BaseGE)
	{
		UE_LOG(LogTemp, Verbose, TEXT("[ProjectERASC] BaseGE is null for CueTag: %s. Trying to find from SourceObject..."), *CueTag.ToString());
		// GE가 없더라도 SourceObject가 이미 어떤 정보를 가지고 있는지 확인용 로그
		if (Parameters.SourceObject.IsValid())
		{
			UE_LOG(LogTemp, Verbose, TEXT("[ProjectERASC] Existing SourceObject: %s"), *Parameters.SourceObject->GetName());
		}
		return;
	}

	TArray<const UObject*> FoundConfigs;

	// 리플렉션 없이 안전하게 부모의 GEComponents에 접근
	for (const TObjectPtr<UGameplayEffectComponent>& Component : BaseGE->GetGEComponents())
	{
		if (UBaseGEC* BaseGEC = Cast<UBaseGEC>(Component))
		{
			FoundConfigs.Reset();
			BaseGEC->CollectCueConfigs(FoundConfigs);

			for (const UObject* Config : FoundConfigs)
			{
				if (!Config) continue;

				FGameplayTag ConfigTag;
				if (const USkillNiagaraSpawnConfig* NiagaraConfig = Cast<USkillNiagaraSpawnConfig>(Config))
				{
					ConfigTag = NiagaraConfig->CueTag;
				}
				else if (const USkillSoundSpawnConfig* SoundConfig = Cast<USkillSoundSpawnConfig>(Config))
				{
					ConfigTag = SoundConfig->CueTag;
				}

				if (ConfigTag.IsValid() && CueTag.MatchesTag(ConfigTag))
				{
					Parameters.SourceObject = Config;
					UE_LOG(LogTemp, Log, TEXT("[ProjectERASC] Successfully injected config for tag: %s from GEC: %s"), *CueTag.ToString(), *BaseGEC->GetName());
					return;
				}
			}
		}
	}

	UE_LOG(LogTemp, Warning, TEXT("[ProjectERASC] Failed to find matching config in GECs for tag: %s"), *CueTag.ToString());
}
