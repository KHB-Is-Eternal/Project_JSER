#include "SkillSystem/GameplayEffectComponent/SummonRangeBaseGEC.h"

#include "Abilities/GameplayAbilityTypes.h"
#include "GameplayEffectTypes.h"
#include "AbilitySystemComponent.h"
#include "SkillSystem/GameplayEffect/BaseGameplayEffect.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemGlobals.h"
#include "GameplayCueManager.h"
#include "GameplayEffect.h"
#include "GameFramework/Actor.h"
#include "GameFramework/Pawn.h"
#include "SkillSystem/Actor/BaseRangeOverlapEffectActor/BaseRangeOverlapEffectActor.h"
#include "SkillSystem/GameplayEffect/BaseGameplayEffect.h"
#include "SkillSystem/GameAbility/SkillBase.h"

#include "SkillSystem/GameplayCueNotify/Particle/SkillNiagaraSpawnConfig.h"
#include "SkillSystem/GameplayCueNotify/Sound/SkillSoundSpawnConfig.h"
#include "SkillSystem/GameplayEffectComponent/SummonRangeAtBone.h"
#include "SkillSystem/GAS/ProjectERGameplayEffectContext.h"
#include "GameFramework/GameStateBase.h"
#include "GameFramework/GameState.h"

void USummonRangeBaseGEC::PreApplyEffect(UAbilitySystemComponent* ASC, const FGameplayEffectContextHandle& ContextHandle, const FGameplayEffectSpec& GESpec) const
{
	if (!IsValid(ASC)) return;

	AActor* const EffectInstigator = IsValid(ContextHandle.GetInstigator())
		? ContextHandle.GetInstigator()
		: ContextHandle.GetEffectCauser();
	if (!IsValid(EffectInstigator)) return;

	// 1. 타겟 식별 및 기준 위치(Origin) 계산
	AActor* const TargetActor = nullptr; // Target을 뽑아오는건 Container에서 해야 가장 정확하지만, 여기서는 Context의 HitResult 사용
	const FTransform OriginTransform = CalculateOriginTransform(GESpec, EffectInstigator, TargetActor);
	const FTransform SpawnTransform = ApplyCommonSpawnOptionsToTransform(OriginTransform, EffectInstigator);

	// 2. 보정된 결과(Origin)를 Context에 기록
	FProjectERGameplayEffectContext* const MutableContext = static_cast<FProjectERGameplayEffectContext*>(const_cast<FGameplayEffectContext*>(ContextHandle.Get()));
	if (MutableContext)
	{
		FHitResult SimulationHit;
		SimulationHit.Location = SpawnTransform.GetLocation();
		SimulationHit.Normal = SpawnTransform.Rotator().Vector();
		
		MutableContext->AddHitResult(SimulationHit, true);
		MutableContext->AddOrigin(SpawnTransform.GetLocation());
	}
}

void USummonRangeBaseGEC::OnExecutePredictive(UAbilitySystemComponent* ASC, const FGameplayEffectContextHandle& ContextHandle, const FGameplayEffectSpec& GESpec) const
{
	// [V7.3] 시전자 클라이언트의 예측 VFX 로직을 OnExecuteVFXCue로 통합하여 호출합니다.
	// SkillBase에서 IsLocallyControlled()일 때만 OnExecutePredictive를 호출하므로 안전합니다.

	// [Fix] 현재 스코프의 예측 키를 전달하여 로컬 예측 실행을 보장합니다.
	FPredictionKey PredictionKey;
	if (ASC) PredictionKey = ASC->ScopedPredictionKey;

	OnExecuteVFXCue(ASC, ContextHandle, GESpec, PredictionKey);
}

FSkillTooltipData USummonRangeBaseGEC::GetTooltipDescription(int32 Level, TSubclassOf<class USkillBase> AbilityClass) const
{
	FSkillTooltipData Data;
	Data.ShortDescription = FText::FromString(TEXT("범위 소환을 사용합니다."));

	FString DetailStr = TEXT("범위 소환 : 특정 범위가 소환됩니다. 닿은 대상에게 효과를 부여합니다.");
	
	FText EffectsText = FormatAppliedEffects(Applied, Level);
	if (!EffectsText.IsEmpty())
	{
		DetailStr += TEXT("\n") + EffectsText.ToString();
	}

	Data.DetailedDescription = FText::FromString(DetailStr);
	return Data;
}

