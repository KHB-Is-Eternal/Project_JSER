#include "SkillSystem/GameplayCueNotify/Sound/GCN_PlaySoundBySpawnConfig.h"
#include "SkillSystem/GameplayCueNotify/Sound/SkillSoundSpawnConfig.h"
#include "SkillSystem/GameplayCueNotify/Sound/SkillSoundSpawnHelper.h"
#include "AbilitySystemComponent.h"
#include "Components/AudioComponent.h"
#include "Sound/SoundBase.h"
#include "CharacterSystem/GameplayTags/GameplayTags.h"

namespace
{
	const USkillSoundSpawnConfig* GetSoundSpawnConfigFromParameters(const FGameplayCueParameters& Parameters)
	{
		return Cast<USkillSoundSpawnConfig>(Parameters.SourceObject.Get());
	}

	bool ShouldSkipSoundOnServer(const AActor* MyTarget)
	{
		if (!IsValid(MyTarget))
		{
			return true;
		}
		return MyTarget->GetNetMode() == NM_DedicatedServer;
	}
}

bool UGCN_PlaySoundBySpawnConfig::OnExecute_Implementation(AActor* MyTarget, const FGameplayCueParameters& Parameters) const
{
	if (ShouldSkipSoundOnServer(MyTarget))
	{
		return false;
	}

	UWorld* const World = MyTarget->GetWorld();
	if (!IsValid(World))
	{
		return false;
	}

	const USkillSoundSpawnConfig* const SpawnConfig = GetSoundSpawnConfigFromParameters(Parameters);
	if (!IsValid(SpawnConfig))
	{
		return false;
	}

	const FSkillSoundSpawnSettings SpawnSettings = SpawnConfig->ToSettings();
	if (SpawnSettings.Sound.IsNull())
	{
		return false;
	}

	const AActor* const EffectCauser = Cast<AActor>(Parameters.EffectCauser.Get());
	const AActor* const Instigator = Cast<AActor>(Parameters.Instigator.Get());

	// 네이티브 태그 참조
	const FGameplayTag& TagSummoner = ProjectER::GameplayCue::Sound::Summoner;
	const FGameplayTag& TagHitTarget = ProjectER::GameplayCue::Sound::HitTarget;
	
	const AActor* SourceActor = nullptr;
	if (Parameters.OriginalTag.MatchesTag(TagSummoner))
	{
	    SourceActor = IsValid(Instigator) ? Instigator : MyTarget;
	}
	else if (Parameters.OriginalTag.MatchesTag(TagHitTarget))
	{
	    SourceActor = MyTarget;
	}
	else // 기본값 (기존 Range 로직 통합)
	{
	    SourceActor = EffectCauser;
	}

	// 3. Transform 설정: SourceActor가 유효하면 그 위치를, 아니면 전달받은 Parameters.Location을 사용
	FTransform SourceTransform = IsValid(SourceActor) ? SourceActor->GetActorTransform() : FTransform(FRotator::ZeroRotator, Parameters.Location);

	SkillSoundSpawnHelper::PlaySoundBySettings(World, SpawnSettings, SourceTransform, SourceActor, nullptr, Parameters.TargetAttachComponent.Get());
	return true;
}

bool UGCN_PlaySoundBySpawnConfig::OnActive_Implementation(AActor* MyTarget, const FGameplayCueParameters& Parameters) const
{
	return OnExecute_Implementation(MyTarget, Parameters);
}

bool UGCN_PlaySoundBySpawnConfig::OnRemove_Implementation(AActor* MyTarget, const FGameplayCueParameters& Parameters) const
{
	if (!IsValid(MyTarget))
	{
		return false;
	}

	const USkillSoundSpawnConfig* const SpawnConfig = GetSoundSpawnConfigFromParameters(Parameters);
	if (!IsValid(SpawnConfig) || SpawnConfig->Sound.IsNull())
	{
		return false;
	}

	USoundBase* const LoadedSound = SpawnConfig->Sound.LoadSynchronous();
	if (!IsValid(LoadedSound))
	{
		return false;
	}

	// 캐릭터에서 동일한 Sound를 가진 오디오 컴포넌트를 찾아 정지
	TArray<UAudioComponent*> AudioComponents;
	MyTarget->GetComponents<UAudioComponent>(AudioComponents);
	for (UAudioComponent* AC : AudioComponents)
	{
		if (IsValid(AC) && AC->Sound == LoadedSound)
		{
			AC->Stop();
		}
	}

	return true;
}
