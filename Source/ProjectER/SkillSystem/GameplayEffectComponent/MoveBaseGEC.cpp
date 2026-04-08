// Fill out your copyright notice in the Description page of Project Settings.

#include "SkillSystem/GameplayEffectComponent/MoveBaseGEC.h"

#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "Components/CapsuleComponent.h"
#include "GameplayEffect.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "SkillSystem/GameAbility/SkillBase.h"
#include "SkillSystem/GameplayEffect/BaseGameplayEffect.h"
#include "SkillSystem/SkillNiagaraSpawnConfig.h"
#include "SkillSystem/SkillSoundSpawnConfig.h"

UMoveBaseGEC::UMoveBaseGEC()
{
}

void UMoveBaseGEC::OnGameplayEffectApplied(FActiveGameplayEffectsContainer& ActiveGEContainer, FGameplayEffectSpec& GESpec, FPredictionKey& PredictionKey) const
{
	Super::OnGameplayEffectApplied(ActiveGEContainer, GESpec, PredictionKey);

	const FGameplayEffectContextHandle& ContextHandle = GESpec.GetEffectContext();
	if (ContextHandle.Get() == nullptr)
	{
		return;
	}

	AActor* const Instigator = IsValid(ContextHandle.GetInstigator())
		? ContextHandle.GetInstigator()
		: ContextHandle.GetEffectCauser();
	if (!IsValid(Instigator))
	{
		return;
	}

	// 루트 모션 애니메이션 재생 중이면 이동 무시
	if (this->bIgnoreIfRootMotion && IsRootMotionActive(Instigator))
	{
		return;
	}

	const FVector StartLoc = Instigator->GetActorLocation();
	const FVector Direction = CalculateMoveDirection(GESpec, Instigator);

	const float Duration = CalculateMoveDuration(GESpec, Instigator, Direction);

	// 시전자 효과(Start/Moving/End)는 이제 몽타주의 AnimNotify에서 처리됩니다.
	// 로컬 예측은 몽타주 재생 시스템이 자동으로 수행합니다.

	// 파생 클래스가 실제 이동 방식 구현
	Execute(Instigator, Direction, GESpec);

	// 애니메이션 속도 동기화
	if (ACharacter* Character = Cast<ACharacter>(Instigator))
	{
		AdjustActiveMontageRate(Character, Duration);
	}
}

bool UMoveBaseGEC::IsRootMotionActive(const AActor* Actor) const
{
	const ACharacter* const Character = Cast<ACharacter>(Actor);
	if (!IsValid(Character))
	{
		return false;
	}

	const UCharacterMovementComponent* const CMC = Character->GetCharacterMovement();
	if (!IsValid(CMC))
	{
		return false;
	}

	return CMC->HasAnimRootMotion() || CMC->CurrentRootMotion.HasActiveRootMotionSources();
}

FVector UMoveBaseGEC::CalculateMoveDirection(const FGameplayEffectSpec& GESpec, const AActor* Instigator) const
{
	if (!IsValid(Instigator))
	{
		return FVector::ForwardVector;
	}

	switch (this->DirectionSource)
	{
	case EMoveDirectionSource::Forward:
		return Instigator->GetActorForwardVector();

	case EMoveDirectionSource::TowardContext:
	{
		const FGameplayEffectContextHandle& Context = GESpec.GetEffectContext();
		if (Context.HasOrigin())
		{
			const FVector ToTarget = Context.GetOrigin() - Instigator->GetActorLocation();
			if (!ToTarget.IsNearlyZero())
			{
				return ToTarget.GetSafeNormal();
			}
		}
		return Instigator->GetActorForwardVector();
	}

	case EMoveDirectionSource::TowardTarget:
	{
		const FGameplayEffectContextHandle& Context = GESpec.GetEffectContext();
		if (const FHitResult* const Hit = Context.GetHitResult())
		{
			FVector TargetLocation = FVector::ZeroVector;
			if (!Hit->Location.IsZero())
			{
				TargetLocation = Hit->Location;
			}
			else if (Hit->GetActor())
			{
				TargetLocation = Hit->GetActor()->GetActorLocation();
			}

			const FVector ToTarget = TargetLocation - Instigator->GetActorLocation();
			if (!ToTarget.IsNearlyZero())
			{
				return ToTarget.GetSafeNormal();
			}
		}
		return Instigator->GetActorForwardVector();
	}
	}

	return Instigator->GetActorForwardVector();
}

