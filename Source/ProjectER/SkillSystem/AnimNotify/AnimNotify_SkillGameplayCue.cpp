// Fill out your copyright notice in the Description page of Project Settings.


#include "SkillSystem/AnimNotify/AnimNotify_SkillGameplayCue.h"
#include "SkillSystem/GameplayCueNotify/Particle/SkillNiagaraSpawnConfig.h"
#if WITH_EDITOR
#include "SkillSystem/AnimNotify/AnimNotifyCueTrackerComponent.h"
#endif
#include "AbilitySystemComponent.h"
#include "AbilitySystemGlobals.h"
#include "GameplayCueManager.h"
#include "Animation/AnimMontage.h"

UAnimNotify_SkillGameplayCue::UAnimNotify_SkillGameplayCue()
	: Super()
{
#if WITH_EDITORONLY_DATA
	NotifyColor = FColor(255, 100, 100, 255);
#endif
}

void UAnimNotify_SkillGameplayCue::Notify(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, const FAnimNotifyEventReference& EventReference)
{
	if (!MeshComp || !SpawnConfig || !SpawnConfig->CueTag.IsValid())
	{
		return;
	}

	FGameplayTag GameplayCueTag = SpawnConfig->CueTag;
	AActor* OwnerActor = MeshComp->GetOwner();

#if WITH_EDITOR
	// 에디터 애니메이션 프리뷰 창에서도 시각 효과가 보이도록 설정 (엔진 로직 복제)
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
	Parameters.SourceObject = SpawnConfig; // 핵심 데이터 주입

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

#if WITH_EDITOR
	if (UAnimNotifyCueTrackerComponent* Tracker = UAnimNotifyCueTrackerComponent::GetOrCreateTracker(OwnerActor))
	{
		Tracker->RegisterNiagaraCue(MeshComp, Cast<UAnimMontage>(Animation), SpawnConfig);
	}

	if (GIsEditor)
	{
		UGameplayCueManager::PreviewComponent = nullptr;
		UGameplayCueManager::PreviewWorld = nullptr;
	}
#endif
}

FString UAnimNotify_SkillGameplayCue::GetNotifyName_Implementation() const
{
	if (SpawnConfig && SpawnConfig->CueTag.IsValid())
	{
		return SpawnConfig->CueTag.ToString() + TEXT(" (Skill Burst)");
	}

	return TEXT("Skill GameplayCue");
}

#if WITH_EDITOR
bool UAnimNotify_SkillGameplayCue::CanBePlaced(UAnimSequenceBase* Animation) const
{
	return (Animation && Animation->IsA(UAnimMontage::StaticClass()));
}
#endif
