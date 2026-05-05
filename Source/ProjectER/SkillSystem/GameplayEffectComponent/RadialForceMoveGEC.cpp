// Fill out your copyright notice in the Description page of Project Settings.

#include "SkillSystem/GameplayEffectComponent/RadialForceMoveGEC.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/RootMotionSource.h"
#include "AbilitySystemComponent.h"

URadialForceMoveGEC::URadialForceMoveGEC()
{
}

float URadialForceMoveGEC::CalculateMoveDuration(const FGameplayEffectSpec& GESpec, const AActor* Instigator, const FVector& Direction) const
{
	return this->Duration;
}

void URadialForceMoveGEC::Execute(AActor* Instigator, const FVector& Direction, const FGameplayEffectSpec& GESpec, FPredictionKey PredictionKey) const
{
	ACharacter* const Character = Cast<ACharacter>(Instigator);
	if (!IsValid(Character))
	{
		return;
	}

	UCharacterMovementComponent* const CMC = Character->GetCharacterMovement();
	if (!IsValid(CMC))
	{
		return;
	}

	const FGameplayEffectContextHandle& Context = GESpec.GetEffectContext();
	FVector Origin = Context.HasOrigin() ? Context.GetOrigin() : Instigator->GetActorLocation();

	TSharedPtr<FRootMotionSource_RadialForce> RadialForce = MakeShared<FRootMotionSource_RadialForce>();
	
	// 예측 키 동기화
	FString KeyStr = PredictionKey.IsValidKey() ? FString::FromInt(PredictionKey.Current) : TEXT("NoKey");
	RadialForce->InstanceName = FName(*FString::Printf(TEXT("RadialForceMoveGEC_%s"), *KeyStr));
	
	RadialForce->AccumulateMode = ERootMotionAccumulateMode::Additive; // 광역 힘은 중첩 가능하도록 Additive 권장
	RadialForce->Priority = 5;
	
	// Radial Force Parameters
	RadialForce->Location = Origin;
	RadialForce->Radius = this->Radius;
	RadialForce->Strength = this->Strength;
	RadialForce->Duration = this->Duration;
	RadialForce->bIsPush = this->bIsPush;
	RadialForce->bNoZForce = this->bNoZForce;
	RadialForce->StrengthDistanceFalloff = this->StrengthDistanceFalloff;
	
	// 공통 종료 설정
	RadialForce->FinishVelocityParams.Mode = this->FinishVelocityMode;
	RadialForce->FinishVelocityParams.SetVelocity = this->FinishSetVelocity;
	RadialForce->FinishVelocityParams.ClampVelocity = this->FinishClampVelocity;

	CMC->ApplyRootMotionSource(RadialForce);

	// 유닛 충돌 무시 처리
	if (this->bIgnoreUnitCollision)
	{
		SetPawnCollisionIgnore(Character, true);
		
		TWeakObjectPtr<URadialForceMoveGEC const> WeakThis = this;
		TWeakObjectPtr<ACharacter> WeakChar = Character;
		FTimerHandle RestoreTimer;
		Character->GetWorld()->GetTimerManager().SetTimer(
			RestoreTimer,
			[WeakThis, WeakChar]()
			{
				if (WeakThis.IsValid() && WeakChar.IsValid())
				{
					WeakThis->SetPawnCollisionIgnore(WeakChar.Get(), false);
				}
			},
			this->Duration + 0.1f,
			false);
	}
}
