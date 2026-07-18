// Fill out your copyright notice in the Description page of Project Settings.


#include "SkillSystem/GameAbility/MouseTargetSkill.h"
#include "Abilities/Tasks/AbilityTask_WaitGameplayEvent.h"
#include "Abilities/Tasks/AbilityTask_WaitTargetData.h"
#include "SkillSystem/SkillConfig/BaseSkillConfig.h"
#include "AbilitySystemInterface.h"
#include "AbilitySystemComponent.h"
#include "SkillSystem/GameplayEffect/BaseGameplayEffect.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "SkillSystem/GameplayAbilityTargetActor/TargetActor.h"
#include "SkillSystem/Actor/SkillIndicatorActor.h"
#include "SkillSystem/GameplayCueNotify/Components/GroundIndicatorComponent.h"
#include "Kismet/KismetMathLibrary.h"
#include "CharacterSystem/Character/BaseCharacter.h"
#include "SkillSystem/GAS/ProjectERGameplayEffectContext.h"
#include "SkillSystem/GameplayEffectComponent/BaseGEC.h"
#include "GameFramework/GameStateBase.h"
#include "GameFramework/GameState.h"

#define ECC_SKill ECC_GameTraceChannel6

UMouseTargetSkill::UMouseTargetSkill()
{
	ExternalTargetActorEventTag = FGameplayTag::RequestGameplayTag(FName("Skill.Data.Target"));
}

void UMouseTargetSkill::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);

	bool bHasValidTarget = false;

	if (TriggerEventData)
	{
		AActor* TargetActor = nullptr;

		// TargetData에서 추출 (ActorArray)
		if (TriggerEventData->TargetData.IsValid(0))
		{
			TArray<AActor*> TargetActors = UAbilitySystemBlueprintLibrary::GetActorsFromTargetData(TriggerEventData->TargetData, 0);
			if (TargetActors.Num() > 0)
			{
				TargetActor = TargetActors[0];
			}
		}

		// Target 필드에서 추출 (Fallback)
		if (!IsValid(TargetActor) && IsValid(TriggerEventData->Target))
		{
			TargetActor = const_cast<AActor*>(TriggerEventData->Target.Get());
		}

		if (IsValid(TargetActor))
		{
			bHasValidTarget = true;
			if (IsTargetActorInRange(TargetActor))
			{
				ExecuteSmartCast(*TriggerEventData);
				return;
			}
		}
	}

	const bool bIsManual = (TriggerEventData != nullptr && !bHasValidTarget);
	StartIndicatorMode(bIsManual);
}

void UMouseTargetSkill::ExecuteSmartCast(const FGameplayEventData& EventData)
{
	AActor* TargetActor = nullptr;

	if (EventData.TargetData.IsValid(0))
	{
		TArray<AActor*> TargetActors = UAbilitySystemBlueprintLibrary::GetActorsFromTargetData(EventData.TargetData, 0);
		if (TargetActors.Num() > 0)
		{
			TargetActor = TargetActors[0];
		}
	}

	if (!IsValid(TargetActor) && IsValid(EventData.Target))
	{
		TargetActor = const_cast<AActor*>(EventData.Target.Get());
	}

	if (IsValid(TargetActor))
	{
		AffectedActor = TargetActor;
		RotateToTarget(TargetActor);
		PrepareToActiveSkill();
	}
}

void UMouseTargetSkill::StartIndicatorMode(bool bIsManual)
{
	Super::StartIndicatorMode(bIsManual);
	SetWaitExternalTargetEventTask();
	SetWaitTargetTask();
}

