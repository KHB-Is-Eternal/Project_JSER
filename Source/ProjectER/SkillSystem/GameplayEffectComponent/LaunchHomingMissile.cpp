// Fill out your copyright notice in the Description page of Project Settings.


#include "SkillSystem/GameplayEffectComponent/LaunchHomingMissile.h"
#include "SkillSystem/Actor/BaseMissileActor/BaseMissileActor.h"
#include "SkillSystem/GameplayEffect/BaseGameplayEffect.h"
#include "SkillSystem/GameplayCueNotify/Particle/SkillNiagaraSpawnConfig.h"
#include "SkillSystem/GameplayCueNotify/Sound/SkillSoundSpawnConfig.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "GameplayEffect.h"
#include "GameplayEffectTypes.h"
#include "GameFramework/Character.h"
#include "Components/SkeletalMeshComponent.h"
#include "SkillSystem/GAS/ProjectERGameplayEffectContext.h"
#include "GameFramework/GameStateBase.h"
#include "GameFramework/GameState.h"

// ============================================================================
// GEC
// ============================================================================

ULaunchHomingMissile::ULaunchHomingMissile()
{
}

void ULaunchHomingMissile::PreApplyEffect(UAbilitySystemComponent* ASC, const FGameplayEffectContextHandle& ContextHandle, const FGameplayEffectSpecHandle& SpecHandle) const
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
		
		// 방향 정보는 HitResult의 Normal 필드를 활용하여 전달
		FHitResult SimulationHit;
		SimulationHit.Location = CurrentPos;
		SimulationHit.Normal = CurrentRot.Vector();
		SimulationHit.HitObjectHandle = FActorInstanceHandle(TargetActor);
		MutableContext->AddHitResult(SimulationHit, true);
	}
}

void ULaunchHomingMissile::OnExecutePredictive(UAbilitySystemComponent* ASC, const FGameplayEffectContextHandle& ContextHandle, const FGameplayEffectSpecHandle& SpecHandle) const
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
		FGameplayCueParameters Params(*SpecHandle.Data.Get());
		Params.Location = CueLocation;
		Params.Normal = CueDirection;
		// [삭제] 시각 정보(ClientActivationTime)가 이미 컨텍스트에 담겨 있으므로 별도의 예측 키 주입이 불필요합니다.
		
		ASC->ExecuteGameplayCue(this->MissileVfx->CueTag, Params);
	}

	// 3. 미사일 발사 사운드 트리거
	if (IsValid(this->MissileSound) && this->MissileSound->CueTag.IsValid())
	{
		FGameplayCueParameters Params(*SpecHandle.Data.Get());
		Params.Location = CueLocation;
		Params.Normal = CueDirection;
		// [삭제] 시각 정보(ClientActivationTime)가 이미 컨텍스트에 담겨 있으므로 별도의 예측 키 주입이 불필요합니다.
		
		ASC->ExecuteGameplayCue(this->MissileSound->CueTag, Params);
	}
}

