// Fill out your copyright notice in the Description page of Project Settings.

#include "SkillSystem/GameplayEffectComponent/MoveToDynamicForceGEC.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/RootMotionSource.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystemGlobals.h"
#include "TimerManager.h"

UMoveToDynamicForceGEC::UMoveToDynamicForceGEC()
{
}

float UMoveToDynamicForceGEC::CalculateMoveDuration(const FGameplayEffectSpec& GESpec, const AActor* Instigator, const FVector& Direction) const
{
	if (this->bUseMontageDuration)
	{
		const ACharacter* Character = Cast<ACharacter>(Instigator);
		if (IsValid(Character) && Character->GetMesh())
		{
			if (UAnimInstance* AnimInst = Character->GetMesh()->GetAnimInstance())
			{
				if (UAnimMontage* CurrentMontage = AnimInst->GetCurrentActiveMontage())
				{
					const float PlayLength = CurrentMontage->GetPlayLength();
					const float CurrentPos = AnimInst->Montage_GetPosition(CurrentMontage);
					const float PlayRate = AnimInst->Montage_GetPlayRate(CurrentMontage);

					// 남은 시간 계산 ( (전체길이 - 현재위치) / 재생속도 )
					const float RemainingTime = (PlayLength - CurrentPos) / (FMath::Abs(PlayRate) > 0.001f ? FMath::Abs(PlayRate) : 1.0f);

					if (RemainingTime > 0.01f)
					{
						return RemainingTime;
					}
				}
			}
		}
	}

	return this->Duration;
}

