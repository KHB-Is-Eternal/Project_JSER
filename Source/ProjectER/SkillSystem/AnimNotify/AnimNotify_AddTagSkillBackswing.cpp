// Fill out your copyright notice in the Description page of Project Settings.


#include "SkillSystem/AnimNotify/AnimNotify_AddTagSkillBackswing.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystemBlueprintLibrary.h"

UAnimNotify_AddTagSkillBackswing::UAnimNotify_AddTagSkillBackswing()
{
	BackswingTag = FGameplayTag::RequestGameplayTag(FName("Skill.Animation.Backswing"));
}

void UAnimNotify_AddTagSkillBackswing::Notify(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, const FAnimNotifyEventReference& EventReference)
{
	Super::Notify(MeshComp, Animation, EventReference);

	if (!MeshComp) return;

	AActor* OwnerActor = MeshComp->GetOwner();
	if (!OwnerActor) return;

	if (!MeshComp->GetWorld() || !MeshComp->GetWorld()->IsGameWorld()) return;

	// 이벤트 전송
	FGameplayEventData Payload;
	Payload.EventTag = BackswingTag;
	Payload.Instigator = OwnerActor;
	Payload.Target = OwnerActor;
	Payload.EventMagnitude = EventMagnitude;

	UAbilitySystemBlueprintLibrary::SendGameplayEventToActor(OwnerActor, BackswingTag, Payload);
}
