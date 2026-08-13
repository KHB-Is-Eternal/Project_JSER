// Fill out your copyright notice in the Description page of Project Settings.


#include "SkillSystem/GameAbility/MouseClickSkill.h"
#include "Abilities/Tasks/AbilityTask_WaitTargetData.h"
#include "Abilities/Tasks/AbilityTask_WaitGameplayEvent.h"
#include "CharacterSystem/Character/BaseCharacter.h"
#include "Monster/BaseMonster.h"
#include "SkillSystem/GameplayAbilityTargetActor/MouseLocationTargetActor.h"
#include "SkillSystem/SkillConfig/BaseSkillConfig.h"
#include "SkillSystem/GameplayEffect/BaseGameplayEffect.h"
#include "SkillSystem/GameplayEffectComponent/BaseGEC.h"
#include "SkillSystem/GAS/ProjectERGameplayEffectContext.h"
#include "Kismet/KismetMathLibrary.h"
#include "AbilitySystemComponent.h"
#include "GameFramework/GameStateBase.h"
#include "SkillSystem/SkillDataAsset.h"

UMouseClickSkill::UMouseClickSkill()
{
	ExternalTargetLocationEventTag = FGameplayTag::RequestGameplayTag(FName("Skill.Data.Location"));
}

void UMouseClickSkill::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);

	if (TriggerEventData && TriggerEventData->TargetData.IsValid(0))
	{
		const FVector Location = TriggerEventData->TargetData.Get(0)->GetEndPoint();
		if (IsInRange(Location))
		{
			ExecuteSmartCast(*TriggerEventData);
			return;
		}
	}

	const bool bIsManual = (TriggerEventData != nullptr && !TriggerEventData->TargetData.IsValid(0));
	StartIndicatorMode(bIsManual);
}

void UMouseClickSkill::ExecuteSmartCast(const FGameplayEventData& EventData)
{
	if (EventData.TargetData.Num() <= 0 || !EventData.TargetData.Get(0)) return;
	const FVector Location = EventData.TargetData.Get(0)->GetEndPoint();
	
	UAbilitySystemComponent* ASC = GetAbilitySystemComponentFromActorInfo();
	if (!ASC) return;

	// 타겟팅 이펙트 컨텍스트 생성 및 위치 저장
	FGameplayEffectContextHandle ContextHandle = ASC->MakeEffectContext();
	ContextHandle.AddOrigin(Location);
	ContextHandle.AddSourceObject(this);
	TargetLocationEffectContext = ContextHandle;

	// 즉시 실행으로 분기
	RotateToLocation(Location);
	PrepareToActiveSkill();
}

void UMouseClickSkill::StartIndicatorMode(bool bIsManual)
{
	Super::StartIndicatorMode(bIsManual);
	SetWaitExternalTargetEventTask();
	SetWaitTargetTask();
}

bool UMouseClickSkill::ShouldAbilityRespondToEvent(const FGameplayAbilityActorInfo* ActorInfo, const FGameplayEventData* Payload) const
{
	if (!Super::ShouldAbilityRespondToEvent(ActorInfo, Payload)) return false;

	// 페이로드가 없거나 위치 정보가 없으면 기본 허용
	if (!Payload || !Payload->TargetData.IsValid(0)) return true;

	const FVector Location = Payload->TargetData.Get(0)->GetEndPoint();
	if (!Location.IsZero())
	{
		if (!IsInRange(Location))
		{
			if (ActorInfo->PlayerController.IsValid())
			{
				return true;
			}

			// 몬스터(AI)는 사거리 밖이면 즉시 시전 거부합니다.
			UE_LOG(LogTemp, Warning, TEXT("[MouseClickSkill] Rejected: Location out of range."));
			return false;
		}
	}

	return true;
}

bool UMouseClickSkill::TryGetMouseLocationInRange(FVector& OutLocation) const
{
	if (!IsLocallyControlled()) return false;

	const FVector TargetLocation = GetMouseLocation();
	if (!IsInRange(TargetLocation)) return false;

	OutLocation = TargetLocation;
	return true;
}

