// Fill out your copyright notice in the Description page of Project Settings.

#include "SkillSystem/GameplayEffectComponent/LaunchMoveGEC.h"

#include "Components/CapsuleComponent.h"
#include "GameplayEffect.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "SkillSystem/GameplayCueNotify/SkillNiagaraSpawnConfig.h"

ULaunchMoveGEC::ULaunchMoveGEC()
{
}

float ULaunchMoveGEC::CalculateMoveDuration(const FGameplayEffectSpec& GESpec, const AActor* Instigator, const FVector& Direction) const
{
	const ACharacter* const Character = Cast<ACharacter>(Instigator);
	if (!IsValid(Character))
	{
		return 0.25f;
	}

	const FVector TargetLoc = CalculateTargetLocation(GESpec, Instigator);
	const float Distance = FVector::Dist(Instigator->GetActorLocation(), TargetLoc);

	if (this->VerticalLaunchSpeed > 0.0f)
	{
		const UCharacterMovementComponent* const CMC = Character->GetCharacterMovement();
		const float Gravity = IsValid(CMC) ? -CMC->GetGravityZ() : 980.0f;
		if (Gravity > 0.0f)
		{
			// t = 2 * Vz / g
			return (2.0f * this->VerticalLaunchSpeed) / Gravity;
		}
	}

	return 0.25f; // 지면 발사 기본 예상 시간
}

void ULaunchMoveGEC::Execute(AActor* Instigator, const FVector& Direction, const FGameplayEffectSpec& GESpec) const
{
	ACharacter* const Character = Cast<ACharacter>(Instigator);
	if (!IsValid(Character))
	{
		return;
	}

	// 발사 속도 계산
	const FVector TargetLoc = CalculateTargetLocation(GESpec, Instigator);
	const float Distance = FVector::Dist(Instigator->GetActorLocation(), TargetLoc);

	float HorizontalSpeed = 0.0f;
	const float VerticalSpeed = this->VerticalLaunchSpeed;

	if (VerticalSpeed > 0.0f)
	{
		// 1. 도약 (Leap): 중력과 수직 속도를 이용해 체공 시간을 구하고, MoveDistance에 낙하하도록 수평 속도 계산
		UCharacterMovementComponent* const CMC = Character->GetCharacterMovement();
		const float Gravity = IsValid(CMC) ? -CMC->GetGravityZ() : 980.0f;

		if (Gravity > 0.0f)
		{
			// t = 2 * Vz / g (올라갔다 내려오는 시간)
			const float TimeInAir = (2.0f * VerticalSpeed) / Gravity;
			HorizontalSpeed = (TimeInAir > 0.05f) ? (Distance / TimeInAir) : 0.0f;
		}
	}
	else
	{
		// 2. 지면 발사: 특정 예상 도달 시간(예: 0.25초)을 기준으로 초기 속도 부여
		const float TargetTime = 0.25f;
		HorizontalSpeed = Distance / TargetTime;
	}

	// 최종 속도 벡터 생성
	const FVector LaunchVelocity = (Direction * HorizontalSpeed) + (FVector::UpVector * VerticalSpeed);

	// 예상 이동 시간 계산 (타이머용)
	const float PredictDuration = (HorizontalSpeed > 0.0f) ? (Distance / HorizontalSpeed) : 0.5f;

	// 유닛 충돌 무시 (예상 이동 시간 동안)
	if (this->bIgnoreUnitCollision)
	{
		SetPawnCollisionIgnore(Character, true);

		TWeakObjectPtr<ULaunchMoveGEC const> WeakThis = this;
		TWeakObjectPtr<ACharacter> WeakChar = Character;
		FTimerHandle RestoreTimer;
		Character->GetWorld()->GetTimerManager().SetTimer(
			RestoreTimer,
			[WeakThis, WeakChar]()
			{
				if (WeakThis.IsValid() && WeakChar.IsValid())
				{
					// 충돌 무시는 서버에서만 제어
					if (WeakThis->bIgnoreUnitCollision && WeakChar->HasAuthority())
					{
						WeakThis->SetPawnCollisionIgnore(WeakChar.Get(), false);
					}
				}
			},
			PredictDuration,
			false);
	}

    // 캐릭터 상태 변경 (확실히 뜨게 함)
    if (VerticalSpeed > 0.0f || this->bZOverride)
    {
        if (UCharacterMovementComponent *CMC = Character->GetCharacterMovement())
        {
            CMC->SetMovementMode(MOVE_Falling);
        }
    }

    Character->LaunchCharacter(LaunchVelocity, this->bXYOverride, this->bZOverride);
}
