// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "SkillIndicatorActor.generated.h"

class UGroundIndicatorComponent;
class UDecalComponent;

UCLASS()
class PROJECTER_API ASkillIndicatorActor : public AActor
{
	GENERATED_BODY()
	
public: 
	ASkillIndicatorActor();

protected:
	virtual void BeginPlay() override;

public: 
	/** 조준선의 절대 크기를 세팅하는 가상함수 */
	virtual void SetupIndicatorSize(const FVector& InSize);

	/** 매 프레임 타겟 데이터를 갱신받아 행동하는 가상함수 */
	virtual void UpdateIndicator(const FVector& InTargetLocation, const FRotator& InTargetRotation, float InDistanceToTarget);

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Indicator", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UGroundIndicatorComponent> GroundIndicatorComp;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Indicator", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UDecalComponent> DecalComp;
};

/**
 * 위치 추적용 파생 클래스 (원형 장판 용도)
 */
UCLASS()
class PROJECTER_API ALocationIndicatorActor : public ASkillIndicatorActor
{
	GENERATED_BODY()

public:
	virtual void SetupIndicatorSize(const FVector& InSize) override;
	virtual void UpdateIndicator(const FVector& InTargetLocation, const FRotator& InTargetRotation, float InDistanceToTarget) override;
};

/**
 * 방향 추적용 파생 클래스 (부채꼴, 화살표 장판 용도)
 */
UCLASS()
class PROJECTER_API ADirectionIndicatorActor : public ASkillIndicatorActor
{
	GENERATED_BODY()

public:
	virtual void SetupIndicatorSize(const FVector& InSize) override;
	virtual void UpdateIndicator(const FVector& InTargetLocation, const FRotator& InTargetRotation, float InDistanceToTarget) override;
};
