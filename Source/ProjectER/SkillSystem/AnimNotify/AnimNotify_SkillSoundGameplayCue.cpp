// Fill out your copyright notice in the Description page of Project Settings.


#include "SkillSystem/AnimNotify/AnimNotify_SkillSoundGameplayCue.h"
#include "SkillSystem/GameplayCueNotify/Sound/SkillSoundSpawnConfig.h"
#include "SkillSystem/GameplayCueNotify/Sound/SkillSoundSpawnHelper.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystemGlobals.h"
#include "GameplayCueManager.h"
#include "Animation/AnimMontage.h"
#include "Kismet/GameplayStatics.h"
#include "Sound/SoundBase.h"

UAnimNotify_SkillSoundGameplayCue::UAnimNotify_SkillSoundGameplayCue()
	: Super()
{
#if WITH_EDITORONLY_DATA
	NotifyColor = FColor(100, 255, 100, 255); // 사운드니까 녹색 계열로 차별화
#endif
}

void UAnimNotify_SkillSoundGameplayCue::Notify(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, const FAnimNotifyEventReference& EventReference)
{
	if (!MeshComp || !SpawnConfig || !SpawnConfig->CueTag.IsValid())
	{
		return;
	}

	FGameplayTag GameplayCueTag = SpawnConfig->CueTag;
	AActor* OwnerActor = MeshComp->GetOwner();

#if WITH_EDITOR
	// 에디터 애니메이션 프리뷰 창에서는 2D 사운드로 재생
	UWorld* World = MeshComp->GetWorld();
	if (World && World->WorldType == EWorldType::EditorPreview)
	{
		if (MeshComp->IsPlaying())
		{
			USoundBase* Sound = SpawnConfig->Sound.LoadSynchronous();
			UGameplayStatics::PlaySound2D(World, Sound, SpawnConfig->VolumeMultiplier, SpawnConfig->PitchMultiplier);
		}
		return;
	}
#endif

	FGameplayCueParameters Parameters;
	Parameters.Instigator = OwnerActor;
	Parameters.TargetAttachComponent = MeshComp;
	Parameters.SourceObject = SpawnConfig; // 사운드 설정 데이터 주입

	if (UAbilitySystemComponent* ASC = UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(OwnerActor))
	{
		if (ASC->GetCurrentMontage() == Animation)
		{
			if (const UGameplayAbility* Ability = ASC->GetAnimatingAbility())
			{
				Parameters.AbilityLevel = Ability->GetAbilityLevel();
			}
		}

		ASC->ExecuteGameplayCue(GameplayCueTag, Parameters);
	}
	else
	{
		if (UGameplayCueManager* GCM = UAbilitySystemGlobals::Get().GetGameplayCueManager())
		{
			GCM->ExecuteGameplayCue_NonReplicated(OwnerActor, GameplayCueTag, Parameters);
		}
	}
}

FString UAnimNotify_SkillSoundGameplayCue::GetNotifyName_Implementation() const
{
	if (SpawnConfig && SpawnConfig->CueTag.IsValid())
	{
		return SpawnConfig->CueTag.ToString() + TEXT(" (Sound Burst)");
	}

	return TEXT("Skill Sound GameplayCue");
}

#if WITH_EDITOR
bool UAnimNotify_SkillSoundGameplayCue::CanBePlaced(UAnimSequenceBase* Animation) const
{
	return (Animation && Animation->IsA(UAnimMontage::StaticClass()));
}
#endif
