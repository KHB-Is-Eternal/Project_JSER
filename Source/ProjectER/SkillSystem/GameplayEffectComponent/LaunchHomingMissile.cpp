// Fill out your copyright notice in the Description page of Project Settings.


#include "SkillSystem/GameplayEffectComponent/LaunchHomingMissile.h"
#include "SkillSystem/Actor/BaseMissileActor/BaseMissileActor.h"
#include "SkillSystem/GameplayEffect/BaseGameplayEffect.h"
#include "SkillSystem/GameplayCueNotify/Particle/SkillNiagaraSpawnConfig.h"
#include "SkillSystem/GameplayCueNotify/Sound/SkillSoundSpawnConfig.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemGlobals.h"
#include "GameplayCueManager.h"
#include "GameplayEffect.h"
#include "GameplayEffectTypes.h"
#include "GameFramework/Character.h"
#include "Components/SkeletalMeshComponent.h"
#include "SkillSystem/GameAbility/SkillBase.h"
#include "SkillSystem/GAS/ProjectERGameplayEffectContext.h"
#include "GameFramework/GameStateBase.h"
#include "GameFramework/GameState.h"
#include "GameFramework/ProjectileMovementComponent.h"

// ============================================================================
// GEC
// ============================================================================

ULaunchHomingMissile::ULaunchHomingMissile()
{
}

void ULaunchHomingMissile::SetupMovement(UProjectileMovementComponent* Movement) const
{
	if (!Movement) return;
	Movement->InitialSpeed = InitialSpeed;
	Movement->MaxSpeed = MaxSpeed;
	Movement->ProjectileGravityScale = 0.0f;
}

void ULaunchHomingMissile::PreApplyEffect(UAbilitySystemComponent* ASC, const FGameplayEffectContextHandle& ContextHandle, const FGameplayEffectSpec& GESpec) const
{
	if (!IsValid(ASC)) return;

	AActor* const Instigator = ASC->GetAvatarActor();
	if (!IsValid(Instigator)) return;

	// 1. 타겟 정보
	AActor* TargetActor = nullptr;
	if (const FHitResult* HitResult = ContextHandle.GetHitResult()) TargetActor = HitResult->GetActor();
	if (!IsValid(TargetActor)) return;

	// 2. 초기 스폰 트랜스폼 계산
	FTransform BaseTransform = CalculateSpawnTransform(Instigator, TargetActor);
	FVector CurrentPos = BaseTransform.GetLocation();
	FRotator CurrentRot = BaseTransform.Rotator();

	// 3. 지연 시간(Lag) 계산
	float Lag = 0.0f;
	if (const FProjectERGameplayEffectContext* ERContext = static_cast<const FProjectERGameplayEffectContext*>(ContextHandle.Get()))
	{
		if (UWorld* World = ASC->GetWorld())
		{
			if (AGameStateBase* GameState = World->GetGameState())
			{
				Lag = GameState->GetServerWorldTimeSeconds() - ERContext->ClientActivationTime;
			}
		}
	}

	// 4. [렉 보상 엔진] 곡선 궤적 시뮬레이션(Fast-Forward)
	// 서버에서 미사일이 스폰되는 시점의 좌표를 클라이언트의 현재 위치와 일치시킵니다.
	if (Lag > 0.01f && IsValid(TargetActor))
	{
		FVector Velocity = CurrentRot.Vector() * InitialSpeed;
		float RemainingLag = FMath::Min(Lag, 0.5f); // 최대 0.5초까지만 보정 (안전성)
		const float SubStep = 0.016f; // 60fps 기준으로 시뮬레이션

		while (RemainingLag > 0.0f)
		{
			float dt = FMath::Min(RemainingLag, SubStep);
			
			// 조종 가속도 계산 (Target 방향으로 휘어지는 힘)
			FVector TargetLoc = TargetActor->GetActorLocation();
			FVector DirToTarget = (TargetLoc - CurrentPos).GetSafeNormal();
			FVector HomingAccel = DirToTarget * HomingAccelerationMagnitude;

			// 물리 시뮬레이션
			Velocity += HomingAccel * dt;
			Velocity = Velocity.GetClampedToMaxSize(MaxSpeed);
			CurrentPos += Velocity * dt;
			
			RemainingLag -= dt;
		}
		CurrentRot = Velocity.Rotation();
	}

	// 5. 보정된 결과(Origin, Normal)를 Context에 기록하여 공유
	FProjectERGameplayEffectContext* const MutableContext = static_cast<FProjectERGameplayEffectContext*>(const_cast<FGameplayEffectContext*>(ContextHandle.Get()));
	if (MutableContext)
	{
		MutableContext->AddOrigin(CurrentPos);
		APawn* const AvatarPawn = (ASC ? Cast<APawn>(ASC->GetAvatarActor()) : nullptr);
		UE_LOG(LogTemp, Warning, TEXT("[GEC] PreApplyEffect - Setting Origin: %s (IsLocallyControlled: %d)"), *CurrentPos.ToString(), AvatarPawn ? AvatarPawn->IsLocallyControlled() : -1);
		
		// 방향 정보는 HitResult의 Normal 필드를 활용하여 전달
		FHitResult SimulationHit;
		SimulationHit.Location = CurrentPos;
		SimulationHit.Normal = CurrentRot.Vector();
		SimulationHit.HitObjectHandle = FActorInstanceHandle(TargetActor);
		MutableContext->AddHitResult(SimulationHit, true);
	}
}