bool UMouseClickSkill::IsInRange(const FVector& Location) const
{
	UMouseClickSkillConfig* Config = Cast<UMouseClickSkillConfig>(CachedConfig);
	if (IsValid(Config) == false) {
		return false;
	}

	// 사거리 제한을 무시하도록 설정되어 있다면 무조건 참 반환
	if (Config->IgnoreRangeLimit())
	{
		return true;
	}

	AActor* Avatar = GetAvatarActorFromActorInfo();
	if (IsValid(Avatar) == false) {
		return false;
	}

	const FVector InstigatorLocation = Avatar->GetActorLocation();
	const float DistanceSquared = FVector::DistSquaredXY(Location, InstigatorLocation);
	const float RangeWithBuffer = GetMaxRange();

	return DistanceSquared <= FMath::Square(RangeWithBuffer + 50.0f); // 50.0f 버퍼 적용
}

void UMouseClickSkill::RotateToLocation(const FVector& Location)
{
	AActor* Avatar = GetAvatarActorFromActorInfo();
	if (!IsValid(Avatar)) return;

	ABaseCharacter* BaseCharacter = Cast<ABaseCharacter>(Avatar);
	if (IsValid(BaseCharacter))
	{
		BaseCharacter->StopMove();
	}

	const FVector StartLocation = Avatar->GetActorLocation();
	const FRotator LookAtRotation = UKismetMathLibrary::FindLookAtRotation(StartLocation, Location);

	FRotator NewRotation = Avatar->GetActorRotation();
	NewRotation.Yaw = LookAtRotation.Yaw;

	Avatar->SetActorRotation(NewRotation);
}

void UMouseClickSkill::ApplyExecutionEffects()
{
	const TArray<FSkillExecutionPhase>& Phases = CachedConfig->GetExecutionPhases();
	if (Phases.IsValidIndex(CurrentPhaseIndex))
	{
		const FGameplayEffectContext* EffectContext = TargetLocationEffectContext.Get();
		FGameplayEffectContextHandle ContextToUse;

		if (EffectContext && EffectContext->HasOrigin())
		{
			ContextToUse = TargetLocationEffectContext;
		}
		else
		{
			UE_LOG(LogTemp, Warning, TEXT("ApplyExecutionEffects::TargetLocationEffectContext has no valid origin. Falling back to default context."));
			UAbilitySystemComponent* const ASC = GetAbilitySystemComponentFromActorInfo();
			ContextToUse = IsValid(ASC) ? ASC->MakeEffectContext() : FGameplayEffectContextHandle();
		}

		ApplyExcutionEffectToSelf(Phases[CurrentPhaseIndex].Effects, ContextToUse);
	}
}


void UMouseClickSkill::OnCancelAbility()
{
	TargetLocationEffectContext = FGameplayEffectContextHandle();
	Super::OnCancelAbility();
}

void UMouseClickSkill::SetWaitExternalTargetEventTask()
{
	if (!ExternalTargetLocationEventTag.IsValid()) return;

	UAbilityTask_WaitGameplayEvent* WaitEventTask = UAbilityTask_WaitGameplayEvent::WaitGameplayEvent(this, ExternalTargetLocationEventTag, nullptr, false, true);
	if (!IsValid(WaitEventTask)) return;

	WaitEventTask->EventReceived.AddDynamic(this, &UMouseClickSkill::OnExternalTargetLocationReceived);
	WaitEventTask->ReadyForActivation();
}

TSubclassOf<class AGameplayAbilityTargetActor> UMouseClickSkill::GetTargetActorClass() const
{
	return AMouseLocationTargetActor::StaticClass();
}