FVector UMoveBaseGEC::CalculateTargetLocation(const FGameplayEffectSpec& GESpec, const AActor* Instigator) const
{
	if (!IsValid(Instigator))
	{
		return IsValid(Instigator) ? Instigator->GetActorLocation() : FVector::ZeroVector;
	}

	const FVector StartLoc = Instigator->GetActorLocation();
	const FVector Direction = CalculateMoveDirection(GESpec, Instigator);
	const FVector DefaultTarget = StartLoc + Direction * this->MoveDistance;

	// 컨텍스트 위치 우선 사용 옵션이 켜져 있고, TowardContext/TowardTarget 방식일 때 체크
	if (this->bPreferContextLocation &&
		(this->DirectionSource == EMoveDirectionSource::TowardContext || this->DirectionSource == EMoveDirectionSource::TowardTarget))
	{
		const FGameplayEffectContextHandle& Context = GESpec.GetEffectContext();
		FVector ContextLoc = FVector::ZeroVector;
		bool bHasValidContextLoc = false;

		if (this->DirectionSource == EMoveDirectionSource::TowardContext && Context.HasOrigin())
		{
			ContextLoc = Context.GetOrigin();
			bHasValidContextLoc = true;
		}
		else if (this->DirectionSource == EMoveDirectionSource::TowardTarget)
		{
			if (const FHitResult* Hit = Context.GetHitResult())
			{
				if (!Hit->Location.IsZero())
				{
					ContextLoc = Hit->Location;
					bHasValidContextLoc = true;
				}
				else if (Hit->GetActor())
				{
					ContextLoc = Hit->GetActor()->GetActorLocation();
					bHasValidContextLoc = true;
				}
			}
		}

		if (bHasValidContextLoc)
		{
			// 컨텍스트 위치가 사거리(MoveDistance) 이내라면 해당 위치 사용
			const float DistSq = FVector::DistSquared(StartLoc, ContextLoc);
			if (DistSq <= FMath::Square(this->MoveDistance))
			{
				return ContextLoc;
			}
		}
	}

	return DefaultTarget;
}

void UMoveBaseGEC::HandleWallHit(AActor* Instigator, const FHitResult& Hit, const FGameplayEffectSpec& GESpec) const
{
	if (!IsValid(Instigator) || !this->bDetectWallHit)
	{
		return;
	}

	UAbilitySystemComponent* const InstigatorASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(Instigator);
	if (!IsValid(InstigatorASC))
	{
		return;
	}

	const FGameplayEffectContextHandle& ContextHandle = GESpec.GetEffectContext();
	USkillBase* const Skill = const_cast<USkillBase*>(Cast<USkillBase>(ContextHandle.GetAbility()));

	for (const TSubclassOf<UBaseGameplayEffect>& EffectClass : this->WallHitApplied)
	{
		if (!IsValid(EffectClass))
		{
			continue;
		}

		FGameplayEffectSpecHandle SpecHandle = InstigatorASC->MakeOutgoingSpec(EffectClass, GESpec.GetLevel(), ContextHandle);
		if (SpecHandle.IsValid())
		{
			InstigatorASC->ApplyGameplayEffectSpecToTarget(*SpecHandle.Data.Get(), InstigatorASC);
		}
	}
}

void UMoveBaseGEC::SnapToGround(FVector& InOutLocation, const AActor* Instigator) const
{
	if (!IsValid(Instigator))
	{
		return;
	}

	UWorld* const World = Instigator->GetWorld();
	if (!IsValid(World))
	{
		return;
	}

	FHitResult FloorHit;
	const FVector TraceEnd = InOutLocation - FVector(0.0f, 0.0f, this->GroundTraceDistance);

	FCollisionQueryParams QueryParams;
	QueryParams.AddIgnoredActor(Instigator);

	if (World->LineTraceSingleByChannel(FloorHit, InOutLocation, TraceEnd, this->GroundTraceChannel, QueryParams))
	{
		InOutLocation.Z = FloorHit.Location.Z;
	}
}

void UMoveBaseGEC::AdjustActiveMontageRate(ACharacter* Character, float MoveDuration) const
{
	if (!IsValid(Character) || !this->bAdjustMontageRate)
	{
		return;
	}

	if (MoveDuration <= 0.0f)
	{
		return;
	}

	UAnimInstance* const AnimInstance = Character->GetMesh() ? Character->GetMesh()->GetAnimInstance() : nullptr;
	if (!IsValid(AnimInstance))
	{
		return;
	}

	FAnimMontageInstance* const MontageInstance = AnimInstance->GetActiveMontageInstance();
	if (MontageInstance == nullptr || !IsValid(MontageInstance->Montage))
	{
		return;
	}

	// 현재 재생 위치를 고려하여 남은 시간 계산
	const float CurrentPosition = MontageInstance->GetPosition();
	const float MontageLength = MontageInstance->Montage->GetPlayLength();
	const float RemainingLength = MontageLength - CurrentPosition;

	if (RemainingLength <= 0.0f)
	{
		return;
	}

	// 실제 이동 시간에 맞춰 재생 속도 계산 (남은 길이 / 이동 시간)
	const float NewRate = FMath::Clamp(RemainingLength / MoveDuration, this->MinPlayRate, this->MaxPlayRate);
	MontageInstance->SetPlayRate(NewRate);
}

void UMoveBaseGEC::SetPawnCollisionIgnore(ACharacter* Character, bool bIgnore) const
{
	if (!IsValid(Character))
	{
		return;
	}

	UCapsuleComponent* const Capsule = Character->GetCapsuleComponent();
	if (!IsValid(Capsule))
	{
		return;
	}

	Capsule->SetCollisionResponseToChannel(ECC_Pawn, bIgnore ? ECR_Ignore : ECR_Block);
}