void ULaunchHomingMissile::OnExecutePredictive(UAbilitySystemComponent* ASC, const FGameplayEffectContextHandle& ContextHandle, const FGameplayEffectSpec& GESpec) const
{
	// [V7.3] 시전자 클라이언트의 예측 VFX 로직을 OnExecuteVFXCue로 통합하여 호출합니다.
	// SkillBase에서 IsLocallyControlled()일 때만 OnExecutePredictive를 호출하므로 안전합니다.
	
	// [Fix] 현재 스코프의 예측 키를 전달하여 로컬 예측 실행을 보장합니다.
	FPredictionKey PredictionKey;
	if (ASC) PredictionKey = ASC->ScopedPredictionKey;
	
	OnExecuteVFXCue(ASC, ContextHandle, GESpec, PredictionKey);
}

void ULaunchHomingMissile::OnExecuteVFXCue(UAbilitySystemComponent* ASC, const FGameplayEffectContextHandle& ContextHandle, const FGameplayEffectSpec& GESpec, FPredictionKey PredictionKey) const
{
	if (!IsValid(ASC)) return;

	// 1. Context에서 보정된 좌표 및 방향 추출
	FVector CueLocation = ContextHandle.GetOrigin();
	FVector CueDirection = FVector::ForwardVector;
	if (const FHitResult* Hit = ContextHandle.GetHitResult())
	{
		CueDirection = Hit->Normal;
	}

	// 2. 미사일 발사 시각 효과(VFX) 트리거
	if (IsValid(this->MissileVfx) && this->MissileVfx->CueTag.IsValid())
	{
		FGameplayCueParameters Params(GESpec);
		Params.Location = CueLocation;
		Params.Normal = CueDirection;
		Params.SourceObject = const_cast<ULaunchHomingMissile*>(this);
		
		Params.Instigator = ContextHandle.GetInstigator();
		Params.EffectCauser = ASC->GetAvatarActor();
		if (!Params.Instigator.IsValid()) Params.Instigator = Params.EffectCauser;

		{
			if (UGameplayCueManager* CueManager = UAbilitySystemGlobals::Get().GetGameplayCueManager())
			{
				CueManager->InvokeGameplayCueExecuted_WithParams(ASC, this->MissileVfx->CueTag, PredictionKey, Params);
			}
		}
	}
	else if (IsValid(this->MissileSound) && this->MissileSound->CueTag.IsValid())
	{
		// [Conditional] VFX 태그가 없어 비주얼 액터가 생성되지 않는 경우에만 직접 사운드 큐를 실행합니다.
		FGameplayCueParameters Params(GESpec);
		Params.Location = CueLocation;
		Params.Normal = CueDirection;

		Params.Instigator = ContextHandle.GetInstigator();
		Params.EffectCauser = ASC->GetAvatarActor();
		if (!Params.Instigator.IsValid()) Params.Instigator = Params.EffectCauser;

		{
			if (UGameplayCueManager* CueManager = UAbilitySystemGlobals::Get().GetGameplayCueManager())
			{
				CueManager->InvokeGameplayCueExecuted_WithParams(ASC, this->MissileSound->CueTag, PredictionKey, Params);
			}
		}
	}
}