void USummonRangeBaseGEC::OnExecuteVFXCue(UAbilitySystemComponent* ASC, const FGameplayEffectContextHandle& ContextHandle, const FGameplayEffectSpec& GESpec, FPredictionKey PredictionKey) const
{
	if (!IsValid(ASC)) return;
	if (!PredictionKey.IsValidKey()) PredictionKey = ASC->ScopedPredictionKey;

	FVector CueLocation = ContextHandle.GetOrigin();
	FVector CueDirection = FVector::UpVector;
	if (const FHitResult* Hit = ContextHandle.GetHitResult())
	{
		CueDirection = Hit->Normal;
	}

	if (IsValid(this->RangeSpawnVfx) && this->RangeSpawnVfx->CueTag.IsValid())
	{
		FGameplayCueParameters Params(GESpec);
		Params.Location = CueLocation;
		Params.Normal = CueDirection;
		Params.SourceObject = const_cast<USummonRangeBaseGEC*>(this);
		
		// 1. 시전자 정보 명시적 주입 (핸드셰이크 필수)
		Params.Instigator = ContextHandle.GetInstigator();
		Params.EffectCauser = ASC->GetAvatarActor();
		if (!Params.Instigator.IsValid()) Params.Instigator = Params.EffectCauser;

		{
			if (UGameplayCueManager* CueManager = UAbilitySystemGlobals::Get().GetGameplayCueManager())
			{
				CueManager->InvokeGameplayCueExecuted_WithParams(ASC, this->RangeSpawnVfx->CueTag, PredictionKey, Params);
			}
		}
	}
	else if (IsValid(this->RangeSpawnSound) && this->RangeSpawnSound->CueTag.IsValid())
	{
		// [Conditional] VFX 태그가 없어 비주얼 액터가 생성되지 않는 경우에만 직접 사운드 큐를 실행합니다.
		FGameplayCueParameters Params(GESpec);
		Params.Location = CueLocation;
		Params.Normal = CueDirection;
		Params.SourceObject = const_cast<USummonRangeBaseGEC*>(this);

		Params.Instigator = ContextHandle.GetInstigator();
		Params.EffectCauser = ASC->GetAvatarActor();
		if (!Params.Instigator.IsValid()) Params.Instigator = Params.EffectCauser;

		{
			if (UGameplayCueManager* CueManager = UAbilitySystemGlobals::Get().GetGameplayCueManager())
			{
				CueManager->InvokeGameplayCueExecuted_WithParams(ASC, this->RangeSpawnSound->CueTag, PredictionKey, Params);
			}
		}
	}
}

void USummonRangeBaseGEC::OnGameplayEffectApplied(FActiveGameplayEffectsContainer& ActiveGEContainer, FGameplayEffectSpec& GESpec, FPredictionKey& PredictionKey) const
{
	Super::OnGameplayEffectApplied(ActiveGEContainer, GESpec, PredictionKey);

	if (!ensure(RangeActorClass)) return;

	const FGameplayEffectContextHandle& ContextHandle = GESpec.GetEffectContext();
	AActor* const EffectInstigator = ContextHandle.GetInstigator() ? ContextHandle.GetInstigator() : ContextHandle.GetEffectCauser();
	
	if (!IsValid(EffectInstigator)) return;

	// 1. 소환 트랜스폼 계산
	FTransform SpawnTransform = GetInitialTransform(ContextHandle, ActiveGEContainer, GESpec, EffectInstigator);

	// 2. 권한 확인 및 실행 (서버 전용)
	if (ActiveGEContainer.Owner && ActiveGEContainer.Owner->IsOwnerActorAuthoritative())
	{
		// 관전자용 VFX 브로드캐스트
		OnExecuteVFXCue(ActiveGEContainer.Owner, ContextHandle, GESpec, PredictionKey);

		// 액터 소환 및 초기화
		if (UWorld* World = EffectInstigator->GetWorld())
		{
			if (ABaseRangeOverlapEffectActor* RangeActor = SpawnDeferredActor(World, RangeActorClass, SpawnTransform, EffectInstigator))
			{
				InitializeActorData(RangeActor, ContextHandle, GESpec, SpawnTransform);
				RangeActor->FinishSpawning(SpawnTransform);
				
				ApplyLagCompensation(RangeActor, ContextHandle);
			}
		}
	}
}

