// Fill out your copyright notice in the Description page of Project Settings.


#include "SkillSystem/AnimNotify/AnimNotifyState_SkillGameplayCue.h"
#include "SkillSystem/GameplayCueNotify/Particle/SkillNiagaraSpawnConfig.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystemGlobals.h"
#include "Animation/AnimMontage.h"

UAnimNotifyState_SkillGameplayCue::UAnimNotifyState_SkillGameplayCue()
	: Super()
{
#if WITH_EDITORONLY_DATA
	NotifyColor = FColor(100, 100, 255, 255);
#endif
}

void UAnimNotifyState_SkillGameplayCue::NotifyBegin(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, float TotalDuration, const FAnimNotifyEventReference& EventReference)
{
	if (!MeshComp || !SpawnConfig || !SpawnConfig->CueTag.IsValid())
	{
		return;
	}

	FGameplayTag GameplayCueTag = SpawnConfig->CueTag;
	AActor* OwnerActor = MeshComp->GetOwner();

#if WITH_EDITOR
	if (GIsEditor && (OwnerActor == nullptr))
	{
		UGameplayCueManager::PreviewComponent = MeshComp;
		UGameplayCueManager::PreviewWorld = MeshComp->GetWorld();

		if (UGameplayCueManager* GCM = UAbilitySystemGlobals::Get().GetGameplayCueManager())
		{
			GCM->LoadNotifyForEditorPreview(GameplayCueTag);
		}
	}
#endif

	FGameplayCueParameters Parameters;
	Parameters.Instigator = OwnerActor;
	Parameters.TargetAttachComponent = MeshComp;
	Parameters.RawMagnitude = TotalDuration;
	Parameters.SourceObject = SpawnConfig;

	if (UAbilitySystemComponent* ASC = UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(OwnerActor))
	{
		if (ASC->GetCurrentMontage() == Animation)
		{
			if (const UGameplayAbility* Ability = ASC->GetAnimatingAbility())
			{
				Parameters.AbilityLevel = Ability->GetAbilityLevel();
			}
		}
		Parameters.Instigator = ASC->GetOwner();
		ASC->AddGameplayCue(GameplayCueTag, Parameters);
	}
	else
	{
		if (UGameplayCueManager* GCM = UAbilitySystemGlobals::Get().GetGameplayCueManager())
		{
			GCM->AddGameplayCue_NonReplicated(OwnerActor, GameplayCueTag, Parameters);
		}
	}

#if WITH_EDITORONLY_DATA
	if (UGameplayCueManager::PreviewProxyTick.IsBound())
	{
		PreviewProxyTick = UGameplayCueManager::PreviewProxyTick;
		UGameplayCueManager::PreviewProxyTick.Unbind();
	}
#endif

#if WITH_EDITOR
	if (GIsEditor)
	{
		UGameplayCueManager::PreviewComponent = nullptr;
		UGameplayCueManager::PreviewWorld = nullptr;
	}
#endif
}

void UAnimNotifyState_SkillGameplayCue::NotifyTick(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, float FrameDeltaTime, const FAnimNotifyEventReference& EventReference)
{
	AActor* OwnerActor = MeshComp->GetOwner();

#if WITH_EDITOR
	if (GIsEditor && OwnerActor == nullptr)
	{
		UGameplayCueManager::PreviewComponent = MeshComp;
		UGameplayCueManager::PreviewWorld = MeshComp->GetWorld();

#if WITH_EDITORONLY_DATA
		PreviewProxyTick.ExecuteIfBound(FrameDeltaTime);
#endif
	}
#endif

#if WITH_EDITOR
	if (GIsEditor)
	{
		UGameplayCueManager::PreviewComponent = nullptr;
		UGameplayCueManager::PreviewWorld = nullptr;
	}
#endif
}

void UAnimNotifyState_SkillGameplayCue::NotifyEnd(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, const FAnimNotifyEventReference& EventReference)
{
	if (!MeshComp || !SpawnConfig || !SpawnConfig->CueTag.IsValid())
	{
		return;
	}

	FGameplayTag GameplayCueTag = SpawnConfig->CueTag;
	AActor* OwnerActor = MeshComp->GetOwner();

#if WITH_EDITOR
	if (GIsEditor && (OwnerActor == nullptr))
	{
		UGameplayCueManager::PreviewComponent = MeshComp;
		UGameplayCueManager::PreviewWorld = MeshComp->GetWorld();
	}
#endif

	FGameplayCueParameters Parameters;
	Parameters.Instigator = OwnerActor;
	Parameters.TargetAttachComponent = MeshComp;

	if (UAbilitySystemComponent* ASC = UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(OwnerActor))
	{
		ASC->RemoveGameplayCue(GameplayCueTag);
	}
	else
	{
		if (UGameplayCueManager* GCM = UAbilitySystemGlobals::Get().GetGameplayCueManager())
		{
			GCM->RemoveGameplayCue_NonReplicated(OwnerActor, GameplayCueTag, Parameters);
		}
	}

#if WITH_EDITOR
	if (GIsEditor)
	{
		UGameplayCueManager::PreviewComponent = nullptr;
		UGameplayCueManager::PreviewWorld = nullptr;
	}
#endif
}

FString UAnimNotifyState_SkillGameplayCue::GetNotifyName_Implementation() const
{
	if (SpawnConfig && SpawnConfig->CueTag.IsValid())
	{
		return SpawnConfig->CueTag.ToString() + TEXT(" (Skill Looping)");
	}

	return TEXT("Skill GameplayCue State");
}

#if WITH_EDITOR
bool UAnimNotifyState_SkillGameplayCue::CanBePlaced(UAnimSequenceBase* Animation) const
{
	return (Animation && Animation->IsA(UAnimMontage::StaticClass()));
}
#endif
