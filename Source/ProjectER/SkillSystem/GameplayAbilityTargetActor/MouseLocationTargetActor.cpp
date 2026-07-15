// Fill out your copyright notice in the Description page of Project Settings.


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

void AMouseLocationTargetActor::Setup(const FSkillIndicatorConfig& InConfig, float InMaxRange)
{
	IndicatorConfig = InConfig;
	MaxRange = InMaxRange;
}

void AMouseLocationTargetActor::StartTargeting(UGameplayAbility* Ability)
{
	Super::StartTargeting(Ability);
	SetActorTickEnabled(true); // 틱 시동 강제화

	// 로컬 플레이어 컨트롤러에서만 조준선 생성 (서버 리소스 방어)
	const bool bIsLocal = PrimaryPC && PrimaryPC->IsLocalPlayerController();
	if (bIsLocal)
	{
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

	if (SpawnedIndicator != nullptr)
	{
		UMouseClickSkill* MouseClickSkill = Cast<UMouseClickSkill>(OwningAbility);
		AActor* Avatar = IsValid(MouseClickSkill) ? MouseClickSkill->GetAvatarActorFromActorInfo() : nullptr;

		if (IsValid(MouseClickSkill) && IsValid(Avatar))
		{
			FVector CharacterLoc = Avatar->GetActorLocation();
			FVector MouseLoc = MouseClickSkill->GetMouseLocation();

			// 캐릭터 평면과 맞추기 위해 Z 좌표 동기화 (평면상 순수 거리 계산용)
			MouseLoc.Z = CharacterLoc.Z;

			FVector Dir = MouseLoc - CharacterLoc;
			float Distance = Dir.Size();
			Dir.Normalize();

			FVector TargetLocation = MouseLoc;

			// 사거리 한계 제한 (MaxRange가 0보다 클 때만 클램프 적용)
			if (MaxRange > 0.f && Distance > MaxRange)
			{
				TargetLocation = CharacterLoc + Dir * MaxRange;
			}

			// 캐릭터에서 조준점(클램프 적용됨)을 바라보는 회전각 계산
			FRotator Rotation = UKismetMathLibrary::FindLookAtRotation(CharacterLoc, TargetLocation);

			// 머터리얼의 동적 길이용 파라미터는 사거리 제한 내의 최종 유효 거리를 전달
			float TargetDistance = (MaxRange > 0.f) ? FMath::Min(Distance, MaxRange) : Distance;

			// 캐릭터의 위치 및 조준점 위치, 회전, 신축거리를 모두 패스 (인디케이터가 자율 제어)
			SpawnedIndicator->UpdateIndicator(CharacterLoc, TargetLocation, Rotation, TargetDistance);
		}
	}
}

void AMouseLocationTargetActor::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	// 인디케이터 액터 소멸 보장 (메모리 누수 방지)
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