// Fill out your copyright notice in the Description page of Project Settings.
// Force recompilation to clear UBT link cache


#include "SkillSystem/GameplayAbilityTargetActor/MouseLocationTargetActor.h"
#include "SkillSystem/GameAbility/MouseClickSkill.h"
#include "SkillSystem/GameAbility/InstantSkill.h"
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
	PrimaryActorTick.bCanEverTick = false;
	bReplicates = false;
}

void AMouseLocationTargetActor::Setup(float InMaxRange)
{
	MaxRange = InMaxRange;
}

void AMouseLocationTargetActor::StartTargeting(UGameplayAbility* Ability)
{
	Super::StartTargeting(Ability);
	SetActorTickEnabled(false);
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

	/* 미사용 데드 코드 주석 처리 (조준선 및 마우스 위치 전달은 UGroundIndicatorComponent 및 TryConfirmMouseLocation에서 전담)
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
	}
	*/
}

void AMouseLocationTargetActor::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	Super::EndPlay(EndPlayReason);
}

bool AMouseLocationTargetActor::TryConfirmMouseLocation()
{
	// 1. 인스턴트 스킬인 경우 위치 검사 없이 즉시 확인 처리
	if (UInstantSkill* InstantSkill = Cast<UInstantSkill>(OwningAbility))
	{
		TargetDataReadyDelegate.Broadcast(FGameplayAbilityTargetDataHandle());
		return true;
	}

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