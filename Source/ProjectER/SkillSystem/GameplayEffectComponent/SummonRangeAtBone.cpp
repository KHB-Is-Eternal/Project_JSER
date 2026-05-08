// Fill out your copyright notice in the Description page of Project Settings.


#include "SkillSystem/GameplayEffectComponent/SummonRangeAtBone.h"

#include "Abilities/GameplayAbilityTypes.h"
#include "GameplayEffect.h"
#include "DrawDebugHelpers.h"
#include "Components/SkeletalMeshComponent.h"
#include "SkillSystem/Actor/BaseRangeOverlapEffectActor/BaseRangeOverlapEffectActor.h"

USummonRangeAtBone::USummonRangeAtBone()
{
}


FTransform USummonRangeAtBone::CalculateOriginTransform(const FGameplayEffectSpec& GESpec, const AActor* Instigator, const AActor* TargetActor) const
{
	if (!IsValid(Instigator))
	{
		return FTransform::Identity;
	}

	FVector BaseLocation = Instigator->GetActorLocation();
	FRotator BaseRotation = Instigator->GetActorRotation();

	if (const USkeletalMeshComponent* const Mesh = Instigator->FindComponentByClass<USkeletalMeshComponent>())
	{
		if (Mesh->DoesSocketExist(this->BoneName))
		{
			BaseLocation = Mesh->GetSocketLocation(this->BoneName);
			BaseRotation = Mesh->GetSocketRotation(this->BoneName);
		}
	}

	FRotator CombinedRotation = BaseRotation;
	if (this->bUseInstigatorRotation)
	{
		CombinedRotation = Instigator->GetActorRotation();
	}

	return FTransform(CombinedRotation, BaseLocation);
}

void USummonRangeAtBone::InitializeRangeActor(ABaseRangeOverlapEffectActor* RangeActor, AActor* Instigator, const FGameplayEffectContextHandle& Context, const FGameplayCueParameters& HitTargetVfxCueParameters, const FGameplayCueParameters& HitTargetSoundCueParameters) const
{
	Super::InitializeRangeActor(RangeActor, Instigator, Context, HitTargetVfxCueParameters, HitTargetSoundCueParameters);

	if (!this->bAttachToBone || !IsValid(Instigator) || !IsValid(RangeActor))
	{
		return;
	}

	if (USkeletalMeshComponent* const Mesh = Instigator->FindComponentByClass<USkeletalMeshComponent>())
	{
		if (Mesh->DoesSocketExist(this->BoneName))
		{
			RangeActor->AttachToComponent(Mesh, FAttachmentTransformRules::KeepWorldTransform, this->BoneName);
		}
	}
}