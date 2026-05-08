// Fill out your copyright notice in the Description page of Project Settings.

#include "SkillSystem/GameplayEffectComponent/MoveBaseGEC.h"

#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "Components/CapsuleComponent.h"
#include "GameplayEffect.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "SkillSystem/GameplayEffect/BaseGameplayEffect.h"
#include "AbilitySystemGlobals.h"
#include "GameplayCueManager.h"
#include "SkillSystem/GameplayCueNotify/Particle/SkillNiagaraSpawnConfig.h"
#include "SkillSystem/GameplayCueNotify/Sound/SkillSoundSpawnConfig.h"

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

	// ë£¨íŠ¸ ëª¨ì…˜ ? ë‹ˆë©”ì´???¬ìƒ ì¤‘ì´ë©??´ë™ ë¬´ì‹œ (?œë²„ ?¬ì´??ì²´í¬)
	if (this->bIgnoreIfRootMotion && IsRootMotionActive(Instigator))
	{
		return;
	}

	const FVector Direction = CalculateMoveDirection(GESpec, Instigator);
	const float Duration = CalculateMoveDuration(GESpec, Instigator, Direction);


	// --- ?´í™???¤í–‰ ---
	if (UAbilitySystemComponent* ASC = UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(Instigator))
	{
		// ?œì‘ ?¨ê³¼ (Burst)
		ExecuteMoveCue(ASC, GESpec, StartVfxConfig, StartSfxConfig, PredictionKey);
		
		// ì§€???¨ê³¼ (Added)
		if (ShouldUseLoopEffects())
		{
			AddMoveCue(ASC, GESpec, LoopVfxConfig, LoopSfxConfig);
		}
	}

	// ?Œìƒ ?´ë˜?¤ê? ?¤ì œ ?´ë™ ë°©ì‹ êµ¬í˜„ (?œë²„ ?¤í–‰ ???„ë‹¬ë°›ì? ?ˆì¸¡ ???¬ìš©)
	Execute(Instigator, Direction, GESpec, PredictionKey);

	// ? ë‹ˆë©”ì´???ë„ ?™ê¸°??
	if (ACharacter* Character = Cast<ACharacter>(Instigator))
	{
		AdjustActiveMontageRate(Character, Duration);
	}
}

