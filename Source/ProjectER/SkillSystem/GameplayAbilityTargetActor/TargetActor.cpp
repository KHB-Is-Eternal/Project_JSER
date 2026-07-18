// Fill out your copyright notice in the Description page of Project Settings.
// Force recompilation to clear UBT link cache


#include "SkillSystem/GameplayAbilityTargetActor/TargetActor.h"
#include "GameFramework/Actor.h"
#include "SkillSystem/GameAbility/MouseTargetSkill.h"
#include "SkillSystem/Actor/SkillIndicatorActor.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/Pawn.h"

ATargetActor::ATargetActor()
{
	PrimaryActorTick.bCanEverTick = true;
	bReplicates = false;
}

void ATargetActor::Setup(const FSkillIndicatorConfig& InIndicatorConfig, float InMaxRange)
{
	IndicatorConfig = InIndicatorConfig;
	MaxRange = InMaxRange;
}

void ATargetActor::StartTargeting(UGameplayAbility* Ability)
{
	Super::StartTargeting(Ability);
	SetActorTickEnabled(true);

	const bool bIsLocal = PrimaryPC && PrimaryPC->IsLocalPlayerController();
	if (bIsLocal)
	{
		// 1) 개별 방향/범위 조준선 스폰
		TSubclassOf<ASkillIndicatorActor> SpawnClass = IndicatorConfig.IndicatorClass.LoadSynchronous();
		if (SpawnClass != nullptr)
		{
			FActorSpawnParameters SpawnParams;
			SpawnParams.Owner = this;
			SpawnParams.Instigator = Cast<APawn>(Ability->GetAvatarActorFromActorInfo());
			SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

			SpawnedIndicator = GetWorld()->SpawnActor<ASkillIndicatorActor>(SpawnClass, FVector::ZeroVector, FRotator::ZeroRotator, SpawnParams);
			if (SpawnedIndicator != nullptr)
			{
				SpawnedIndicator->SetupIndicator(IndicatorConfig.IndicatorSize);
				SpawnedIndicator->SetLocationOffset(IndicatorConfig.LocationOffset);
				SpawnedIndicator->SetRotationOffset(IndicatorConfig.RotationOffset);
			}
		}
	}
}

void ATargetActor::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (SpawnedIndicator != nullptr)
	{
		SpawnedIndicator->Destroy();
		SpawnedIndicator = nullptr;
	}

	Super::EndPlay(EndPlayReason);
}

void ATargetActor::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	if (PrimaryPC != nullptr && PrimaryPC->GetPawn() != nullptr)
	{
		FVector CharacterLoc = PrimaryPC->GetPawn()->GetActorLocation();
		FRotator Rotation = PrimaryPC->GetPawn()->GetActorRotation();
		const float CurrentMaxRange = MaxRange;

		// 1) 방향선 업데이트
		if (SpawnedIndicator != nullptr)
		{
			SpawnedIndicator->UpdateIndicator(CharacterLoc, CharacterLoc, Rotation, CurrentMaxRange);
		}
	}
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