bool UMouseTargetSkill::ShouldAbilityRespondToEvent(const FGameplayAbilityActorInfo* ActorInfo, const FGameplayEventData* Payload) const
{
	if (!Super::ShouldAbilityRespondToEvent(ActorInfo, Payload)) return false;

	// 페이로드가 없으면 기본 허용
	if (!Payload) return true;

	AActor* TargetActor = nullptr;

	// TargetData에서 추출
	if (Payload->TargetData.IsValid(0))
	{
		TArray<AActor*> TargetActors = UAbilitySystemBlueprintLibrary::GetActorsFromTargetData(Payload->TargetData, 0);
		if (TargetActors.Num() > 0)
		{
			TargetActor = TargetActors[0];
		}
	}

	// Target 필드에서 추출
	if (!IsValid(TargetActor) && IsValid(Payload->Target))
	{
		TargetActor = const_cast<AActor*>(Payload->Target.Get());
	}

	// 타겟이 있는 경우 사거리 및 관계 검증
	if (IsValid(TargetActor))
	{
		if (!IsTargetActorInRange(TargetActor))
		{
			// 플레이어는 사거리 밖이어도 어빌리티 진입을 허용합니다.
			// ActivateAbility에서 조준선(인디케이터) 대기 모드로 폴백합니다.
			if (ActorInfo->PlayerController.IsValid())
			{
				return true;
			}

			// 몬스터(AI)는 사거리 밖이면 즉시 시전 거부합니다.
			UE_LOG(LogTemp, Warning, TEXT("[MouseTargetSkill] Rejected: Target out of range or invalid relationship."));
			return false;
		}
	}

	return true;
}

void UMouseTargetSkill::ExecuteSkill()
{
	Super::ExecuteSkill();

	AActor* const TargetActor = AffectedActor.Get();
	if (!IsValid(TargetActor)) return;

	UMouseTargetSkillConfig* Config = Cast<UMouseTargetSkillConfig>(CachedConfig);
	if (!IsValid(Config)) return;

	const TArray<FTargetExecutionPhase>& TargetPhases = Config->GetTargetPhases();
	if (TargetPhases.IsValidIndex(CurrentPhaseIndex))
	{
		const TArray<TSubclassOf<UBaseGameplayEffect>>& EffectDataAssets = TargetPhases[CurrentPhaseIndex].TargetEffects;
		if (EffectDataAssets.Num() > 0)
		{
			ApplyEffectsTarget(TargetActor, EffectDataAssets, TargetPhases[CurrentPhaseIndex].MagnitudeCalculators);
		}
	}
}

void UMouseTargetSkill::CompleteFinishSkill()
{
	CleanUpSkill();
	Super::CompleteFinishSkill();
}

void UMouseTargetSkill::OnCancelAbility()
{
	CleanUpSkill();
	Super::OnCancelAbility();
}

void UMouseTargetSkill::SetWaitTargetTask()
{
	UAbilityTask_WaitTargetData* WaitTargetTask = UAbilityTask_WaitTargetData::WaitTargetData(
		this,
		TEXT("WaitTargetTask"),
		EGameplayTargetingConfirmation::UserConfirmed,
		ATargetActor::StaticClass()
	);

	WaitTargetTask->ValidData.AddDynamic(this, &UMouseTargetSkill::OnTargetDataReady);
	WaitTargetTask->Cancelled.AddDynamic(this, &UMouseTargetSkill::OnTargetCancelled);

	AGameplayAbilityTargetActor* SpawnedActor = nullptr;
	ATargetActor* MyTargetActor = nullptr;
	if (WaitTargetTask->BeginSpawningActor(this, ATargetActor::StaticClass(), SpawnedActor))
	{
		MyTargetActor = Cast<ATargetActor>(SpawnedActor);
		if (MyTargetActor)
		{
			CurrentTargetActor = MyTargetActor;
			MyTargetActor->PrimaryPC = Cast<APlayerController>(GetActorInfo().PlayerController);

			// 🌟 UMouseTargetSkillConfig에 세팅된 조준선 설정(사거리 포함)을 타겟 액터에 주입
			USkillDataAsset* DataAsset = GetSkillDataAsset();
			UMouseTargetSkillConfig* Config = Cast<UMouseTargetSkillConfig>(CachedConfig);

			FSkillRangeConfig TargetRangeConfig;
			if (Config != nullptr)
			{
				TargetRangeConfig = Config->GetRangeConfig();
			}

			FSkillIndicatorConfig SetupIndicatorConfig;
			if (DataAsset != nullptr)
			{
				SetupIndicatorConfig = DataAsset->GetIndicatorConfig();
			}
			MyTargetActor->Setup(SetupIndicatorConfig, TargetRangeConfig.Range);

			// 🌟 동적으로 캐릭터 발밑에 사거리 장판 생성
			AActor* Avatar = GetAvatarActorFromActorInfo();
			if (Avatar != nullptr)
			{
				ActiveRangeIndicatorComp = TargetRangeConfig.MakeGroundIndicatorComponent(Avatar);
			}

			WaitTargetTask->FinishSpawningActor(this, SpawnedActor);
		}
	}

	WaitTargetTask->ReadyForActivation();

	// 수동 조준(Alt 입력)인 상태에서는 즉시 컨펌하지 않고 대기 모드를 유지합니다.
	if (!bIsManualAiming && IsLocallyControlled())
	{
		if (IsValid(MyTargetActor)) {
			MyTargetActor->TryConfirmMouseTarget();
		}
	}
}