void ULaunchHomingMissile::OnGameplayEffectApplied(
	FActiveGameplayEffectsContainer& ActiveGEContainer,
	FGameplayEffectSpec& GESpec,
	FPredictionKey& PredictionKey) const
{
	Super::OnGameplayEffectApplied(ActiveGEContainer, GESpec, PredictionKey);

	if (!ensure(MissileActorClass)) return;

	const FGameplayEffectContextHandle& ContextHandle = GESpec.GetEffectContext();
	AActor* const EffectInstigator = ContextHandle.GetInstigator() ? ContextHandle.GetInstigator() : ContextHandle.GetEffectCauser();
	if (!IsValid(EffectInstigator)) return;

	// 1. 타겟 식별
	AActor* TargetActor = nullptr;
	if (const FHitResult* HitResult = ContextHandle.GetHitResult()) TargetActor = HitResult->GetActor();
	if (!IsValid(TargetActor)) TargetActor = GetTargetActorFromContainer(ActiveGEContainer);
	if (!IsValid(TargetActor)) return;

	// 2. 소환 트랜스폼 계산
	FTransform SpawnTransform = GetInitialTransform(ContextHandle, ActiveGEContainer, EffectInstigator, TargetActor);

	// 3. 권한 확인 및 실행 (서버 전용)
	if (ActiveGEContainer.Owner && ActiveGEContainer.Owner->IsOwnerActorAuthoritative())
	{
		// 관전자용 VFX 브로드캐스트
		OnExecuteVFXCue(ActiveGEContainer.Owner, ContextHandle, GESpec, PredictionKey);

		if (UWorld* World = EffectInstigator->GetWorld())
		{
			if (ABaseMissileActor* MissileActor = SpawnDeferredActor(World, MissileActorClass, SpawnTransform, EffectInstigator))
			{
				InitializeActorData(MissileActor, ContextHandle, GESpec, SpawnTransform, TargetActor);
				MissileActor->FinishSpawning(SpawnTransform);
			}
		}
	}
}

FTransform ULaunchHomingMissile::GetInitialTransform(const FGameplayEffectContextHandle& ContextHandle, FActiveGameplayEffectsContainer& ActiveGEContainer, AActor* Instigator, AActor* TargetActor) const
{
	// [Fix] 서버는 항상 CalculateSpawnTransform을 통해 직접 계산한 최신 좌표를 신뢰합니다.
	// (클라이언트에서 오염된 Context Origin이 넘어올 수 있기 때문)
	return CalculateSpawnTransform(Instigator, TargetActor);
}

ABaseMissileActor* ULaunchHomingMissile::SpawnDeferredActor(UWorld* World, TSubclassOf<ABaseMissileActor> ActorClass, const FTransform& Transform, AActor* Instigator) const
{
	return World->SpawnActorDeferred<ABaseMissileActor>(
		ActorClass,
		Transform,
		Instigator,
		Cast<APawn>(Instigator),
		ESpawnActorCollisionHandlingMethod::AlwaysSpawn);
}

void ULaunchHomingMissile::InitializeActorData(ABaseMissileActor* Actor, const FGameplayEffectContextHandle& ContextHandle, const FGameplayEffectSpec& GESpec, const FTransform& Transform, AActor* TargetActor) const
{
	UAbilitySystemComponent* const CauserASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(ContextHandle.GetInstigator());
	UGameplayAbility* const Ability = const_cast<UGameplayAbility*>(ContextHandle.GetAbility());
	
	TArray<FGameplayEffectSpecHandle> EffectSpecs;
	if (IsValid(CauserASC) && IsValid(Ability))
	{
		for (const TSubclassOf<UBaseGameplayEffect>& EffectClass : this->Applied)
		{
			if (IsValid(EffectClass))
			{
				FGameplayEffectSpecHandle Spec = CauserASC->MakeOutgoingSpec(EffectClass, GESpec.GetLevel(), ContextHandle);
				UBaseGEC::InheritHitTags(GESpec, Spec);
				EffectSpecs.Add(Spec);
			}
		}
		UBaseGEC::GetSkillProcEffects(CauserASC, Ability, Actor, ContextHandle, EffectSpecs, true, &GESpec);
	}

	// 명중 효과 파라미터 구성
	FGameplayCueParameters HitVfxParams(GESpec);
	if (ImpactVfx) { HitVfxParams.OriginalTag = ImpactVfx->CueTag; HitVfxParams.EffectCauser = Actor; HitVfxParams.SourceObject = ImpactVfx; }
	
	FGameplayCueParameters HitSoundParams(GESpec);
	if (ImpactSound) { HitSoundParams.OriginalTag = ImpactSound->CueTag; HitSoundParams.EffectCauser = Actor; HitSoundParams.SourceObject = ImpactSound; }

	// [Fix] 핸드셰이크를 위해 시전 시간을 먼저 설정해야 합니다. (InitializeMissile 내부에서 핸드셰이크 시도함)
	if (const FProjectERGameplayEffectContext* ErContext = ProjectERContextUtils::GetProjectERContext(ContextHandle))
	{
		Actor->SetClientActivationTime(ErContext->ClientActivationTime);
	}

	// 미사일 초기화
	Actor->InitializeMissile(EffectSpecs, ContextHandle.GetInstigator(), TargetActor, HitVfxParams, HitSoundParams, 
		InitialSpeed, MaxSpeed, HomingAccelerationMagnitude, ReachThreshold, bDestroyOnHit, Transform.GetRotation().GetForwardVector());
	
	Actor->SetLifeSpan(LifeSpan);
}

