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

	// 1. 이벤트 데이터 기반 즉시 실행 (Fast Track)
	// ShouldAbilityRespondToEvent에서 이미 사거리/타겟 검증이 완료되었으므로 타겟 유무만 확인합니다.
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
			AffectedActor = TargetActor;
			RotateToTarget(TargetActor);
			PrepareToActiveSkill();
			return;
		}
	}

	// 2. 이벤트 데이터가 없는 일반 케이스 (마우스 입력 대기)
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

	// 타겟이 있는 경우 개별 검증 수행
	if (IsValid(TargetActor))
	{
		if (!IsTargetActorInRange(TargetActor))
		{
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
			ApplyEffectsTarget(TargetActor, EffectDataAssets);
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
			WaitTargetTask->FinishSpawningActor(this, SpawnedActor);
		}
	}

	WaitTargetTask->ReadyForActivation();

	if (IsLocallyControlled())
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

void UMouseTargetSkill::ApplyEffectsTarget(AActor* TargetActor, const TArray<TSubclassOf<UBaseGameplayEffect>>& SkillEffectDataAssets)
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
	ApplyEffectToTargetInternal(UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(TargetActor), SkillEffectDataAssets, ContextHandle);
}

void UMouseTargetSkill::CleanUpSkill()
{
	CurrentTargetActor = nullptr;
	AffectedActor = nullptr;
}
