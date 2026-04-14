#include "SkillSystem/GameplayEffectComponent/SummonRangeBaseGEC.h"

#include "Abilities/GameplayAbilityTypes.h"
#include "GameplayEffectTypes.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemGlobals.h"
#include "GameplayCueManager.h"
#include "GameplayEffect.h"
#include "GameFramework/Actor.h"
#include "GameFramework/Pawn.h"
#include "SkillSystem/Actor/BaseRangeOverlapEffectActor/BaseRangeOverlapEffectActor.h"
#include "SkillSystem/GameplayEffect/BaseGameplayEffect.h"

#include "SkillSystem/GameplayCueNotify/Particle/SkillNiagaraSpawnConfig.h"
#include "SkillSystem/GameplayCueNotify/Sound/SkillSoundSpawnConfig.h"
#include "SkillSystem/GameplayEffectComponent/SummonRangeAtBone.h"
#include "SkillSystem/GAS/ProjectERGameplayEffectContext.h"
#include "GameFramework/GameStateBase.h"
#include "GameFramework/GameState.h"

void USummonRangeBaseGEC::PreApplyEffect(UAbilitySystemComponent* ASC, const FGameplayEffectContextHandle& ContextHandle, const FGameplayEffectSpecHandle& SpecHandle) const
{
	if (!IsValid(ASC)) return;

	AActor* const EffectInstigator = IsValid(ContextHandle.GetInstigator())
		? ContextHandle.GetInstigator()
		: ContextHandle.GetEffectCauser();
	if (!IsValid(EffectInstigator) || !ShouldProcessOnInstigator(EffectInstigator)) return;

	// 1. 타겟 식별 및 기준 위치(Origin) 계산
	AActor* const TargetActor = nullptr; // Target을 뽑아오는건 Container에서 해야 가장 정확하지만, 여기서는 Context의 HitResult 사용
	const FTransform OriginTransform = CalculateOriginTransform(*SpecHandle.Data.Get(), EffectInstigator, TargetActor);
	const FTransform SpawnTransform = ApplyCommonSpawnOptionsToTransform(OriginTransform, EffectInstigator);

	// 2. 보정된 결과(Origin)를 Context에 기록
	FProjectERGameplayEffectContext* const MutableContext = static_cast<FProjectERGameplayEffectContext*>(const_cast<FGameplayEffectContext*>(ContextHandle.Get()));
	if (MutableContext)
	{
		MutableContext->AddOrigin(SpawnTransform.GetLocation());
		
		FHitResult SimulationHit;
		SimulationHit.Location = SpawnTransform.GetLocation();
		SimulationHit.Normal = SpawnTransform.Rotator().Vector();
		MutableContext->AddHitResult(SimulationHit, true);
	}
}

void USummonRangeBaseGEC::OnExecutePredictive(UAbilitySystemComponent* ASC, const FGameplayEffectContextHandle& ContextHandle, const FGameplayEffectSpecHandle& SpecHandle) const
{
	if (!IsValid(ASC)) return;

	FVector CueLocation = ContextHandle.GetOrigin();
	FVector CueDirection = FVector::UpVector;
	if (const FHitResult* Hit = ContextHandle.GetHitResult())
	{
		CueDirection = Hit->Normal;
	}

	if (IsValid(this->RangeSpawnVfx) && this->RangeSpawnVfx->CueTag.IsValid())
	{
		FGameplayCueParameters Params(*SpecHandle.Data.Get());
		Params.Location = CueLocation;
		Params.Normal = CueDirection;
		Params.SourceObject = const_cast<USummonRangeBaseGEC*>(this);


		ASC->ExecuteGameplayCue(this->RangeSpawnVfx->CueTag, Params);
	}

	if (IsValid(this->RangeSpawnSound) && this->RangeSpawnSound->CueTag.IsValid())
	{
		FGameplayCueParameters Params(*SpecHandle.Data.Get());
		Params.Location = CueLocation;
		Params.Normal = CueDirection;
		Params.SourceObject = const_cast<USummonRangeBaseGEC*>(this);


		ASC->ExecuteGameplayCue(this->RangeSpawnSound->CueTag, Params);
	}
}