void UMoveToDynamicForceGEC::Execute(AActor* Instigator, const FVector& Direction, const FGameplayEffectSpec& GESpec, FPredictionKey PredictionKey) const
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

	const FVector InitialTargetLoc = CalculateTargetLocation(GESpec, Instigator);
	const FVector StartLoc = Instigator->GetActorLocation();
	const float FinalDuration = CalculateMoveDuration(GESpec, Instigator, Direction);

	TSharedPtr<FRootMotionSource_MoveToDynamicForce> MoveToSource = MakeShared<FRootMotionSource_MoveToDynamicForce>();
	
	// 예측 키 동기화
	FString KeyStr = PredictionKey.IsValidKey() ? FString::FromInt(PredictionKey.Current) : TEXT("NoKey");
	MoveToSource->InstanceName = FName(*FString::Printf(TEXT("MoveToDynamicForceGEC_%s"), *KeyStr));
	
	MoveToSource->AccumulateMode = ERootMotionAccumulateMode::Override;
	MoveToSource->Priority = 5;
	
	// Dynamic Move Parameters
	MoveToSource->StartLocation = StartLoc;
	MoveToSource->TargetLocation = InitialTargetLoc;
	MoveToSource->Duration = FinalDuration;
	MoveToSource->bRestrictSpeedToExpected = this->bRestrictSpeedToExpected;
	MoveToSource->PathOffsetCurve = this->PathOffsetCurve;
	
	// 공통 종료 설정
	MoveToSource->FinishVelocityParams.Mode = this->FinishVelocityMode;
	MoveToSource->FinishVelocityParams.SetVelocity = this->FinishSetVelocity;
	MoveToSource->FinishVelocityParams.ClampVelocity = this->FinishClampVelocity;

	const uint16 RootMotionSourceID = CMC->ApplyRootMotionSource(MoveToSource);

	// --- 실시간 타겟 추적 로직 (TowardTarget + bTrackTargetActor 조합) ---
	if (this->DirectionSource == EMoveDirectionSource::TowardTarget && this->bTrackTargetActor)
	{
		TWeakObjectPtr<AActor> WeakTarget;
		const FGameplayEffectContextHandle& Context = GESpec.GetEffectContext();
		if (const FHitResult* Hit = Context.GetHitResult())
		{
			WeakTarget = Hit->GetActor();
		}

		if (WeakTarget.IsValid())
		{
			TWeakObjectPtr<ACharacter> WeakCharacter = Character;
			TWeakObjectPtr<AActor> WeakTargetActor = WeakTarget.Get();
			TWeakObjectPtr<UMoveToDynamicForceGEC const> WeakThis = this;
			const float ArrivalDist = this->ReachedDestinationDistance;

			// 매 프레임 타겟 위치 갱신을 위한 타이머 시작
			FTimerHandle TrackingTimer;
			Character->GetWorld()->GetTimerManager().SetTimer(
				TrackingTimer,
				[WeakCharacter, WeakTargetActor, WeakThis, RootMotionSourceID, ArrivalDist, GESpec, PredictionKey]()
				{
					if (!WeakCharacter.IsValid() || !WeakTargetActor.IsValid() || !WeakThis.IsValid())
					{
						return;
					}

					UCharacterMovementComponent* CurrentCMC = WeakCharacter->GetCharacterMovement();
					if (CurrentCMC)
					{
						const FVector MyLoc = WeakCharacter->GetActorLocation();
						const FVector TargetLoc = WeakTargetActor->GetActorLocation();
						const float DistanceToTarget = FVector::Dist2D(MyLoc, TargetLoc);

						// 1. 목적지 도달 판정
						if (DistanceToTarget <= ArrivalDist)
						{
							CurrentCMC->CurrentRootMotion.RemoveRootMotionSourceByID(RootMotionSourceID);
							
							// 목적지 도달 시 도착 효과 실행 및 지속 효과 제거
							if (UAbilitySystemComponent* ASC = UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(WeakCharacter.Get()))
							{
								WeakThis->RemoveMoveCue(ASC, WeakThis->LoopVfxConfig, WeakThis->LoopSfxConfig);
								WeakThis->ExecuteMoveCue(ASC, GESpec, WeakThis->EndVfxConfig, WeakThis->EndSfxConfig, PredictionKey);
							}
							return; 
						}

						// 2. 활성화된 루트 모션 소스 찾기
						TSharedPtr<FRootMotionSource> ActiveSource = CurrentCMC->CurrentRootMotion.GetRootMotionSourceByID(RootMotionSourceID);
						if (ActiveSource.IsValid())
						{
							FRootMotionSource_MoveToDynamicForce* DynamicSource = static_cast<FRootMotionSource_MoveToDynamicForce*>(ActiveSource.Get());
							if (DynamicSource)
							{
								DynamicSource->SetTargetLocation(TargetLoc);
							}
						}
					}
				},
				0.016f, // 대략 60fps 간격
				true
			);

			// 이동 종료 후 타이머 해제
			FTimerHandle StopTrackingTimer;
			Character->GetWorld()->GetTimerManager().SetTimer(
				StopTrackingTimer,
				[WeakCharacter, WeakThis, GESpec, PredictionKey, TrackingTimer]() mutable
				{
					if (WeakCharacter.IsValid())
					{
						WeakCharacter->GetWorld()->GetTimerManager().ClearTimer(TrackingTimer);

						// 시간 종료 시 도착 효과 실행 및 지속 효과 제거
						if (UAbilitySystemComponent* ASC = UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(WeakCharacter.Get()))
						{
							if (WeakThis.IsValid())
							{
								WeakThis->RemoveMoveCue(ASC, WeakThis->LoopVfxConfig, WeakThis->LoopSfxConfig);
								WeakThis->ExecuteMoveCue(ASC, GESpec, WeakThis->EndVfxConfig, WeakThis->EndSfxConfig, PredictionKey);
							}
						}
					}
				},
				FinalDuration,
				false
			);
		}
	}

	// 유닛 충돌 무시 처리
	if (this->bIgnoreUnitCollision)
	{
		SetPawnCollisionIgnore(Character, true);
		
		TWeakObjectPtr<UMoveToDynamicForceGEC const> WeakThis = this;
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
			FinalDuration + 0.1f,
			false);
	}
}

FSkillTooltipData UMoveToDynamicForceGEC::GetTooltipDescription(int32 Level, TSubclassOf<class USkillBase> AbilityClass) const
{
	FSkillTooltipData Data;
	Data.ShortDescription = FText::FromString(TEXT("대상을 추격하여 돌진합니다."));

	FString DetailStr = TEXT("돌진 : 대상을 향해 빠르게 돌진합니다.");
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
