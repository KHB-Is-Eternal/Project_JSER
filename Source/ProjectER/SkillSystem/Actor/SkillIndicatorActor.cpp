// Fill out your copyright notice in the Description page of Project Settings.


#include "SkillSystem/Actor/SkillIndicatorActor.h"
#include "SkillSystem/GameplayCueNotify/Components/GroundIndicatorComponent.h"
#include "Materials/MaterialInstanceDynamic.h"

ASkillIndicatorActor::ASkillIndicatorActor()
{
	PrimaryActorTick.bCanEverTick = false;
	bReplicates = false;

	SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
	SetRootComponent(SceneRoot);

	GroundIndicatorComp = CreateDefaultSubobject<UGroundIndicatorComponent>(TEXT("GroundIndicatorComp"));
	GroundIndicatorComp->SetupAttachment(SceneRoot);

	// 독립 조준선 액터로서 실시간 바닥 트래킹을 수행하도록 컴포넌트 옵션을 강제 설정
	GroundIndicatorComp->SetTrackingDynamicGround(true);
}

void ASkillIndicatorActor::BeginPlay()
{
	// 에디터에서 할당한 머티리얼이 존재하면 동적 머티리얼 인스턴스(MID)를 자동 생성하여 내장 메쉬 0번 슬롯에 적용
	if (GroundIndicatorComp != nullptr && IndicatorMaterial != nullptr)
	{
		IndicatorMID = UMaterialInstanceDynamic::Create(IndicatorMaterial, this);
		if (IndicatorMID != nullptr)
		{
			GroundIndicatorComp->SetIndicatorMaterial(0, IndicatorMID);
		}
	}

	Super::BeginPlay();
}

void ASkillIndicatorActor::SetupIndicator(const FVector& InSize)
{
	if (GroundIndicatorComp != nullptr)
	{
		// 다른 장판들의 원래 기획된 월드 스크린 크기를 보장하기 위해 50.f 스케일로 최종 복구
		GroundIndicatorComp->SetIndicatorScale(InSize / 50.f);
	}

	if (IndicatorMID != nullptr)
	{
		IndicatorMID->SetScalarParameterValue(TEXT("SizeX"), InSize.X);
		IndicatorMID->SetScalarParameterValue(TEXT("SizeY"), InSize.Y);
		IndicatorMID->SetScalarParameterValue(TEXT("SizeZ"), InSize.Z);
	}
}

void ASkillIndicatorActor::SetLocationOffset(const FVector& InOffset)
{
	LocationOffset = InOffset;
}

void ASkillIndicatorActor::SetRotationOffset(const FRotator& InOffset)
{
	RotationOffset = InOffset;
}

void ASkillIndicatorActor::UpdateIndicator(
	const FVector& InCharacterLocation, 
	const FVector& InTargetLocation, 
	const FRotator& InTargetRotation, 
	float InDistanceToTarget)
{
	// 0. 회전 기준값 선행 계산 (로컬 오프셋 방향 산출용)
	const FRotator BaseRotation = (RotationType == ESkillIndicatorRotationType::LookAtMouse) 
		? (InTargetRotation + RotationOffset) 
		: (FRotator::ZeroRotator + RotationOffset);

	const FVector BaseLocation = (PositionType == ESkillIndicatorPositionType::Character) 
		? InCharacterLocation 
		: InTargetLocation;

	// X: 전방, Y: 우측, Z: 상방 기준으로 로컬 오프셋을 월드 위치로 변환하여 더함
	const FVector FinalLocation = BaseLocation + BaseRotation.RotateVector(LocationOffset);

	// 1. 위치 동기화 (C++ 강제 보장, 블루프린트 오버라이딩에 영향 없음)
	SetActorLocation(FinalLocation);

	// 2. 회전 동기화 (C++ 강제 보장, 블루프린트 오버라이딩에 영향 없음)
	SetActorRotation(BaseRotation);

	// 틱 순서 어긋남 방지: 위치가 덮어씌워진 즉시 실시간으로 지면 밀착 높이를 즉각 갱신
	if (GroundIndicatorComp != nullptr)
	{
		GroundIndicatorComp->UpdateGroundPosition();
	}

	// // 🌟 [안전장치] 머티리얼 인스턴스의 Length 파라미터 값을 C++에서 실시간으로 직접 강제 주입
	// if (IndicatorMID != nullptr)
	// {
	// 	IndicatorMID->SetScalarParameterValue(TEXT("Length"), InDistanceToTarget);
	// }

	// 3. 블루프퍼린트 전용 수축 업데이트 이벤트 호출 (비주얼 파라미터 제어 기회 제공)
	BP_OnUpdateIndicator(InDistanceToTarget);
}