void ULaunchHomingMissile::OnGameplayEffectApplied(
	FActiveGameplayEffectsContainer& ActiveGEContainer,
	FGameplayEffectSpec& GESpec,
	FPredictionKey& PredictionKey) const
{
	Super::OnGameplayEffectApplied(ActiveGEContainer, GESpec, PredictionKey);

	// --- Context 검증 ---
	const FGameplayEffectContextHandle& ContextHandle = GESpec.GetEffectContext();
	const FGameplayEffectContext* EffectContext = ContextHandle.Get();
	if (!EffectContext)
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

	UWorld* const World = Instigator->GetWorld();
	if (!IsValid(World))
	{
		return;
	}

	// --- 클래스 세팅 검증 ---
	if (!IsValid(this->MissileActorClass))
	{
		return;
	}

	// --- 타겟 및 스폰 위치 계산 ---
	// 1순위: HitResult에서 타겟 추출
	AActor* TargetActor = nullptr;
	if (const FHitResult* HitResult = ContextHandle.GetHitResult())
	{
		TargetActor = HitResult->GetActor();
	}
	// 2순위: HitResult가 없거나 Actor가 없으면 GE가 적용된 대상(Owner)을 타겟으로 사용
	if (!IsValid(TargetActor))
	{
		TargetActor = GetTargetActorFromContainer(ActiveGEContainer);
	}

	if (!IsValid(TargetActor))
	{
		return;
	}

	// [V7.2] PreApplyEffect에서 보정된 트랜스폼을 사용합니다.
	FTransform SpawnTransform = CalculateSpawnTransform(Instigator, TargetActor);
	if (ContextHandle.HasOrigin())
	{
		SpawnTransform.SetLocation(ContextHandle.GetOrigin());
		if (const FHitResult* Hit = ContextHandle.GetHitResult())
		{
			SpawnTransform.SetRotation(Hit->Normal.Rotation().Quaternion());
		}
	}

	// 2. 권한 확인: 실제 액터 스폰은 서버에서만 수행합니다.
	if (!ActiveGEContainer.Owner || !ActiveGEContainer.Owner->IsOwnerActorAuthoritative())
	{
		return;
	}

	// --- 미사일 액터 지연 생성 ---
	APawn* const SpawnInstigator = Cast<APawn>(ContextHandle.GetInstigator());
	ABaseMissileActor* const MissileActor = World->SpawnActorDeferred<ABaseMissileActor>(
		this->MissileActorClass,
		SpawnTransform,
		Instigator,
		SpawnInstigator,
		ESpawnActorCollisionHandlingMethod::AlwaysSpawn);
	if (!IsValid(MissileActor))
	{
		return;
	}

	// --- 효과 Spec 생성 --- 
	UAbilitySystemComponent* const CauserASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(Instigator);
	UGameplayAbility* const Ability = const_cast<UGameplayAbility*>(ContextHandle.GetAbility());

	TArray<FGameplayEffectSpecHandle> EffectSpecs;
	if (IsValid(CauserASC) && IsValid(Ability))
	{
		for (const TSubclassOf<UBaseGameplayEffect>& EffectClass : this->Applied)
		{
			if (IsValid(EffectClass))
			{
				FGameplayEffectSpecHandle SpecHandle = CauserASC->MakeOutgoingSpec(EffectClass, GESpec.GetLevel(), ContextHandle);
				if (SpecHandle.IsValid())
				{
					EffectSpecs.Add(SpecHandle);
				}
			}
		}

		// 강화 효과(SkillProc) 확인 및 적용
		UBaseGEC::GetSkillProcEffects(CauserASC, Ability, MissileActor, ContextHandle, EffectSpecs);
	}
	
	// --- 명중 효과(VFX, Sound)용 파라미터 구성 ---
	// 명중 효과는 로컬 예측이 필수적이지 않으므로 기존 방식(InitializeMissile 통해 전달)을 유지합니다. 
	FGameplayCueParameters HitVfxCueParams(GESpec);
	if (IsValid(this->ImpactVfx) && this->ImpactVfx->CueTag.IsValid())
	{
		HitVfxCueParams.OriginalTag = this->ImpactVfx->CueTag;
		HitVfxCueParams.Instigator = ContextHandle.GetInstigator();
		HitVfxCueParams.EffectCauser = MissileActor;
		HitVfxCueParams.Location = SpawnTransform.GetLocation();
		HitVfxCueParams.SourceObject = this->ImpactVfx;
		HitVfxCueParams.GameplayEffectLevel = GESpec.GetLevel();
	}

	FGameplayCueParameters HitSoundCueParams(GESpec);
	if (IsValid(this->ImpactSound) && this->ImpactSound->CueTag.IsValid())
	{
		HitSoundCueParams.OriginalTag = this->ImpactSound->CueTag;
		HitSoundCueParams.Instigator = ContextHandle.GetInstigator();
		HitSoundCueParams.EffectCauser = MissileActor;
		HitSoundCueParams.Location = SpawnTransform.GetLocation();
		HitSoundCueParams.SourceObject = this->ImpactSound;
		HitSoundCueParams.GameplayEffectLevel = GESpec.GetLevel();
	}

	// --- 미사일 초기화 ---
	// SpawnTransform의 방향을 직접 추출 (FinishSpawning 이전이므로 GetActorForwardVector() 불확실)
	const FVector InitialDirection = SpawnTransform.GetRotation().GetForwardVector();

	MissileActor->InitializeMissile(
		EffectSpecs,
		Instigator,
		TargetActor,
		HitVfxCueParams,
		HitSoundCueParams,
		this->InitialSpeed,
		this->MaxSpeed,
		this->HomingAccelerationMagnitude,
		this->ReachThreshold,
		this->bDestroyOnHit,
		InitialDirection
	);
	MissileActor->SetLifeSpan(this->LifeSpan);

	// 컨텍스트로부터 시전 시간 추출 및 판정 액터에 주입 (VFX 핸드셰이크용)
	if (const FProjectERGameplayEffectContext* ErContext = static_cast<const FProjectERGameplayEffectContext*>(ContextHandle.Get()))
	{
		MissileActor->SetClientActivationTime(ErContext->ClientActivationTime);
	}

	// --- 스폰 완료 ---
	MissileActor->FinishSpawning(SpawnTransform);



	// --- 시각 효과 실행 ---
	// [수정] 네이티브 GameplayCue 시스템이 기존 로직을 대체합니다.
	// 시전자에 관여된 효과는 몽타주 AnimNotify 등에서 담당합니다.
	ExecuteVfx(GESpec, ContextHandle, Instigator, MissileActor);
	ExecuteSound(GESpec, ContextHandle, Instigator, MissileActor);
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

void ULaunchHomingMissile::ExecuteVfx(
	const FGameplayEffectSpec& GESpec,
	const FGameplayEffectContextHandle& ContextHandle,
	AActor* Instigator,
	ABaseMissileActor* MissileActor) const
{
	// [수정] 수동으로 GameplayCue를 실행하던 로직을 제거하고, Phase 2(OnExecutePredictive)에서 예측 발사 효과를 처리합니다. 
}

void ULaunchHomingMissile::ExecuteSound(
	const FGameplayEffectSpec& GESpec,
	const FGameplayEffectContextHandle& ContextHandle,
	AActor* Instigator,
	ABaseMissileActor* MissileActor) const
{
	// [수정] 수동으로 GameplayCue를 실행하던 로직을 제거하고, Phase 2(OnExecutePredictive)에서 예측 발송 사운드를 처리합니다. 
}
