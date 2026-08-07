// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "SkillIndicatorActor.generated.h"

class UGroundIndicatorComponent;
class UMaterialInstanceDynamic;
class APlayerController;

UENUM(BlueprintType)
enum class ESkillIndicatorPositionType : uint8
{
	Character,   // 시전자 발밑 고정
	Mouse        // 마우스 조준점 추적
};

UENUM(BlueprintType)
enum class ESkillIndicatorRotationType : uint8
{
	None,            // 회전 없음 (기본값)
	LookAtMouse,     // 시전자 -> 마우스 조준점을 바라보도록 회전
	CharacterForward // 캐릭터의 현재 정면(바라보는 방향)을 기준으로 회전 고정
};

UCLASS()
class PROJECTER_API ASkillIndicatorActor : public AActor
{
	GENERATED_BODY()
	
public: 
	ASkillIndicatorActor();

protected:
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;

public: 
	/** 어빌리티에서 초기화 정보를 주입받아 자율 틱을 시작하도록 설정합니다. */
	void InitializeIndicator(AActor* InAvatar, float InMaxRange);
	/** 내장 메쉬 크기를 다이렉트로 대입하는 함수 (C++ 강제 보장) */
	virtual void SetupIndicator(const FVector& InSize);

	/** 외부(데이터 에셋 등)에서 실시간 위치 오프셋을 주입하는 함수 */
	UFUNCTION(BlueprintCallable, Category = "Indicator")
	void SetLocationOffset(const FVector& InOffset);

	/** 외부(데이터 에셋 등)에서 회전 오프셋을 주입하는 함수 */
	UFUNCTION(BlueprintCallable, Category = "Indicator")
	void SetRotationOffset(const FRotator& InOffset);

	/** 내부 동적 머티리얼 인스턴스(MID)를 안전하게 반환하는 Getter */
	UFUNCTION(BlueprintPure, Category = "Indicator")
	FORCEINLINE UMaterialInstanceDynamic* GetIndicatorMID() const { return IndicatorMID; }

	/** 매 프레임 타겟 데이터를 갱신받아 행동하는 함수 (C++ 위치/회전 강제 보장) */
	void UpdateIndicator(const FVector& InCharacterLocation, const FVector& InTargetLocation, const FRotator& InTargetRotation, float InDistanceToTarget);

protected:
	/** 블루프린트에서 필요한 가변 머티리얼 파라미터(Length 등)를 수신받아 처리할 수 있게 하는 이벤트 */
	UFUNCTION(BlueprintImplementableEvent, Category = "Indicator")
	void BP_OnUpdateIndicator(float InDistanceToTarget);

protected:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Indicator Settings")
	ESkillIndicatorPositionType PositionType = ESkillIndicatorPositionType::Character;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Indicator Settings")
	ESkillIndicatorRotationType RotationType = ESkillIndicatorRotationType::LookAtMouse;

	/** 아티스트가 제작한 텍스처 방향 오정렬을 보정하기 위한 회전 오프셋 */
	UPROPERTY(BlueprintReadOnly, Category = "Indicator Settings")
	FRotator RotationOffset = FRotator::ZeroRotator;

	/** 조준선 장판의 위치를 보정하기 위한 로컬 오프셋 (X: 전방, Y: 우측, Z: 상방) */
	UPROPERTY(BlueprintReadOnly, Category = "Indicator Settings")
	FVector LocationOffset = FVector::ZeroVector;

	/** 조준선 장판에 사용할 원본 머티리얼 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Indicator Settings")
	TObjectPtr<UMaterialInterface> IndicatorMaterial;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Indicator Components", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<USceneComponent> SceneRoot;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Indicator Components", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UGroundIndicatorComponent> GroundIndicatorComp;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Indicator Components", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UMaterialInstanceDynamic> IndicatorMID;

protected:
	UPROPERTY()
	TObjectPtr<AActor> AvatarActor;

	UPROPERTY()
	TObjectPtr<APlayerController> PC;

	float MaxRange = 0.f;
};