FTransform ULaunchHomingMissile::CalculateSpawnTransform(
	const AActor* Instigator,
	const AActor* TargetActor) const
{
	if (!IsValid(Instigator))
	{
		return FTransform::Identity;
	}

	FVector SpawnLocation = Instigator->GetActorLocation();
	FRotator SpawnRotation = Instigator->GetActorRotation();

	// 1. 소켓(Bone) 위치 사용
	if (!this->BoneName.IsNone())
	{
		const ACharacter* Character = Cast<ACharacter>(Instigator);
		const USkeletalMeshComponent* Mesh = Character
			? Character->GetMesh()
			: Instigator->FindComponentByClass<USkeletalMeshComponent>();

		if (IsValid(Mesh) && Mesh->DoesSocketExist(this->BoneName))
		{
			SpawnLocation = Mesh->GetSocketLocation(this->BoneName);
		}
	}

	// 2. 타겟을 향한 방향 계산
	if (IsValid(TargetActor))
	{
		// 타겟의 중심이나 피격 위치 등의 실제 유도 지점인 RootComponent의 위치를 정확히 조준합니다.
		const FVector TargetLocation = TargetActor->GetRootComponent()
			? TargetActor->GetRootComponent()->GetComponentLocation()
			: TargetActor->GetActorLocation();

		FVector Dir = TargetLocation - SpawnLocation;
		if (!Dir.IsNearlyZero())
		{
			SpawnRotation = Dir.Rotation();
		}
	}

	return FTransform(SpawnRotation, SpawnLocation);
}

AActor* ULaunchHomingMissile::GetTargetActorFromContainer(FActiveGameplayEffectsContainer& ActiveGEContainer) const
{
	return ActiveGEContainer.Owner ? ActiveGEContainer.Owner->GetOwner() : nullptr;
}

FSkillTooltipData ULaunchHomingMissile::GetTooltipDescription(int32 Level, TSubclassOf<class USkillBase> AbilityClass) const
{
	FSkillTooltipData Data;
	Data.ShortDescription = FText::FromString(TEXT("유도 투사체를 발사합니다."));

	FString DetailStr = TEXT("유도 미사일 : 대상을 추적하는 유도 미사일을 날립니다.");
	FText EffectsText = FormatAppliedEffects(Applied, Level);
	if (!EffectsText.IsEmpty())
	{
		DetailStr += TEXT("\n") + EffectsText.ToString();
	}

	Data.DetailedDescription = FText::FromString(DetailStr);
	return Data;
}


void ULaunchHomingMissile::CollectNiagaraPaths(TArray<FSoftObjectPath>& OutPaths) const
{
	Super::CollectNiagaraPaths(OutPaths);
	if (MissileVfx && !MissileVfx->NiagaraSystem.IsNull())
	{
		OutPaths.AddUnique(MissileVfx->NiagaraSystem.ToSoftObjectPath());
	}
	if (ImpactVfx && !ImpactVfx->NiagaraSystem.IsNull())
	{
		OutPaths.AddUnique(ImpactVfx->NiagaraSystem.ToSoftObjectPath());
	}
	for (const TSubclassOf<UBaseGameplayEffect>& GEClass : Applied)
	{
		if (GEClass)
		{
			if (const UBaseGameplayEffect* GE = GEClass->GetDefaultObject<UBaseGameplayEffect>())
			{
				for (const UGameplayEffectComponent* Component : GE->GetGEComponents())
				{
					if (const UBaseGEC* BaseGEC = Cast<UBaseGEC>(Component))
					{
						BaseGEC->CollectNiagaraPaths(OutPaths);
					}
				}
			}
		}
	}
}