FTransform USummonRangeBaseGEC::GetInitialTransform(const FGameplayEffectContextHandle& ContextHandle, FActiveGameplayEffectsContainer& ActiveGEContainer, const FGameplayEffectSpec& GESpec, AActor* Instigator) const
{
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
		SpawnTransform = CalculateSpawnTransform(GESpec, Instigator, TargetActor);
	}
	return SpawnTransform;
}

ABaseRangeOverlapEffectActor* USummonRangeBaseGEC::SpawnDeferredActor(UWorld* World, TSubclassOf<ABaseRangeOverlapEffectActor> ActorClass, const FTransform& Transform, AActor* Instigator) const
{
	return World->SpawnActorDeferred<ABaseRangeOverlapEffectActor>(
		ActorClass,
		Transform,
		Instigator,
		Cast<APawn>(Instigator),
		ESpawnActorCollisionHandlingMethod::AlwaysSpawn);
}

void USummonRangeBaseGEC::InitializeActorData(ABaseRangeOverlapEffectActor* Actor, const FGameplayEffectContextHandle& ContextHandle, const FGameplayEffectSpec& GESpec, const FTransform& Transform) const
{
	const FVector Location = Transform.GetLocation();
	
	FGameplayCueParameters VfxParams = BuildNiagaraCueParameters(GESpec, HitTargetVfx ? HitTargetVfx->CueTag : FGameplayTag(), ContextHandle, Actor, Location, HitTargetVfx.Get());
	FGameplayCueParameters SfxParams = BuildNiagaraCueParameters(GESpec, HitTargetSound ? HitTargetSound->CueTag : FGameplayTag(), ContextHandle, Actor, Location, HitTargetSound.Get());

	if (const FProjectERGameplayEffectContext* ErContext = ProjectERContextUtils::GetProjectERContext(ContextHandle))
	{
		Actor->SetClientActivationTime(ErContext->ClientActivationTime);
	}

	InitializeRangeActor(Actor, ContextHandle.GetInstigator(), ContextHandle, VfxParams, SfxParams, GESpec);
	Actor->SetLifeSpan(this->LifeSpan);
}

void USummonRangeBaseGEC::ApplyLagCompensation(ABaseRangeOverlapEffectActor* Actor, const FGameplayEffectContextHandle& ContextHandle) const
{
	const FProjectERGameplayEffectContext* ERContext = ProjectERContextUtils::GetProjectERContext(ContextHandle);
	UWorld* World = Actor->GetWorld();

	if (ERContext && ERContext->ClientActivationTime > 0.0f && World)
	{
		if (AGameStateBase* GameState = World->GetGameState())
		{
			float Latency = GameState->GetServerWorldTimeSeconds() - ERContext->ClientActivationTime;
			if (Latency > 0.0f)
			{
				FVector Velocity = Actor->GetVelocity();
				if (!Velocity.IsNearlyZero())
				{
					Actor->AddActorWorldOffset(Velocity * Latency);
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

void USummonRangeBaseGEC::InitializeRangeActor(ABaseRangeOverlapEffectActor* RangeActor, AActor* Instigator, const FGameplayEffectContextHandle& Context, const FGameplayCueParameters& HitTargetVfxCueParameters, const FGameplayCueParameters& HitTargetSoundCueParameters, const FGameplayEffectSpec& ParentSpec) const
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

		FGameplayEffectSpecHandle Spec = CauserASC->MakeOutgoingSpec(TSubclassOf<UGameplayEffect>(EffectClass), Ability->GetAbilityLevel(), Context);
		UBaseGEC::InheritHitTags(ParentSpec, Spec);
		InitGEHandles.Add(Spec);
	}

	// 강화 효과(SkillProc) 확인 및 전이
	UBaseGEC::GetSkillProcEffects(CauserASC, Ability, RangeActor, Context, InitGEHandles, true, &ParentSpec);

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

void USummonRangeBaseGEC::CollectNiagaraPaths(TArray<FSoftObjectPath>& OutPaths) const
{
	Super::CollectNiagaraPaths(OutPaths);
	if (RangeSpawnVfx && !RangeSpawnVfx->NiagaraSystem.IsNull())
	{
		OutPaths.AddUnique(RangeSpawnVfx->NiagaraSystem.ToSoftObjectPath());
	}
	if (HitTargetVfx && !HitTargetVfx->NiagaraSystem.IsNull())
	{
		OutPaths.AddUnique(HitTargetVfx->NiagaraSystem.ToSoftObjectPath());
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
