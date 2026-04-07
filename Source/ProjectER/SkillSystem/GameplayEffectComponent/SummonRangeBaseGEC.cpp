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
#include "SkillSystem/GameAbility/SkillBase.h"
#include "SkillSystem/GameplayEffect/BaseGameplayEffect.h"

#include "SkillSystem/SkillNiagaraSpawnConfig.h"
#include "SkillSystem/SkillSoundSpawnConfig.h"
#include "SkillSystem/GameplayEffectComponent/SummonRangeAtBone.h"



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

	// 1. 타겟 식별 및 위치 계산
	AActor* const TargetActor = GetTargetActorFromContainer(ActiveGEContainer);
	const FTransform OriginTransform = CalculateOriginTransform(GESpec, EffectInstigator, TargetActor);
	const FTransform SpawnTransform = ApplyCommonSpawnOptionsToTransform(OriginTransform, EffectInstigator);
	const FVector RangeSpawnLocation = SpawnTransform.GetLocation();

	// 2. 액터 소환 (지연 생성)
	APawn* const SpawnInstigator = Cast<APawn>(ContextHandle.GetInstigator());
	ABaseRangeOverlapEffectActor* const DeferredSpawnedActor = World->SpawnActorDeferred<ABaseRangeOverlapEffectActor>(
		this->RangeActorClass,
		SpawnTransform,
		EffectInstigator,
		SpawnInstigator,
		ESpawnActorCollisionHandlingMethod::AlwaysSpawn);
	if (!IsValid(DeferredSpawnedActor))
	{
		return;
	}

	// 3. 초기화 및 마무리
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

	InitializeRangeActor(DeferredSpawnedActor, EffectInstigator, ContextHandle, HitTargetVfxCueParameters, HitTargetSoundCueParameters);
	DeferredSpawnedActor->FinishSpawning(SpawnTransform);

	// 4. 시각 효과 실행
	ExecuteGameplayCues(GESpec, ContextHandle, EffectInstigator, DeferredSpawnedActor, SpawnTransform, OriginTransform);
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

void USummonRangeBaseGEC::ExecuteGameplayCues(const FGameplayEffectSpec& GESpec, const FGameplayEffectContextHandle& ContextHandle, AActor* EffectInstigator, ABaseRangeOverlapEffectActor* RangeActor, const FTransform& SpawnTransform, const FTransform& OriginTransform) const
{
	UAbilitySystemComponent* const InstigatorASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(EffectInstigator);
	if (!IsValid(InstigatorASC))
	{
		return;
	}

	FScopedPredictionWindow ForcedWindow(InstigatorASC, FPredictionKey(), false);

	if (IsValid(this->SummonerSpawnVfx) && this->SummonerSpawnVfx->CueTag.IsValid())
	{
		// 시전자 나이아가라: 원점 좌표(OriginTransform) 사용, EffectCauser = RangeActor (기본 동작)
		const FGameplayCueParameters SummonerCueParams = BuildNiagaraCueParameters(GESpec, this->SummonerSpawnVfx->CueTag, ContextHandle, RangeActor, OriginTransform.GetLocation(), this->SummonerSpawnVfx);
		InstigatorASC->ExecuteGameplayCue(this->SummonerSpawnVfx->CueTag, SummonerCueParams);
	}

	if (IsValid(this->RangeSpawnVfx) && this->RangeSpawnVfx->CueTag.IsValid())
	{
		// 범위 나이아가라: 원점 좌표(OriginTransform) 사용, EffectCauser = RangeActor (기본 동작)
		FGameplayCueParameters RangeCueParams = BuildNiagaraCueParameters(GESpec, this->RangeSpawnVfx->CueTag, ContextHandle, RangeActor, OriginTransform.GetLocation(), this->RangeSpawnVfx, OriginTransform.GetRotation().GetForwardVector());
		if (IsValid(RangeActor))
		{
			RangeCueParams.TargetAttachComponent = RangeActor->GetRootComponent();
		}
		InstigatorASC->ExecuteGameplayCue(this->RangeSpawnVfx->CueTag, RangeCueParams);
	}

	// Sound 실행
	if (IsValid(this->SummonerSpawnSound) && this->SummonerSpawnSound->CueTag.IsValid())
	{
		const FGameplayCueParameters SummonerCueParams = BuildNiagaraCueParameters(GESpec, this->SummonerSpawnSound->CueTag, ContextHandle, RangeActor, OriginTransform.GetLocation(), this->SummonerSpawnSound);
		InstigatorASC->ExecuteGameplayCue(this->SummonerSpawnSound->CueTag, SummonerCueParams);
	}

	if (IsValid(this->RangeSpawnSound) && this->RangeSpawnSound->CueTag.IsValid())
	{
		FGameplayCueParameters RangeCueParams = BuildNiagaraCueParameters(GESpec, this->RangeSpawnSound->CueTag, ContextHandle, RangeActor, OriginTransform.GetLocation(), this->RangeSpawnSound, OriginTransform.GetRotation().GetForwardVector());
		if (IsValid(RangeActor))
		{
			RangeCueParams.TargetAttachComponent = RangeActor->GetRootComponent();
		}
		InstigatorASC->ExecuteGameplayCue(this->RangeSpawnSound->CueTag, RangeCueParams);
	}
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
	USkillBase* const NonConstSkill = const_cast<USkillBase*>(Cast<USkillBase>(Context.GetAbility()));
	if (!IsValid(CauserASC) || !IsValid(NonConstSkill))
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

		InitGEHandles.Add(CauserASC->MakeOutgoingSpec(TSubclassOf<UGameplayEffect>(EffectClass), NonConstSkill->GetAbilityLevel(), Context));
	}

	// 강화 효과(SkillProc) 확인 및 전이
	UBaseGEC::GetSkillProcEffects(CauserASC, NonConstSkill, RangeActor, Context, InitGEHandles);

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