void UMoveBaseGEC::OnExecutePredictive(UAbilitySystemComponent* ASC, const FGameplayEffectContextHandle& ContextHandle, const FGameplayEffectSpec& GESpec) const
{
	if (!ASC || !ASC->AbilityActorInfo->IsLocallyControlled())
	{
		return;
	}

	AActor* const Instigator = ASC->GetAvatarActor();
	if (!IsValid(Instigator))
	{
		return;
	}

	// ë£¨íŠ¸ ëª¨ì…˜ ? ë‹ˆë©”ì´???¬ìƒ ì¤‘ì´ë©??´ë™ ë¬´ì‹œ (?´ë¼?´ì–¸???¬ì´???ˆì¸¡ ì²´í¬)
	if (this->bIgnoreIfRootMotion && IsRootMotionActive(Instigator))
	{
		return;
	}

	const FVector Direction = CalculateMoveDirection(GESpec, Instigator);
	const float Duration = CalculateMoveDuration(GESpec, Instigator, Direction);


	// --- ?´í™???¤í–‰ (?ˆì¸¡) ---
	// ?œì‘ ?¨ê³¼ (Burst)
	ExecuteMoveCue(ASC, GESpec, StartVfxConfig, StartSfxConfig, ASC->ScopedPredictionKey);
	
	// ì§€???¨ê³¼ (Added)
	if (ShouldUseLoopEffects())
	{
		AddMoveCue(ASC, GESpec, LoopVfxConfig, LoopSfxConfig);
	}

	// ?Œìƒ ?´ë˜?¤ê? ?¤ì œ ?´ë™ ë°©ì‹ êµ¬í˜„ (?´ë¼?´ì–¸???ˆì¸¡ ?¤í–‰ ??ScopedPredictionKey ?¬ìš©)
	Execute(Instigator, Direction, GESpec, ASC->ScopedPredictionKey);

	// ? ë‹ˆë©”ì´???ë„ ?™ê¸°??
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

	// ì»¨í…?¤íŠ¸ ?„ì¹˜ ?°ì„  ?¬ìš© ?µì…˜??ì¼œì ¸ ?ˆê³ , TowardContext/TowardTarget ë°©ì‹????ì²´í¬
	if (this->bPreferContextLocation &&
		(this->DirectionSource == EMoveDirectionSource::TowardContext || 
		 this->DirectionSource == EMoveDirectionSource::TowardTarget))
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
			// ì»¨í…?¤íŠ¸ ?„ì¹˜ê°€ ?¬ê±°ë¦?MoveDistance) ?´ë‚´?¼ë©´ ?´ë‹¹ ?„ì¹˜ ?¬ìš©
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

	// ?„ì¬ ?¬ìƒ ?„ì¹˜ë¥?ê³ ë ¤?˜ì—¬ ?¨ì? ?œê°„ ê³„ì‚°
	const float CurrentPosition = MontageInstance->GetPosition();
	const float MontageLength = MontageInstance->Montage->GetPlayLength();
	const float RemainingLength = MontageLength - CurrentPosition;

	if (RemainingLength <= 0.0f)
	{
		return;
	}

	// ?¤ì œ ?´ë™ ?œê°„??ë§ì¶° ?¬ìƒ ?ë„ ê³„ì‚° (?¨ì? ê¸¸ì´ / ?´ë™ ?œê°„)
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

void UMoveBaseGEC::ExecuteMoveCue(UAbilitySystemComponent* ASC, const FGameplayEffectSpec& GESpec, const USkillNiagaraSpawnConfig* Vfx, const USkillSoundSpawnConfig* Sfx, FPredictionKey PK) const
{
	if (!IsValid(ASC)) return;

	auto ExecuteOne = [&](const UObject* Config, const FGameplayTag& Tag)
	{
		if (IsValid(Config) && Tag.IsValid())
		{
			FGameplayCueParameters Params(GESpec);
			Params.Location = ASC->GetAvatarActor()->GetActorLocation();
			Params.SourceObject = const_cast<UObject*>(Config);

			// [Fix] ?ˆì¸¡ ?¤ê? ? íš¨?˜ì? ?Šì? ê²½ìš°(?œë²„?ì„œ ?Œì‹¤??ê²½ìš°) ì¤‘ë³µ ?¤í–‰??ë§‰ê¸° ?„í•œ ê°€??
			if (PK.IsValidKey() || ASC->GetOwnerActor()->HasAuthority())
			{
				ASC->ExecuteGameplayCue(Tag, Params);
			}
			else
			{
				UE_LOG(LogTemp, Display, TEXT(">>> MoveBaseGEC: Suppressed Duplicate Cue (Client side, PK is 0) - Tag: [%s]"), *Tag.ToString());
			}
		}
	};

	if (Vfx) ExecuteOne(Vfx, Vfx->CueTag);
	if (Sfx) ExecuteOne(Sfx, Sfx->CueTag);
}

void UMoveBaseGEC::AddMoveCue(UAbilitySystemComponent* ASC, const FGameplayEffectSpec& GESpec, const USkillNiagaraSpawnConfig* Vfx, const USkillSoundSpawnConfig* Sfx) const
{
	if (!IsValid(ASC)) return;

	auto AddOne = [&](const UObject* Config, const FGameplayTag& Tag)
	{
		if (IsValid(Config) && Tag.IsValid())
		{
			FGameplayCueParameters Params(GESpec);
			Params.Location = ASC->GetAvatarActor()->GetActorLocation();
			Params.SourceObject = const_cast<UObject*>(Config);
			ASC->AddGameplayCue(Tag, Params);
		}
	};

	if (Vfx) AddOne(Vfx, Vfx->CueTag);
	if (Sfx) AddOne(Sfx, Sfx->CueTag);
}

void UMoveBaseGEC::RemoveMoveCue(UAbilitySystemComponent* ASC, const USkillNiagaraSpawnConfig* Vfx, const USkillSoundSpawnConfig* Sfx) const
{
	if (!IsValid(ASC)) return;

	if (Vfx && Vfx->CueTag.IsValid()) ASC->RemoveGameplayCue(Vfx->CueTag);
	if (Sfx && Sfx->CueTag.IsValid()) ASC->RemoveGameplayCue(Sfx->CueTag);
}