void USummonRangeBaseGEC::OnGameplayEffectApplied(FActiveGameplayEffectsContainer& ActiveGEContainer, FGameplayEffectSpec& GESpec, FPredictionKey& PredictionKey) const
{
	Super::OnGameplayEffectApplied(ActiveGEContainer, GESpec, PredictionKey);

	const FGameplayEffectContextHandle& ContextHandle = GESpec.GetEffectContext();
	const FGameplayEffectContext* const EffectContext = ContextHandle.Get();
	if (EffectContext == nullptr)
	{
		return;
	}

	AActor* const EffectInstigator = IsValid(ContextHandle.GetInstigator())
		? ContextHandle.GetInstigator()
		: ContextHandle.GetEffectCauser();
	if (!IsValid(EffectInstigator))
	{
		return;
	}

	if (!ShouldProcessOnInstigator(EffectInstigator))
	{
		return;
	}

	UWorld* const World = EffectInstigator->GetWorld();
	if (!IsValid(World))
	{
		return;
	}

	if (!IsValid(this->RangeActorClass))
	{
		return;
	}

	// 1. 타켓 식별 및 위치 계산 (PreApplyEffect에서 보정된 Origin 우선 사용)
	FTransform SpawnTransform = FTransform::Identity;
	if (ContextHandle.HasOrigin())
	{
		SpawnTransform.SetLocation(ContextHandle.GetOrigin());
		if (const FHitResult* Hit = ContextHandle.GetHitResult())
		{
			SpawnTransform.SetRotation(Hit->Normal.Rotation().Quaternion());
		}
	}
	else
	{
		AActor* const TargetActor = GetTargetActorFromContainer(ActiveGEContainer);
		const FTransform OriginTransform = CalculateOriginTransform(GESpec, EffectInstigator, TargetActor);
		SpawnTransform = ApplyCommonSpawnOptionsToTransform(OriginTransform, EffectInstigator);
	}
	const FVector RangeSpawnLocation = SpawnTransform.GetLocation();

	// 2. 권한 확인: 실제 액터 소환은 서버에서만 수행합니다. (중복 소환 방지)
	if (ActiveGEContainer.Owner && ActiveGEContainer.Owner->IsOwnerActorAuthoritative())
	{
		// 3. 액터 소환 (지연 생성)
		APawn* const SpawnInstigator = Cast<APawn>(ContextHandle.GetInstigator());
		ABaseRangeOverlapEffectActor* const DeferredSpawnedActor = World->SpawnActorDeferred<ABaseRangeOverlapEffectActor>(
			this->RangeActorClass,
			SpawnTransform,
			EffectInstigator,
			SpawnInstigator,
			ESpawnActorCollisionHandlingMethod::AlwaysSpawn);
			
		if (IsValid(DeferredSpawnedActor))
		{
			// 4. 초기화 및 마무리
			const FGameplayCueParameters HitTargetVfxCueParameters = BuildNiagaraCueParameters(
				GESpec,
				IsValid(this->HitTargetVfx.Get()) ? this->HitTargetVfx->CueTag : FGameplayTag(),
				ContextHandle,
				DeferredSpawnedActor,
				RangeSpawnLocation,
				this->HitTargetVfx.Get());

			const FGameplayCueParameters HitTargetSoundCueParameters = BuildNiagaraCueParameters(
				GESpec,
				IsValid(this->HitTargetSound.Get()) ? this->HitTargetSound->CueTag : FGameplayTag(),
				ContextHandle,
				DeferredSpawnedActor,
				RangeSpawnLocation,
				this->HitTargetSound.Get());

			// 컨텍스트로부터 시전 시간 추출 및 장판 액터에 주입 (VFX 핸드셰이크용)
			if (const FProjectERGameplayEffectContext* ErContext = static_cast<const FProjectERGameplayEffectContext*>(ContextHandle.Get()))
			{
				DeferredSpawnedActor->SetClientActivationTime(ErContext->ClientActivationTime);
			}

			InitializeRangeActor(DeferredSpawnedActor, EffectInstigator, ContextHandle, HitTargetVfxCueParameters, HitTargetSoundCueParameters);

			DeferredSpawnedActor->FinishSpawning(SpawnTransform);

			// 5. 렉 보상 (Lag Compensation / Fast-Forward)
			// 클라이언트 측 발동 시간과 서버 현재 시간을 비교하여 소환 위치를 오프셋합니다.
			if (const FProjectERGameplayEffectContext* ERContext = static_cast<const FProjectERGameplayEffectContext*>(EffectContext))
			{
				if (ERContext->ClientActivationTime > 0.0f)
				{
					if (AGameStateBase* GameState = World->GetGameState())
					{
						float ServerTime = GameState->GetServerWorldTimeSeconds();
						float Latency = ServerTime - ERContext->ClientActivationTime;
						if (Latency > 0.0f)
						{
							// 발사체의 경우 이동 속도가 존재하므로, 속도 * 지연시간 만큼 앞으로 밀어줍니다.
							FVector Velocity = DeferredSpawnedActor->GetVelocity();
							if (!Velocity.IsNearlyZero())
							{
								FVector Correction = Velocity * Latency;
								DeferredSpawnedActor->AddActorWorldOffset(Correction);
							}
						}
					}
				}
			}
		}
	}
}


FTransform USummonRangeBaseGEC::CalculateSpawnTransform(const FGameplayEffectSpec& GESpec, const AActor* Instigator, const AActor* TargetActor) const
{
	const FTransform OriginTransform = CalculateOriginTransform(GESpec, Instigator, TargetActor);
	return ApplyCommonSpawnOptionsToTransform(OriginTransform, Instigator);
}

FTransform USummonRangeBaseGEC::CalculateOriginTransform(const FGameplayEffectSpec& GESpec, const AActor* Instigator, const AActor* TargetActor) const
{
	return FTransform::Identity;
}



