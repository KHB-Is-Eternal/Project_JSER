// Fill out your copyright notice in the Description page of Project Settings.


#include "SkillSystem/Actor/SkillIndicatorActor.h"
#include "SkillSystem/GameplayCueNotify/Components/GroundIndicatorComponent.h"
#include "Components/DecalComponent.h"

ASkillIndicatorActor::ASkillIndicatorActor()
{
	PrimaryActorTick.bCanEverTick = false;
	bReplicates = false;

	GroundIndicatorComp = CreateDefaultSubobject<UGroundIndicatorComponent>(TEXT("GroundIndicatorComp"));
	SetRootComponent(GroundIndicatorComp);

	DecalComp = CreateDefaultSubobject<UDecalComponent>(TEXT("DecalComp"));
	DecalComp->SetupAttachment(GroundIndicatorComp);
}

void ASkillIndicatorActor::BeginPlay()
{
	Super::BeginPlay();
}

void ASkillIndicatorActor::SetupIndicatorSize(const FVector& InSize)
{
	if (DecalComp != nullptr)
	{
		DecalComp->DecalSize = InSize;
	}
}

void ASkillIndicatorActor::UpdateIndicator(const FVector& InTargetLocation, const FRotator& InTargetRotation, float InDistanceToTarget)
{
}

// ==========================================
// ALocationIndicatorActor (위치 추적용)
// ==========================================

void ALocationIndicatorActor::SetupIndicatorSize(const FVector& InSize)
{
	if (DecalComp != nullptr)
	{
		// 원형 장판 등은 가로세로 반경(Y, Z)을 덮어씁니다. X는 투영 깊이입니다.
		DecalComp->DecalSize = FVector(DecalComp->DecalSize.X, InSize.Y, InSize.Z);
	}
}

void ALocationIndicatorActor::UpdateIndicator(const FVector& InTargetLocation, const FRotator& InTargetRotation, float InDistanceToTarget)
{
	SetActorLocation(InTargetLocation);
}

// ==========================================
// ADirectionIndicatorActor (방향 추적용)
// ==========================================

void ADirectionIndicatorActor::SetupIndicatorSize(const FVector& InSize)
{
	if (DecalComp != nullptr)
	{
		// 방향 화살표 등은 길이(X)와 폭(Y)을 개별적으로 제어합니다.
		DecalComp->DecalSize = FVector(InSize.X, InSize.Y, DecalComp->DecalSize.Z);
	}
}

void ADirectionIndicatorActor::UpdateIndicator(const FVector& InTargetLocation, const FRotator& InTargetRotation, float InDistanceToTarget)
{
	SetActorRotation(InTargetRotation);
}
