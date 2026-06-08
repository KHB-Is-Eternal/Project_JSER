// Fill out your copyright notice in the Description page of Project Settings.

#include "SkillSystem/GameplayEffectComponent/JumpForceMoveGEC.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/RootMotionSource.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystemGlobals.h"

UJumpForceMoveGEC::UJumpForceMoveGEC()
{
}

float UJumpForceMoveGEC::CalculateMoveDuration(const FGameplayEffectSpec& GESpec, const AActor* Instigator, const FVector& Direction) const
{
	return this->JumpDuration > 0.0f ? this->JumpDuration : 0.5f;
}

void UJumpForceMoveGEC::Execute(AActor* Instigator, const FVector& Direction, const FGameplayEffectSpec& GESpec, FPredictionKey PredictionKey) const
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

	const FVector TargetLoc = CalculateTargetLocation(GESpec, Instigator);
	const float Distance = FVector::Dist(Instigator->GetActorLocation(), TargetLoc);
	const FRotator JumpRotation = Direction.Rotation();

	TSharedPtr<FRootMotionSource_JumpForce> JumpForce = MakeShared<FRootMotionSource_JumpForce>();
	
	// 예측 키 동기화
	FString KeyStr = PredictionKey.IsValidKey() ? FString::FromInt(PredictionKey.Current) : TEXT("NoKey");
	JumpForce->InstanceName = FName(*FString::Printf(TEXT("JumpForceMoveGEC_%s"), *KeyStr));
	
	JumpForce->AccumulateMode = ERootMotionAccumulateMode::Override;
	JumpForce->Priority = 5;
	
	// Jump Parameters
	JumpForce->Rotation = JumpRotation;
	JumpForce->Distance = Distance;
	JumpForce->Height = this->JumpHeight;
	JumpForce->Duration = this->JumpDuration;
	JumpForce->PathOffsetCurve = this->PathOffsetCurve;
	
	// 공통 종료 설정
	JumpForce->FinishVelocityParams.Mode = this->FinishVelocityMode;
	JumpForce->FinishVelocityParams.SetVelocity = this->FinishSetVelocity;
	JumpForce->FinishVelocityParams.ClampVelocity = this->FinishClampVelocity;

	// 도약 전 이동 모드 변경 (지면 마찰 무시 및 공중 물리 적용)
	CMC->SetMovementMode(MOVE_Falling);

	CMC->ApplyRootMotionSource(JumpForce);

	// --- 이펙트 종료 타이머 ---
	TWeakObjectPtr<UJumpForceMoveGEC const> WeakThis = this;
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
		this->JumpDuration,
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
			this->JumpDuration + 0.1f,
			false);
	}
}

FSkillTooltipData UJumpForceMoveGEC::GetTooltipDescription(int32 Level, TSubclassOf<class USkillBase> AbilityClass) const
{
	FSkillTooltipData Data;
	Data.ShortDescription = FText::FromString(TEXT("공중으로 도약시킵니다."));

	FString DetailStr = FString::Printf(TEXT("도약 : %.0f 높이로 공중 도약하여 대상을 이동시킵니다."), JumpHeight);
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
	return Data;
}