void UMouseTargetSkill::SetWaitExternalTargetEventTask()
{
	if (!ExternalTargetActorEventTag.IsValid()) return;

	UAbilityTask_WaitGameplayEvent* WaitEventTask = UAbilityTask_WaitGameplayEvent::WaitGameplayEvent(this, ExternalTargetActorEventTag, nullptr, false, true);
	if (!IsValid(WaitEventTask)) return;

	WaitEventTask->EventReceived.AddDynamic(this, &UMouseTargetSkill::OnExternalTargetActorReceived);
	WaitEventTask->ReadyForActivation();
}

void UMouseTargetSkill::SubmitExternalTargetActor(AActor* InTargetActor)
{
	if (!IsTargetActorInRange(InTargetActor))
	{
		UE_LOG(LogTemp, Warning, TEXT("SubmitExternalTargetActor::OutOfRange"));
		return;
	}

	if (CurrentTargetActor.IsValid())
	{
		CurrentTargetActor->SubmitExternalTarget(InTargetActor);
		return;
	}

	PendingExternalTargetActor = InTargetActor;
}

bool UMouseTargetSkill::ConsumePendingExternalTargetActor(AActor*& OutTargetActor)
{
	if (!PendingExternalTargetActor.IsValid()) return false;

	OutTargetActor = PendingExternalTargetActor.Get();
	PendingExternalTargetActor = nullptr;
	return true;
}


void UMouseTargetSkill::OnTargetDataReady(const FGameplayAbilityTargetDataHandle& DataHandle)
{
	ClearRangeIndicator();

	TArray<AActor*> TargetActors = UAbilitySystemBlueprintLibrary::GetActorsFromTargetData(DataHandle, 0);

	if (TargetActors.Num() <= 0)
	{
		return;
	}

	AffectedActor = nullptr;
	for (AActor* const Actor : TargetActors)
	{
		if (!IsValid(Actor)) continue;
		AffectedActor = Actor;
		RotateToTarget(Actor);
		break;
	}

	if (!AffectedActor.IsValid()) return;

	PrepareToActiveSkill();
}

void UMouseTargetSkill::OnTargetCancelled(const FGameplayAbilityTargetDataHandle& DataHandle)
{
	CurrentTargetActor = nullptr;
	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, true);
}

void UMouseTargetSkill::OnExternalTargetActorReceived(FGameplayEventData Payload)
{
	const AActor* Target = Payload.Target;

	if (IsValid(Target))
	{
		AActor* NonConstTarget = const_cast<AActor*>(Target);
		SubmitExternalTargetActor(NonConstTarget);
	}
}

AActor* UMouseTargetSkill::GetTargetUnderCursorInRange()
{
	if (IsLocallyControlled() == false) {
		return nullptr;
	}

	AActor* HitActor = GetTargetUnderCursor();

	if (!IsValid(HitActor)) return nullptr;

	if (IsTargetActorInRange(HitActor))
	{
		return HitActor;
	}

	return nullptr;
}

bool UMouseTargetSkill::IsTargetActorInRange(AActor* InTargetActor) const
{
	UMouseTargetSkillConfig* Config = Cast<UMouseTargetSkillConfig>(CachedConfig);
	if (!Config) return false;
	
	ETargetRelationship Rel = Config->GetApplyTo();
	return IsInRange(InTargetActor) && USkillBase::IsValidRelationship(GetAvatarActorFromActorInfo(), InTargetActor, Rel);
}

AActor* UMouseTargetSkill::GetTargetUnderCursor()
{
	APlayerController* PC = Cast<APlayerController>(GetActorInfo().PlayerController.Get());
	if (!PC) return nullptr;

	FHitResult HitResult;
	PC->GetHitResultUnderCursor(ECC_SKill, false, HitResult);

	return HitResult.GetActor();
}

