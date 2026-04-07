// Fill out your copyright notice in the Description page of Project Settings.


#include "SkillSystem/GameplayEffectComponent/LaunchHomingMissile.h"
#include "SkillSystem/Actor/BaseMissileActor/BaseMissileActor.h"
#include "SkillSystem/GameplayEffect/BaseGameplayEffect.h"
#include "SkillSystem/GameAbility/SkillBase.h"
#include "SkillSystem/SkillNiagaraSpawnConfig.h"
#include "SkillSystem/SkillSoundSpawnConfig.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "GameplayEffect.h"
#include "GameplayEffectTypes.h"
#include "GameFramework/Character.h"
#include "Components/SkeletalMeshComponent.h"

// ============================================================================
// GEC
// ============================================================================

ULaunchHomingMissile::ULaunchHomingMissile()
{
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

	const FTransform SpawnTransform = CalculateSpawnTransform(Instigator, TargetActor);

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
	USkillBase* const Skill = const_cast<USkillBase*>(Cast<USkillBase>(ContextHandle.GetAbility()));

	TArray<FGameplayEffectSpecHandle> EffectSpecs;
	if (IsValid(CauserASC) && IsValid(Skill))
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

		// 강화 효과(SkillProc) 확인 및 전이
		UBaseGEC::GetSkillProcEffects(CauserASC, Skill, MissileActor, ContextHandle, EffectSpecs);
	}
	
	// --- 적중 효과(VFX, Sound) 큐 파라미터 구성 ---
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
	// SpawnTransform의 방향을 직접 추출 (FinishSpawning 이전이므로 GetActorForwardVector() 불신뢰)
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

	// --- 스폰 완료 ---
	MissileActor->FinishSpawning(SpawnTransform);

	// --- Summoner / Missile VFX & Sound 실행 ---
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

	// 1. 본(Bone) 위치 사용
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
		// 타겟의 중심이나 원격 위치 대신, 실제 유도 지점인 RootComponent의 위치를 정확히 조준합니다.
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
	UAbilitySystemComponent* const InstigatorASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(Instigator);
	if (!IsValid(InstigatorASC))
	{
		return;
	}

	{
		FScopedPredictionWindow ForcedWindow(InstigatorASC, FPredictionKey(), false);

		// Summoner VFX (시전자 위치에 재생)
		if (IsValid(this->SummonerVfx) && this->SummonerVfx->CueTag.IsValid())
		{
			FGameplayCueParameters SummonerParams(GESpec);
			SummonerParams.OriginalTag = this->SummonerVfx->CueTag;
			SummonerParams.Instigator = ContextHandle.GetInstigator();
			SummonerParams.EffectCauser = MissileActor;
			SummonerParams.Location = Instigator->GetActorLocation();
			SummonerParams.SourceObject = this->SummonerVfx;
			SummonerParams.GameplayEffectLevel = GESpec.GetLevel();
			InstigatorASC->ExecuteGameplayCue(this->SummonerVfx->CueTag, SummonerParams);
		}

		// Missile VFX (미사일 본체에 부착되어 이동하며 재생)
		if (IsValid(this->MissileVfx) && this->MissileVfx->CueTag.IsValid())
		{
			FGameplayCueParameters MissileParams(GESpec);
			MissileParams.OriginalTag = this->MissileVfx->CueTag;
			MissileParams.Instigator = ContextHandle.GetInstigator();
			MissileParams.EffectCauser = MissileActor;
			MissileParams.Location = MissileActor->GetActorLocation();
			MissileParams.Normal = MissileActor->GetActorForwardVector();
			MissileParams.SourceObject = this->MissileVfx;
			MissileParams.GameplayEffectLevel = GESpec.GetLevel();
			if (IsValid(MissileActor))
			{
				MissileParams.TargetAttachComponent = MissileActor->GetRootComponent();
			}
			InstigatorASC->ExecuteGameplayCue(this->MissileVfx->CueTag, MissileParams);
		}
	}
}

void ULaunchHomingMissile::ExecuteSound(
	const FGameplayEffectSpec& GESpec,
	const FGameplayEffectContextHandle& ContextHandle,
	AActor* Instigator,
	ABaseMissileActor* MissileActor) const
{
	UAbilitySystemComponent* const InstigatorASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(Instigator);
	if (!IsValid(InstigatorASC))
	{
		return;
	}

	{
		FScopedPredictionWindow ForcedWindow(InstigatorASC, FPredictionKey(), false);

		// Summoner Sound
		if (IsValid(this->SummonerSound) && this->SummonerSound->CueTag.IsValid())
		{
			FGameplayCueParameters SummonerParams(GESpec);
			SummonerParams.OriginalTag = this->SummonerSound->CueTag;
			SummonerParams.Instigator = ContextHandle.GetInstigator();
			SummonerParams.EffectCauser = MissileActor;
			SummonerParams.Location = Instigator->GetActorLocation();
			SummonerParams.SourceObject = this->SummonerSound;
			SummonerParams.GameplayEffectLevel = GESpec.GetLevel();
			InstigatorASC->ExecuteGameplayCue(this->SummonerSound->CueTag, SummonerParams);
		}

		// Missile Sound
		if (IsValid(this->MissileSound) && this->MissileSound->CueTag.IsValid())
		{
			FGameplayCueParameters MissileParams(GESpec);
			MissileParams.OriginalTag = this->MissileSound->CueTag;
			MissileParams.Instigator = ContextHandle.GetInstigator();
			MissileParams.EffectCauser = MissileActor;
			MissileParams.Location = MissileActor->GetActorLocation();
			MissileParams.Normal = MissileActor->GetActorForwardVector();
			MissileParams.SourceObject = this->MissileSound;
			MissileParams.GameplayEffectLevel = GESpec.GetLevel();
			if (IsValid(MissileActor))
			{
				MissileParams.TargetAttachComponent = MissileActor->GetRootComponent();
			}
			InstigatorASC->ExecuteGameplayCue(this->MissileSound->CueTag, MissileParams);
		}
	}
}
