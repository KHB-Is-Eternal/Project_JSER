// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "Engine/EngineTypes.h"
#include "SkillSystem/GameplayEffectComponent/BaseGEC.h"

class UBaseGameplayEffect;
class USkillNiagaraSpawnConfig;
class USkillSoundSpawnConfig;
struct FGameplayEffectSpec;
struct FActiveGameplayEffectsContainer;
struct FPredictionKey;

#include "MoveBaseGEC.generated.h"

// 이동 방향 결정 방식
UENUM(BlueprintType)
enum class EMoveDirectionSource : uint8
{
	Forward       UMETA(DisplayName = "캐릭터 전방"),
	TowardContext UMETA(DisplayName = "Context Origin 방향"),
	TowardTarget  UMETA(DisplayName = "Target Actor 방향"),
};

UCLASS(Abstract)
class PROJECTER_API UMoveBaseGEC : public UBaseGEC
{
	GENERATED_BODY()

public:
	UMoveBaseGEC();

protected:
	virtual void OnGameplayEffectApplied(FActiveGameplayEffectsContainer& ActiveGEContainer, FGameplayEffectSpec& GESpec, FPredictionKey& PredictionKey) const override;

	// 파생 클래스에서 이동 방식별 구현 (순수 가상)
	virtual void Execute(AActor* Instigator, const FVector& Direction, const FGameplayEffectSpec& GESpec) const PURE_VIRTUAL(UMoveBaseGEC::Execute, );

	// 이동 소요 시간 반환 (애니메이션 동기화용)
	virtual float CalculateMoveDuration(const FGameplayEffectSpec& GESpec, const AActor* Instigator, const FVector& Direction) const PURE_VIRTUAL(UMoveBaseGEC::CalculateMoveDuration, return 0.15f;);

	// 공통 유틸리티 함수
	bool IsRootMotionActive(const AActor* Actor) const;

	FVector CalculateMoveDirection(const FGameplayEffectSpec& GESpec, const AActor* Instigator) const;

	// 컨텍스트 위치와 MoveDistance를 고려한 최종 타겟 위치 계산
	FVector CalculateTargetLocation(const FGameplayEffectSpec& GESpec, const AActor* Instigator) const;

	void HandleWallHit(AActor* Instigator, const FHitResult& Hit, const FGameplayEffectSpec& GESpec) const;

	void SnapToGround(FVector& InOutLocation, const AActor* Instigator) const;
	
	// 활성 몽타주 속도 조정
	void AdjustActiveMontageRate(ACharacter* Character, float MoveDuration) const;

	// 유닛 충돌 채널 설정/복구 헬퍼
	void SetPawnCollisionIgnore(ACharacter* Character, bool bIgnore) const;

public:
	// --- 공통 이동 설정 ---
	UPROPERTY(EditDefaultsOnly, Category = "Move|Base")
	EMoveDirectionSource DirectionSource = EMoveDirectionSource::Forward;

	UPROPERTY(EditDefaultsOnly, Category = "Move|Base")
	float MoveDistance = 500.0f;

	// --- 안전 설정 ---
	UPROPERTY(EditDefaultsOnly, Category = "Move|Safety")
	bool bIgnoreIfRootMotion = true;

	UPROPERTY(EditDefaultsOnly, Category = "Move|Safety")
	bool bIgnoreUnitCollision = false;

	// 컨텍스트(TargetData 등) 위치가 존재하고 이동 거리 이내라면 해당 위치를 우선 사용
	UPROPERTY(EditDefaultsOnly, Category = "Move|Safety")
	bool bPreferContextLocation = true;

	// 지면 추적 거리 고정 (에디터 수정 불가)
	UPROPERTY(VisibleDefaultsOnly, Category = "Move|Safety")
	float GroundTraceDistance = 500.0f;

	UPROPERTY(EditDefaultsOnly, Category = "Move|Safety")
	TEnumAsByte<ECollisionChannel> GroundTraceChannel = ECC_Visibility;

	// --- Wall Hit (벽꿍) ---
	UPROPERTY(EditDefaultsOnly, Category = "Move|WallHit")
	bool bDetectWallHit = false;

	UPROPERTY(EditDefaultsOnly, Category = "Move|WallHit",
		meta = (EditCondition = "bDetectWallHit"))
	TArray<TSubclassOf<UBaseGameplayEffect>> WallHitApplied;

	// --- Animation ---
	// 활성 몽타주 속도 조정 여부
	UPROPERTY(EditDefaultsOnly, Category = "Move|Animation")
	bool bAdjustMontageRate = true;

	// 최소 PlayRate 제한
	UPROPERTY(EditDefaultsOnly, Category = "Move|Animation",
		meta = (EditCondition = "bAdjustMontageRate"))
	float MinPlayRate = 0.5f;

	// 최대 PlayRate 제한
	UPROPERTY(EditDefaultsOnly, Category = "Move|Animation",
		meta = (EditCondition = "bAdjustMontageRate"))
	float MaxPlayRate = 3.0f;
};