void UMouseClickSkill::OnTargetDataReady(const FGameplayAbilityTargetDataHandle& DataHandle) 
{
	Super::OnTargetDataReady(DataHandle);

	if (!DataHandle.IsValid(0)) return;

	FVector Location = FVector::ZeroVector;
	const FGameplayAbilityTargetData* TargetData = DataHandle.Get(0);
	if (TargetData && TargetData->GetScriptStruct() == FGameplayAbilityTargetData_LocationInfo::StaticStruct())
	{
		const FGameplayAbilityTargetData_LocationInfo* LocationData = static_cast<const FGameplayAbilityTargetData_LocationInfo*>(TargetData);
		Location = LocationData->TargetLocation.GetTargetingTransform().GetLocation();
	}
	else if (TargetData)
	{
		Location = TargetData->GetEndPoint();
	}

	if (IsInRange(Location) == false)
	{
		EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, true);
		return;
	}

	AActor* Avatar = GetAvatarActorFromActorInfo();
	UAbilitySystemComponent* ASC = GetAbilitySystemComponentFromActorInfo();
	if (!ASC) return;

	FGameplayEffectContextHandle ContextHandle = ASC->MakeEffectContext();
	ContextHandle.AddOrigin(Location);
	ContextHandle.AddSourceObject(this);
	ContextHandle.SetAbility(this);
	ContextHandle.AddInstigator(Avatar, Avatar);
	TargetLocationEffectContext = ContextHandle.Duplicate();

	RotateToLocation(Location);
	PrepareToActiveSkill();
}

void UMouseClickSkill::OnTargetCancelled(const FGameplayAbilityTargetDataHandle& DataHandle)
{
	Super::OnTargetCancelled(DataHandle);
}

void UMouseClickSkill::OnExternalTargetLocationReceived(FGameplayEventData Payload)
{
	if (!Payload.TargetData.IsValid(0)) {
		UE_LOG(LogTemp, Warning, TEXT("OnExternalTargetLocationReceived::false"));
		return;
	}

	const FVector TargetLocation = Payload.TargetData.Get(0)->GetEndPoint();
	SubmitExternalTargetLocation(TargetLocation);
}

void UMouseClickSkill::SubmitExternalTargetLocation(const FVector& InLocation)
{
	if (!IsInRange(InLocation))
	{
		UE_LOG(LogTemp, Warning, TEXT("SubmitExternalTargetLocation::false"));
		return;
	}

	if (AMouseLocationTargetActor* MouseLocationTargetActor = Cast<AMouseLocationTargetActor>(CurrentTargetActor.Get()))
	{
		MouseLocationTargetActor->SubmitExternalLocation(InLocation);
		return;
	}

	PendingExternalTargetLocation = InLocation;
}

bool UMouseClickSkill::ConsumePendingExternalTargetLocation(FVector& OutLocation)
{
	if (!PendingExternalTargetLocation.IsSet()) return false;

	OutLocation = PendingExternalTargetLocation.GetValue();
	PendingExternalTargetLocation.Reset();
	return true;
}

bool UMouseClickSkill::IsTargetLocationInRange(const FVector& InLocation) const
{
	return IsInRange(InLocation);
}

FVector UMouseClickSkill::GetMouseLocation() const
{
	APlayerController* PC = GetActorInfo().PlayerController.Get();
	AActor* Avatar = GetAvatarActorFromActorInfo();
	if (!PC || !Avatar) return FVector::ZeroVector;

	FVector WorldLoc, WorldDir;
	if (PC->DeprojectMousePositionToWorld(WorldLoc, WorldDir))
	{
		FVector PlaneOrigin = Avatar->GetActorLocation(); // 캐릭터 발밑 높이
		FVector PlaneNormal = FVector::UpVector;
		FVector LineEnd = WorldLoc + WorldDir * 10000.f;

		// 1. 평행 체크 (분모가 0이 되는지 확인)
		// 방향 벡터와 평면 법선 벡터의 내적을 구합니다.
		float Denominator = FVector::DotProduct(WorldDir, PlaneNormal);

		// 분모가 0에 가깝지 않을 때만 (즉, 평면을 향하고 있을 때만) 함수 호출
		if (FMath::Abs(Denominator) > KINDA_SMALL_NUMBER)
		{
			return FMath::LinePlaneIntersection(WorldLoc, LineEnd, PlaneOrigin, PlaneNormal);
		}
	}
	return Avatar->GetActorLocation();
}

void UMouseClickSkill::EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled)
{
	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}

void UMouseClickSkill::ExecuteSkill()
{
	Super::ExecuteSkill();
}

void UMouseClickSkill::CompleteFinishSkill()
{
	Super::CompleteFinishSkill();
}

void UMouseClickSkill::CleanUpSkill()
{
	PendingExternalTargetLocation.Reset();
	AffectedActor = nullptr;
}