AActor* USummonRangeBaseGEC::GetTargetActorFromContainer(FActiveGameplayEffectsContainer& ActiveGEContainer) const
{
	return ActiveGEContainer.Owner ? ActiveGEContainer.Owner->GetOwner() : nullptr;
}

bool USummonRangeBaseGEC::ShouldProcessOnInstigator(const AActor* Instigator) const
{
	return IsValid(Instigator);
}

FGameplayCueParameters USummonRangeBaseGEC::BuildNiagaraCueParameters(const FGameplayEffectSpec& GESpec, const FGameplayTag& OriginalTag, const FGameplayEffectContextHandle& EffectContext, AActor* EffectCauser, const FVector& CueLocation, const UObject* SourceObject, const FVector& CueNormal) const
{
	FGameplayCueParameters CueParams(GESpec);
	CueParams.OriginalTag = OriginalTag;
	CueParams.Instigator = EffectContext.GetInstigator();
	CueParams.EffectCauser = EffectCauser;
	CueParams.Location = CueLocation;
	CueParams.Normal = CueNormal;
	CueParams.GameplayEffectLevel = GESpec.GetLevel();

	if (SourceObject != nullptr)
	{
		CueParams.SourceObject = SourceObject;
	}

	return CueParams;
}

void USummonRangeBaseGEC::InitializeRangeActor(ABaseRangeOverlapEffectActor* RangeActor, AActor* Instigator, const FGameplayEffectContextHandle& Context, const FGameplayCueParameters& HitTargetVfxCueParameters, const FGameplayCueParameters& HitTargetSoundCueParameters) const
{
	if (!IsValid(RangeActor) || !IsValid(Instigator))
	{
		return;
	}

	UAbilitySystemComponent* const CauserASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(Instigator);
	UGameplayAbility* const Ability = const_cast<UGameplayAbility*>(Context.GetAbility());
	if (!IsValid(CauserASC) || !IsValid(Ability))
	{
		return;
	}

	TArray<FGameplayEffectSpecHandle> InitGEHandles;
	for (const TSubclassOf<UBaseGameplayEffect>& EffectClass : this->Applied)
	{
		if (!IsValid(EffectClass))
		{
			continue;
		}

		InitGEHandles.Add(CauserASC->MakeOutgoingSpec(TSubclassOf<UGameplayEffect>(EffectClass), Ability->GetAbilityLevel(), Context));
	}

	// 강화 효과(SkillProc) 확인 및 전이
	UBaseGEC::GetSkillProcEffects(CauserASC, Ability, RangeActor, Context, InitGEHandles);

	RangeActor->InitializeEffectData(InitGEHandles, Instigator, this->CollisionRadius, this->bHitOncePerTarget, nullptr, HitTargetVfxCueParameters, HitTargetSoundCueParameters);
	RangeActor->SetLifeSpan(this->LifeSpan);
}

void USummonRangeBaseGEC::SnapLocationToGround(FVector& InOutLocation, const AActor* Instigator) const
{
	if (!this->bSnapToGround)
	{
		return;
	}

	UWorld* const World = IsValid(Instigator) ? Instigator->GetWorld() : nullptr;
	if (!IsValid(World))
	{
		return;
	}

	FHitResult FloorHit;
	FVector TraceEnd = InOutLocation;
	TraceEnd.Z -= 1000.0f;

	FCollisionQueryParams QueryParams;
	if (IsValid(Instigator))
	{
		QueryParams.AddIgnoredActor(Instigator);
	}

	if (World->LineTraceSingleByChannel(FloorHit, InOutLocation, TraceEnd, this->GroundTraceChannel, QueryParams))
	{
		InOutLocation.X = FloorHit.Location.X;
		InOutLocation.Y = FloorHit.Location.Y;

		float FinalZOffset = this->FloatingHeight;
		if (this->bUseBoxExtentOffset)
		{
			FinalZOffset += this->CollisionRadius.Z;
		}

		InOutLocation.Z = FloorHit.Location.Z + FinalZOffset;
	}
}

void USummonRangeBaseGEC::ApplyCommonSpawnOptions(FVector& InOutLocation, FRotator& InOutRotation, const AActor* Instigator) const
{
	// 1. 회전 오프셋 적용
	InOutRotation += this->RotationOffset;

	// 2. 위치 오프셋 적용 (최종 회전값 기준)
	InOutLocation += InOutRotation.Quaternion().RotateVector(this->LocationOffset);

	// 3. 지면 스냅
	SnapLocationToGround(InOutLocation, Instigator);
}

FTransform USummonRangeBaseGEC::ApplyCommonSpawnOptionsToTransform(const FTransform& InOriginTransform, const AActor* Instigator) const
{
	FVector TargetLocation = InOriginTransform.GetLocation();
	FRotator CombinedRotation = InOriginTransform.Rotator();

	ApplyCommonSpawnOptions(TargetLocation, CombinedRotation, Instigator);

	return FTransform(CombinedRotation, TargetLocation);
}
