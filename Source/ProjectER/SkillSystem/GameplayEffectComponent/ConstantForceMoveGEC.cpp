// Fill out your copyright notice in the Description page of Project Settings.

#include "SkillSystem/GameplayEffectComponent/ConstantForceMoveGEC.h"

#include "AbilitySystemBlueprintLibrary.h"
#include "Components/CapsuleComponent.h"
#include "GameplayEffect.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/RootMotionSource.h"
#include "SkillSystem/GameplayCueNotify/Particle/SkillNiagaraSpawnConfig.h"
#include "TimerManager.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystemGlobals.h"

UConstantForceMoveGEC::UConstantForceMoveGEC()
{
}

float UConstantForceMoveGEC::CalculateMoveDuration(const FGameplayEffectSpec& GESpec, const AActor* Instigator, const FVector& Direction) const
{
	if (this->MoveSpeed > 0.0f)
	{
		const FVector TargetLoc = CalculateTargetLocation(GESpec, Instigator);
		const float Distance = FVector::Dist(Instigator->GetActorLocation(), TargetLoc);
		return Distance / this->MoveSpeed;
	}
	return 0.2f;
}

void UConstantForceMoveGEC::Execute(AActor* Instigator, const FVector& Direction, const FGameplayEffectSpec& GESpec, FPredictionKey PredictionKey) const
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
	const float Duration = (this->MoveSpeed > 0.0f)
		? (Distance / this->MoveSpeed)
		: 0.2f;

	TSharedPtr<FRootMotionSource_ConstantForce> ConstantForce = MakeShared<FRootMotionSource_ConstantForce>();
	
	// 예측 키를 사용하여 인스턴스 이름 동기화 (네트워크 버벅임 해결 핵심)
	FString KeyStr = PredictionKey.IsValidKey() ? FString::FromInt(PredictionKey.Current) : TEXT("NoKey");
	ConstantForce->InstanceName = FName(*FString::Printf(TEXT("ConstantForceMoveGEC_%s"), *KeyStr));
	
	ConstantForce->AccumulateMode = ERootMotionAccumulateMode::Override;
	ConstantForce->Priority = 5;
	ConstantForce->Force = Direction * this->MoveSpeed;
	ConstantForce->Duration = Duration;
	
	// 부모 클래스의 공통 종료 설정 적용
	ConstantForce->FinishVelocityParams.Mode = this->FinishVelocityMode;
	ConstantForce->FinishVelocityParams.SetVelocity = this->FinishSetVelocity;
	ConstantForce->FinishVelocityParams.ClampVelocity = this->FinishClampVelocity;

	CMC->ApplyRootMotionSource(ConstantForce);

	if (this->bIgnoreUnitCollision)
	{
		SetPawnCollisionIgnore(Character, true);
	}

	const FVector StartLoc = Instigator->GetActorLocation();
	const FVector ExpectedEndLoc = TargetLoc;

	TWeakObjectPtr<UConstantForceMoveGEC const> WeakThis = this;
	TWeakObjectPtr<AActor> WeakInstigator = Instigator;

	FTimerHandle PostMoveTimer;
	Instigator->GetWorld()->GetTimerManager().SetTimer(
    PostMoveTimer,
    [WeakThis, WeakInstigator, StartLoc, ExpectedEndLoc, GESpecCopy = GESpec, PredictionKey]()
    {
        // 1. 유효성 검사 (가장 먼저 수행)
        if (!WeakThis.IsValid() || !WeakInstigator.IsValid())
        {
            return;
        }

        AActor* InstigatorPtr = WeakInstigator.Get();
        const FVector ActualEndLoc = InstigatorPtr->GetActorLocation();
        
        // --- 이펙트 종료 처리 ---
        if (UAbilitySystemComponent* ASC = UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(InstigatorPtr))
        {
            // 지속 효과 제거 및 도착 효과 실행
            WeakThis->RemoveMoveCue(ASC, WeakThis->LoopVfxConfig, WeakThis->LoopSfxConfig);
            WeakThis->ExecuteMoveCue(ASC, GESpecCopy, WeakThis->EndVfxConfig, WeakThis->EndSfxConfig, PredictionKey);
        }

        // 2. 벽 충돌 감지 로직
        if (WeakThis->bDetectWallHit)
        {
            const float ExpectedDist = FVector::Dist(StartLoc, ExpectedEndLoc);
            const float ActualDist = FVector::Dist(StartLoc, ActualEndLoc);

            // 예상 거리보다 현저히 적게 이동했다면 벽에 부딪힌 것으로 간주
            if (ExpectedDist > 0.0f && ActualDist < ExpectedDist * 0.85f)
            {
                FHitResult FakeHit;
                FakeHit.Location = ActualEndLoc;
                FakeHit.ImpactPoint = ActualEndLoc; // ImpactPoint도 채워주는 것이 안전합니다.
                
                WeakThis->HandleWallHit(InstigatorPtr, FakeHit, GESpecCopy);
            }
        }

        // 3. 충돌 무시 복구 로직
        if (WeakThis->bIgnoreUnitCollision)
        {
            if (ACharacter* CharPtr = Cast<ACharacter>(InstigatorPtr))
            {
                WeakThis->SetPawnCollisionIgnore(CharPtr, false);
            }
        }

        // 시전자 효과(EndVfx, MovingVfx 종료 등)는 몽타주의 AnimNotify에서 처리됩니다.
    },
    Duration + 0.05f,
    false);
}
