// Fill out your copyright notice in the Description page of Project Settings.
// Force recompilation to clear UBT link cache


#include "SkillSystem/GameplayAbilityTargetActor/TargetActor.h"
#include "GameFramework/Actor.h"
#include "SkillSystem/GameAbility/MouseTargetSkill.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/Pawn.h"

ATargetActor::ATargetActor()
{
	PrimaryActorTick.bCanEverTick = true;
	bReplicates = false;
}

void ATargetActor::Setup(float InMaxRange)
{
	MaxRange = InMaxRange;
}

void ATargetActor::StartTargeting(UGameplayAbility* Ability)
{
	Super::StartTargeting(Ability);
	SetActorTickEnabled(true);
}

void ATargetActor::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	Super::EndPlay(EndPlayReason);
}

void ATargetActor::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
}

void ATargetActor::ConfirmTargetingAndContinue()
{
    UMouseTargetSkill* MouseSkill = Cast<UMouseTargetSkill>(OwningAbility);
    if (!ensureMsgf(IsValid(MouseSkill), TEXT("ATargetActor::ConfirmTargetingAndContinue - MouseSkill Is Not Valid"))) { return; }

    if (TryConfirmMouseTarget() == false)
    {
        FGameplayAbilityTargetDataHandle CancelHandle;
        CanceledDelegate.Broadcast(CancelHandle);
    }
}

bool ATargetActor::TryConfirmMouseTarget()
{
    UMouseTargetSkill* MouseSkill = Cast<UMouseTargetSkill>(OwningAbility);
    if (!ensureMsgf(IsValid(MouseSkill), TEXT("ATargetActor::ConfirmTargetingAndContinue - MouseSkill Is Not Valid"))) { return false; }

    AActor* ValidTarget = MouseSkill->GetTargetUnderCursorInRange();

    if (ValidTarget)
    {
        FGameplayAbilityTargetDataHandle Handle = UAbilitySystemBlueprintLibrary::AbilityTargetDataFromActor(ValidTarget);
        TargetDataReadyDelegate.Broadcast(Handle);
        return true;
    }

    return false;
}

bool ATargetActor::SubmitExternalTarget(AActor* InTargetActor)
{
    UMouseTargetSkill* MouseSkill = Cast<UMouseTargetSkill>(OwningAbility);
    if (!IsValid(MouseSkill)) return false;
    if (!MouseSkill->IsTargetActorInRange(InTargetActor)) return false;

    FGameplayAbilityTargetDataHandle Handle = UAbilitySystemBlueprintLibrary::AbilityTargetDataFromActor(InTargetActor);
    TargetDataReadyDelegate.Broadcast(Handle);
    return true;
}