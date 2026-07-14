// Fill out your copyright notice in the Description page of Project Settings.


#include "SkillSystem/AnimNotify/AnimNotifyState_SkillSoundGameplayCue.h"
#include "SkillSystem/GameplayCueNotify/Sound/SkillSoundSpawnConfig.h"
#include "SkillSystem/GameplayCueNotify/Sound/SkillSoundSpawnHelper.h"
#if WITH_EDITOR
#include "SkillSystem/AnimNotify/AnimNotifyCueTrackerComponent.h"
#endif
#include "AbilitySystemComponent.h"
#include "AbilitySystemGlobals.h"
#include "Animation/AnimMontage.h"
#include "Components/AudioComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Sound/SoundBase.h"

UAnimNotifyState_SkillSoundGameplayCue::UAnimNotifyState_SkillSoundGameplayCue()
	: Super()
{
#if WITH_EDITORONLY_DATA
	NotifyColor = FColor(150, 255, 150, 255); // 사운드니까 밝은 녹색 계열로 차별화
#endif
}

void UAnimNotifyState_SkillSoundGameplayCue::NotifyBegin(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, float TotalDuration, const FAnimNotifyEventReference& EventReference)
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
	Parameters.RawMagnitude = TotalDuration;
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
		ASC->InvokeGameplayCueEvent(GameplayCueTag, EGameplayCueEvent::OnActive, Parameters);
	}
	else
	{
		if (UGameplayCueManager* GCM = UAbilitySystemGlobals::Get().GetGameplayCueManager())
		{
			GCM->AddGameplayCue_NonReplicated(OwnerActor, GameplayCueTag, Parameters);
		}
	}

#if WITH_EDITOR
	if (UAnimNotifyCueTrackerComponent* Tracker = UAnimNotifyCueTrackerComponent::GetOrCreateTracker(OwnerActor))
	{
		Tracker->RegisterSoundCue(MeshComp, Cast<UAnimMontage>(Animation), SpawnConfig);
	}
#endif

#if WITH_EDITORONLY_DATA
	if (UGameplayCueManager::PreviewProxyTick.IsBound())
	{
		PreviewProxyTick = UGameplayCueManager::PreviewProxyTick;
		UGameplayCueManager::PreviewProxyTick.Unbind();
	}
#endif
}

void UAnimNotifyState_SkillSoundGameplayCue::NotifyTick(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, float FrameDeltaTime, const FAnimNotifyEventReference& EventReference)
{
	AActor* OwnerActor = MeshComp->GetOwner();

#if WITH_EDITOR
	if (MeshComp->GetWorld() && MeshComp->GetWorld()->WorldType == EWorldType::EditorPreview)
	{
#if WITH_EDITORONLY_DATA
		PreviewProxyTick.ExecuteIfBound(FrameDeltaTime);
#endif
		return;
	}
#endif
}

void UAnimNotifyState_SkillSoundGameplayCue::NotifyEnd(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, const FAnimNotifyEventReference& EventReference)
{
	if (!MeshComp || !SpawnConfig || !SpawnConfig->CueTag.IsValid())
	{
		return;
	}

	FGameplayTag GameplayCueTag = SpawnConfig->CueTag;
	AActor* OwnerActor = MeshComp->GetOwner();

#if WITH_EDITOR
	// 에디터 애니메이션 프리뷰 창에서의 처리
	UWorld* World = MeshComp->GetWorld();
	if (World && World->WorldType == EWorldType::EditorPreview)
	{
		// PlaySound2D는 정지가 불가능하므로 프리뷰에서는 루핑 정지 로직을 수행하지 않음
		return;
	}
#endif

	FGameplayCueParameters Parameters;
	Parameters.Instigator = OwnerActor;
	Parameters.TargetAttachComponent = MeshComp;

#if WITH_EDITOR
	if (IsValid(OwnerActor))
	{
		if (UAnimNotifyCueTrackerComponent* Tracker = OwnerActor->FindComponentByClass<UAnimNotifyCueTrackerComponent>())
		{
			if (SpawnConfig && !SpawnConfig->Sound.IsNull())
			{
				Tracker->UnregisterCue(MeshComp, SpawnConfig->Sound.LoadSynchronous());
			}
		}
	}
#endif

	if (UAbilitySystemComponent* ASC = UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(OwnerActor))
	{
		ASC->InvokeGameplayCueEvent(GameplayCueTag, EGameplayCueEvent::Removed, Parameters);
	}
	else
	{
		if (UGameplayCueManager* GCM = UAbilitySystemGlobals::Get().GetGameplayCueManager())
		{
			GCM->RemoveGameplayCue_NonReplicated(OwnerActor, GameplayCueTag, Parameters);
		}
	}
}

FString UAnimNotifyState_SkillSoundGameplayCue::GetNotifyName_Implementation() const
{
	if (SpawnConfig && SpawnConfig->CueTag.IsValid())
	{
		return SpawnConfig->CueTag.ToString() + TEXT(" (Sound Looping)");
	}

	return TEXT("Skill Sound GameplayCue State");
}

#if WITH_EDITOR
bool UAnimNotifyState_SkillSoundGameplayCue::CanBePlaced(UAnimSequenceBase* Animation) const
{
	return (Animation && Animation->IsA(UAnimMontage::StaticClass()));
}
#endif