bool UMouseTargetSkill::IsInRange(AActor* Actor) const
{
	if (!IsValid(Actor)) return false;

	AActor* Avatar = GetAvatarActorFromActorInfo();
	if (!IsValid(Avatar)) return false;

	UMouseTargetSkillConfig* Config = Cast<UMouseTargetSkillConfig>(CachedConfig);
	if (!ensureMsgf(IsValid(Config), TEXT("UMouseTargetSkill::IsInRange - Config Is Not Valid"))) { return false; }

	FVector TargetLocation = Actor->GetActorLocation();
	FVector InstigatorLocation = Avatar->GetActorLocation();

	float DistanceSquared = FVector::DistSquaredXY(TargetLocation, InstigatorLocation);

	float RangeWithBuffer = Config->GetRange();

	if (DistanceSquared <= FMath::Square(RangeWithBuffer + 50.0f)) // SkillBase와 동일하게 50.0f 버퍼 적용
	{
		return true;
	}

	return false;
}

void UMouseTargetSkill::RotateToTarget(AActor* Actor)
{
	// 1. 유효성 체크
	if (!IsValid(Actor)) return;

	AActor* Avatar = GetAvatarActorFromActorInfo();
	if (!IsValid(Avatar)) return;

	ABaseCharacter* BaseCharacter = Cast<ABaseCharacter>(Avatar);
	if (!IsValid(BaseCharacter)) return;
	BaseCharacter->StopMove();

	FVector StartLocation = Avatar->GetActorLocation();
	FVector TargetLocation = Actor->GetActorLocation();

	FRotator LookAtRotation = UKismetMathLibrary::FindLookAtRotation(StartLocation, TargetLocation);

	// 4. 수평 회전(Yaw)만 적용 (캐릭터가 위아래로 기울어지는 것 방지)
	FRotator NewRotation = Avatar->GetActorRotation();
	NewRotation.Yaw = LookAtRotation.Yaw;

	Avatar->SetActorRotation(NewRotation);
}

void UMouseTargetSkill::ApplyEffectsTarget(AActor* TargetActor, const TArray<TSubclassOf<UBaseGameplayEffect>>& SkillEffectDataAssets, const TArray<FSkillMagnitudeCalculation>& Calculators)
{
	UMouseTargetSkillConfig* Config = Cast<UMouseTargetSkillConfig>(CachedConfig);
	if (!Config) return;
	
	ETargetRelationship Rel = Config->GetApplyTo();

	// 1. 타겟 유효성 및 팀 관계 확인
	if (!IsValid(TargetActor) || !USkillBase::IsValidRelationship(GetAvatarActorFromActorInfo(), TargetActor, Rel))
	{
		return;
	}

	// 2. 공통 컨텍스트 생성 (타겟 위치 정보 포함)
	UAbilitySystemComponent* const SourceASC = GetASC();
	if (!ensure(SourceASC)) return;

	FGameplayEffectContextHandle ContextHandle = SourceASC->MakeEffectContext();
	
	FHitResult HitResult(TargetActor, nullptr, TargetActor->GetActorLocation(), FVector::UpVector);
	ContextHandle.AddHitResult(HitResult, true);
	ContextHandle.AddOrigin(TargetActor->GetActorLocation());

	// 3. 부모 클래스의 통합 로직 호출
	ApplyEffectToTargetInternal(UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(TargetActor), SkillEffectDataAssets, Calculators, ContextHandle);
}

void UMouseTargetSkill::CleanUpSkill()
{
	CurrentTargetActor = nullptr;
	AffectedActor = nullptr;

	ClearRangeIndicator();
}

void UMouseTargetSkill::EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled)
{
	ClearRangeIndicator();
	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}

void UMouseTargetSkill::ClearRangeIndicator()
{
	if (ActiveRangeIndicatorComp.IsValid())
	{
		ActiveRangeIndicatorComp->SetVisibility(false);
		ActiveRangeIndicatorComp->UnregisterComponent();
		ActiveRangeIndicatorComp->DestroyComponent();
		ActiveRangeIndicatorComp = nullptr;
	}
}

