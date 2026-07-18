// Fill out your copyright notice in the Description page of Project Settings.
// Force recompilation to clear UBT link cache


#include "SkillSystem/GameplayAbilityTargetActor/MouseLocationTargetActor.h"
#include "SkillSystem/GameAbility/MouseClickSkill.h"
#include "SkillSystem/Actor/SkillIndicatorActor.h"
#include "Abilities/GameplayAbilityTargetTypes.h"
#include "Kismet/KismetMathLibrary.h"
#include "Engine/World.h"

namespace
{
	FGameplayAbilityTargetDataHandle MakeLocationTargetData(const FVector& Location)
	{
		FGameplayAbilityTargetDataHandle DataHandle;
		FGameplayAbilityTargetData_LocationInfo* LocData = new FGameplayAbilityTargetData_LocationInfo();
		LocData->TargetLocation.LocationType = EGameplayAbilityTargetingLocationType::LiteralTransform;
		LocData->TargetLocation.LiteralTransform = FTransform(Location);
		DataHandle.Add(LocData);
		return DataHandle;
	}
}

AMouseLocationTargetActor::AMouseLocationTargetActor()
{
	PrimaryActorTick.bCanEverTick = true;
	bReplicates = false;
}

void AMouseLocationTargetActor::Setup(const FSkillIndicatorConfig& InIndicatorConfig, float InMaxRange)
{
	IndicatorConfig = InIndicatorConfig;
	MaxRange = InMaxRange;
}

void AMouseLocationTargetActor::StartTargeting(UGameplayAbility* Ability)
{
	Super::StartTargeting(Ability);
	SetActorTickEnabled(true);

	const bool bIsLocal = PrimaryPC && PrimaryPC->IsLocalPlayerController();
	if (bIsLocal)
	{
		// 🌟 1) 마우스 방향/궤적 지시 조준선 스폰
		TSubclassOf<ASkillIndicatorActor> DirectionSpawnClass = IndicatorConfig.IndicatorClass.LoadSynchronous();
		if (DirectionSpawnClass != nullptr)
		{
			FActorSpawnParameters SpawnParams;
			SpawnParams.Owner = this;
			SpawnParams.Instigator = Cast<APawn>(Ability->GetAvatarActorFromActorInfo());
			SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

			SpawnedIndicator = GetWorld()->SpawnActor<ASkillIndicatorActor>(DirectionSpawnClass, FVector::ZeroVector, FRotator::ZeroRotator, SpawnParams);
			if (SpawnedIndicator != nullptr)
			{
				SpawnedIndicator->SetupIndicator(IndicatorConfig.IndicatorSize);
				SpawnedIndicator->SetLocationOffset(IndicatorConfig.LocationOffset);
				SpawnedIndicator->SetRotationOffset(IndicatorConfig.RotationOffset);
			}
		}
	}
}

void AMouseLocationTargetActor::ConfirmTargetingAndContinue()
{
	if (TryConfirmMouseLocation() == false)
	{
		FGameplayAbilityTargetDataHandle CancelHandle;
		CanceledDelegate.Broadcast(CancelHandle);
	}
}

void AMouseLocationTargetActor::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	UMouseClickSkill* MouseClickSkill = Cast<UMouseClickSkill>(OwningAbility);
	AActor* Avatar = IsValid(MouseClickSkill) ? MouseClickSkill->GetAvatarActorFromActorInfo() : nullptr;

	if (IsValid(MouseClickSkill) && IsValid(Avatar))
	{
		FVector CharacterLoc = Avatar->GetActorLocation();
		FVector MouseLoc = MouseClickSkill->GetMouseLocation();
		MouseLoc.Z = CharacterLoc.Z;

		FVector Dir = MouseLoc - CharacterLoc;
		float Distance = Dir.Size();
		Dir.Normalize();

		FVector TargetLocation = MouseLoc;
		const float CurrentMaxRange = MaxRange;

		// 사거리 한계 제한
		if (CurrentMaxRange > 0.f && Distance > CurrentMaxRange)
		{
			TargetLocation = CharacterLoc + Dir * CurrentMaxRange;
		}

		FRotator Rotation = UKismetMathLibrary::FindLookAtRotation(CharacterLoc, TargetLocation);
		float TargetDistance = (CurrentMaxRange > 0.f) ? FMath::Min(Distance, CurrentMaxRange) : Distance;

		// 🌟 1) 방향선 업데이트 (마우스 위치 추적 및 신축)
		if (SpawnedIndicator != nullptr)
		{
			SpawnedIndicator->UpdateIndicator(CharacterLoc, TargetLocation, Rotation, TargetDistance);
		}
	}
}

void AMouseLocationTargetActor::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	// 인디케이터 액터들 소멸 보장
	if (SpawnedIndicator != nullptr)
	{
		SpawnedIndicator->Destroy();
		SpawnedIndicator = nullptr;
	}

	Super::EndPlay(EndPlayReason);
}

bool AMouseLocationTargetActor::TryConfirmMouseLocation()
{
	UMouseClickSkill* MouseClickSkill = Cast<UMouseClickSkill>(OwningAbility);
	if (!IsValid(MouseClickSkill)) return false;

	FVector MouseLocation = FVector::ZeroVector;
	if (!MouseClickSkill->TryGetMouseLocationInRange(MouseLocation)) return false;

	TargetDataReadyDelegate.Broadcast(MakeLocationTargetData(MouseLocation));
	return true;
}

bool AMouseLocationTargetActor::SubmitExternalLocation(const FVector& InLocation)
{
	UMouseClickSkill* MouseClickSkill = Cast<UMouseClickSkill>(OwningAbility);
	if (!IsValid(MouseClickSkill)) return false;
	if (!MouseClickSkill->IsTargetLocationInRange(InLocation)) return false;

	TargetDataReadyDelegate.Broadcast(MakeLocationTargetData(InLocation));
	return true;
}