// Fill out your copyright notice in the Description page of Project Settings.

#include "SkillSystem/GameplayEffectComponent/RadialForceMoveGEC.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/RootMotionSource.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystemGlobals.h"

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

	// --- 이펙트 종료 타이머 ---
	TWeakObjectPtr<URadialForceMoveGEC const> WeakThis = this;
	TWeakObjectPtr<ACharacter> WeakChar = Character;

	FTimerHandle EffectTimer;
	Character->GetWorld()->GetTimerManager().SetTimer(
		EffectTimer,
		[WeakThis, WeakChar, GESpec, PredictionKey]()
		{
			if (WeakThis.IsValid() && WeakChar.IsValid())
			{
				if (UAbilitySystemComponent* ASC = UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(WeakChar.Get()))
				{
					WeakThis->RemoveMoveCue(ASC, WeakThis->LoopVfxConfig, WeakThis->LoopSfxConfig);
					WeakThis->ExecuteMoveCue(ASC, GESpec, WeakThis->EndVfxConfig, WeakThis->EndSfxConfig, PredictionKey);
				}
			}
		},
		this->Duration,
		false);

	// 유닛 충돌 무시 처리
	if (this->bIgnoreUnitCollision)
	{
		SetPawnCollisionIgnore(Character, true);

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

FSkillTooltipData URadialForceMoveGEC::GetTooltipDescription(int32 Level, TSubclassOf<class USkillBase> AbilityClass) const
{
	FSkillTooltipData Data;
	if (bIsPush)
	{
		Data.ShortDescription = FText::FromString(TEXT("대상을 밀쳐냅니다."));
		FString DetailStr = TEXT("밀침 : 대상을 밀쳐냅니다.");
		if (bDetectWallHit && WallHitApplied.Num() > 0)
		{
			DetailStr += TEXT("\n\n벽과 충돌 시 추가 효과가 적용됩니다.");
			FText WallHitText = FormatAppliedEffects(WallHitApplied, Level);
			if (!WallHitText.IsEmpty())
			{
				DetailStr += TEXT("\n") + WallHitText.ToString();
			}
		}
		Data.DetailedDescription = FText::FromString(DetailStr);
	}
	else
	{
		Data.ShortDescription = FText::FromString(TEXT("대상을 당깁니다."));
		FString DetailStr = TEXT("당김 : 대상을 당깁니다.");
		if (bDetectWallHit && WallHitApplied.Num() > 0)
		{
			DetailStr += TEXT("\n\n벽과 충돌 시 추가 효과가 적용됩니다.");
			FText WallHitText = FormatAppliedEffects(WallHitApplied, Level);
			if (!WallHitText.IsEmpty())
			{
				DetailStr += TEXT("\n") + WallHitText.ToString();
			}
		}
		Data.DetailedDescription = FText::FromString(DetailStr);
	}
	return Data;
}
