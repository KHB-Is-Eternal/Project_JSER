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
#include "SkillSystem/Actor/SkillIndicatorActor.h"
#include "SkillSystem/GameplayCueNotify/Components/GroundIndicatorComponent.h"

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
	const FVector Location = EventData.TargetData.Get(0)->GetEndPoint();
	
	// 타겟팅 이펙트 컨텍스트 생성 및 위치 저장
	FGameplayEffectContextHandle ContextHandle = GetAbilitySystemComponentFromActorInfo()->MakeEffectContext();
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
			// 플레이어는 사거리 밖이어도 어빌리티 진입을 허용합니다.
			// ActivateAbility에서 조준선(인디케이터) 대기 모드로 폴백합니다.
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
	AActor* Avatar = GetAvatarActorFromActorInfo();
	if (IsValid(Avatar) == false) {
		//UE_LOG(LogTemp, Warning, TEXT("IsInRange ::IsValid(Avatar) == false"));
		return false;
	}

	UMouseClickSkillConfig* Config = Cast<UMouseClickSkillConfig>(CachedConfig);
	if (IsValid(Config) == false) {
		//UE_LOG(LogTemp, Warning, TEXT("IsInRange ::IsValid(Config) == false"));
		return false;
	}

	const FVector InstigatorLocation = Avatar->GetActorLocation();
	const float DistanceSquared = FVector::DistSquaredXY(Location, InstigatorLocation);
	const float RangeWithBuffer = Config->GetRange();

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
	CurrentMouseLocationTargetActor = nullptr;

	ClearRangeIndicator();

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

void UMouseClickSkill::SetWaitTargetTask()
{
	UAbilityTask_WaitTargetData* WaitTargetTask = UAbilityTask_WaitTargetData::WaitTargetData(
		this,
		TEXT("WaitMouseLocationTargetTask"),
		EGameplayTargetingConfirmation::UserConfirmed,
		AMouseLocationTargetActor::StaticClass()
	);

	WaitTargetTask->ValidData.AddDynamic(this, &UMouseClickSkill::OnTargetDataReady);
	WaitTargetTask->Cancelled.AddDynamic(this, &UMouseClickSkill::OnTargetCancelled);

	AGameplayAbilityTargetActor* SpawnedActor = nullptr;
	AMouseLocationTargetActor* MouseLocationTargetActor = nullptr;
	if (WaitTargetTask->BeginSpawningActor(this, AMouseLocationTargetActor::StaticClass(), SpawnedActor))
	{
		MouseLocationTargetActor = Cast<AMouseLocationTargetActor>(SpawnedActor);
		if (MouseLocationTargetActor)
		{
			CurrentMouseLocationTargetActor = MouseLocationTargetActor;
			MouseLocationTargetActor->PrimaryPC = Cast<APlayerController>(GetActorInfo().PlayerController);

			// 조준선 설정 및 최대 사거리 데이터 주입
			USkillDataAsset* DataAsset = GetSkillDataAsset();
			UMouseClickSkillConfig* ClickConfig = Cast<UMouseClickSkillConfig>(CachedConfig);
			
			FSkillRangeConfig FinalRangeConfig;
			if (IsValid(ClickConfig))
			{
				FinalRangeConfig = ClickConfig->GetRangeConfig();
			}

			FSkillIndicatorConfig SetupIndicatorConfig;
			if (DataAsset != nullptr)
			{
				SetupIndicatorConfig = DataAsset->GetIndicatorConfig();
			}
			MouseLocationTargetActor->Setup(SetupIndicatorConfig, FinalRangeConfig.Range);

			// 🌟 동적으로 캐릭터 발밑에 사거리 장판 생성
			AActor* Avatar = GetAvatarActorFromActorInfo();
			if (Avatar != nullptr)
			{
				ActiveRangeIndicatorComp = FinalRangeConfig.MakeGroundIndicatorComponent(Avatar);
			}

			WaitTargetTask->FinishSpawningActor(this, SpawnedActor);
		}
	}

	WaitTargetTask->ReadyForActivation();

	FVector PendingLocation = FVector::ZeroVector;
	if (ConsumePendingExternalTargetLocation(PendingLocation))
	{
		MouseLocationTargetActor->SubmitExternalLocation(PendingLocation);
		return;
	}

	// 수동 조준(Alt 입력)인 상태에서는 즉시 컨펌하지 않고 장판 대기 모드를 유지합니다.
	if (!bIsManualAiming)
	{
		APlayerController* PlayerController = Cast<APlayerController>(GetActorInfo().PlayerController.Get());
		const bool bCanUseMouseConfirm = IsLocallyControlled() && IsValid(PlayerController) && PlayerController->IsLocalPlayerController();
		if (bCanUseMouseConfirm)
		{
			MouseLocationTargetActor->TryConfirmMouseLocation();
		}
	}
}

void UMouseClickSkill::OnTargetDataReady(const FGameplayAbilityTargetDataHandle& DataHandle) 
{
	ClearRangeIndicator();

	if (!DataHandle.IsValid(0) /*|| !CommitAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo) */ ) return;

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
	FGameplayEffectContextHandle ContextHandle = GetAbilitySystemComponentFromActorInfo()->MakeEffectContext();
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
	CurrentMouseLocationTargetActor = nullptr;
	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, true);
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

	if (CurrentMouseLocationTargetActor.IsValid())
	{
		CurrentMouseLocationTargetActor->SubmitExternalLocation(InLocation);
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
	ClearRangeIndicator();
	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}

void UMouseClickSkill::ClearRangeIndicator()
{
	if (ActiveRangeIndicatorComp.IsValid())
	{
		ActiveRangeIndicatorComp->SetVisibility(false);
		ActiveRangeIndicatorComp->UnregisterComponent();
		ActiveRangeIndicatorComp->DestroyComponent();
		ActiveRangeIndicatorComp = nullptr;
	}
}

