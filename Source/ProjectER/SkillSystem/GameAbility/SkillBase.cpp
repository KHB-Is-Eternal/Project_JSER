// Fill out your copyright notice in the Description page of Project Settings.


#include "SkillBase.h"
#include "ProjectER.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "Abilities/Tasks/AbilityTask_WaitGameplayTag.h"
#include "Abilities/Tasks/AbilityTask_PlayMontageAndWait.h"
#include "Abilities/Tasks/AbilityTask_WaitTargetData.h"
#include "Abilities/Tasks/AbilityTask_WaitGameplayEvent.h"
#include "Abilities/GameplayAbilityTargetActor.h"
#include "CharacterSystem/GAS/AttributeSet/BaseAttributeSet.h"
#include "SkillSystem/AbilityTask/AbilityTask_WaitGameplayEventSyn.h"
#include "SkillSystem/SkillConfig/BaseSkillConfig.h"
#include "SkillSystem/SkillDataAsset.h"
#include "SkillSystem/Calculator/SkillMagnitudeCalculator.h"
#include "SkillSystem/SkillData.h"
#include "SkillSystem/GameplayEffect/BaseGameplayEffect.h"
#include "SkillSystem/GameplayEffect/GE_SharedCooldown.h"
#include "Monster/BaseMonster.h"
#include "CharacterSystem/Character/BaseCharacter.h"
#include "CharacterSystem/Interface/TargetableInterface.h"
#include "GameModeBase/State/ER_PlayerState.h"

#include "AbilitySystemLog.h" // GAS 관련 로그 확인용
#include "CharacterSystem/GameplayTags/GameplayTags.h"
#include "AbilitySystemGlobals.h" // [김현수 추가분] 태그 체크용

#include "CharacterSystem/Player/BasePlayerController.h" // [김현수 추가분]
#include "SkillSystem/GAS/ProjectERGameplayEffectContext.h"
#include "GameFramework/GameStateBase.h"
#include "CharacterSystem/GameplayTags/GameplayTags.h"
#include "GameFramework/GameState.h"
#include "SkillSystem/GameplayEffectComponent/BaseGEC.h"

USkillBase::USkillBase()
{
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
	NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::LocalPredicted;
	ReplicationPolicy = EGameplayAbilityReplicationPolicy::ReplicateYes;
	CastingTag = ProjectER::Skill::Animation::Casting;
	ActiveTag = ProjectER::Skill::Animation::Active;
	BackswingTag = ProjectER::Skill::Animation::Backswing;
	//FailedOutOfRangeTag = FGameplayTag::RequestGameplayTag(FName("State.Failed.OutOfRange"));
	ActivationBlockedTags.AddTag(CastingTag);
	ActivationBlockedTags.AddTag(ActiveTag);
	ActivationBlockedTags.AddTag(ProjectER::State::Life::Death);
	ActivationBlockedTags.AddTag(ProjectER::State::Life::Down);
	// Hard CC: 모든 스킬 차단
	ActivationBlockedTags.AddTag(ProjectER::State::Debuff::Hard::Stun);
	ActivationBlockedTags.AddTag(ProjectER::State::Debuff::Hard::Airborne);
	// Soft CC: 침묵은 스킬 사용 차단 (이동은 가능)
	ActivationBlockedTags.AddTag(ProjectER::State::Debuff::Soft::Silence);
	
	CooldownGameplayEffectClass = UGE_SharedCooldown::StaticClass();
}

void USkillBase::SetSkillTagCount(FGameplayTag Tag, int32 Count)
{
	if (Tag.IsValid() && GetASC()) GetASC()->SetTagMapCount(Tag, Count);
}

void USkillBase::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
	CurrentPhaseIndex = 0;
	// [김현수 추가분]
	if (AActor* const AvatarActor = GetAvatarActorFromActorInfo())
	{
		if (APlayerController* const PC = Cast<APlayerController>(AvatarActor->GetInstigatorController()))
		{
			if (ABasePlayerController* const BasePC = Cast<ABasePlayerController>(PC))
			{
				if (BasePC->IsCrafting())
				{
					BasePC->CancelCrafting();
				}
			}
		}
	}

	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);
}

void USkillBase::EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled)
{
	if (bWasCancelled) {
		OnCancelAbility();
	}

	// 1. 스킬 종료(End) Gameplay Event 발송
	SendEndEvent();

	if (ActorInfo && ActorInfo->OwnerActor.IsValid())
	{
		if (ABaseMonster* Monster = Cast<ABaseMonster>(ActorInfo->OwnerActor.Get()))
		{
			// 몬스터의 StateTree에도 스킬 입력에 매핑되는 세부 종료 태그를 전송합니다.
			const FGameplayTag InputTag = GetInputTag();
			const FGameplayTag EndEventTag = ResolveSkillEventTag(
				InputTag,
				ProjectER::Event::Action::Skill::End::Q,
				ProjectER::Event::Action::Skill::End::W,
				ProjectER::Event::Action::Skill::End::E,
				ProjectER::Event::Action::Skill::End::R,
				ProjectER::Event::Action::Skill::End::Passive);

			if (EndEventTag.IsValid())
			{
				Monster->SendStateTreeEvent(EndEventTag);
			}
		}
	}

	ChangeSkillState(ESkillAbilityState::None);
	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}

void USkillBase::OnGiveAbility(const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilitySpec& Spec)
{
	Super::OnGiveAbility(ActorInfo, Spec);

	USkillDataAsset* DataAsset = Cast<USkillDataAsset>(Spec.SourceObject);
	CachedConfig = IsValid(DataAsset) ? DataAsset->SkillConfig : nullptr;
	DynamicCostGE = IsValid(CachedConfig) ? CachedConfig->CreateCostGameplayEffect(this) : nullptr;
}

bool USkillBase::ShouldAbilityRespondToEvent(const FGameplayAbilityActorInfo* ActorInfo, const FGameplayEventData* Payload) const
{
	return Super::ShouldAbilityRespondToEvent(ActorInfo, Payload);
}


//void USkillBase::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
//{
//	Super::PostEditChangeProperty(PropertyChangedEvent);
//}

void USkillBase::ApplyCooldown(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo) const
{
	UGameplayEffect* CooldownGE = GetCooldownGameplayEffect();
	UAbilitySystemComponent* const ASC = GetASC();

	if (CooldownGE && CachedConfig && IsValid(ASC))
	{
		// [Fix] 클라이언트에서 유효한 예측 키(PK)가 없는 경우 로컬 적용을 스킵합니다.
		// PK 없이 적용된 GE는 서버 GE와 Reconcile되지 않아 '유령 태그'를 남기는 주범이 됩니다.
		if (ASC->IsNetMode(NM_Client) && !ASC->ScopedPredictionKey.IsValidKey())
		{
			return;
		}

		FGameplayEffectSpecHandle SpecHandle = MakeOutgoingGameplayEffectSpec(CooldownGE->GetClass(), GetAbilityLevel());

		if (SpecHandle.IsValid())
		{
			float Duration = CachedConfig->GetBaseCooldownDuration(GetAbilityLevel());
			
			// Skill Haste (스킬 가속) 반영
			float Haste = ASC->GetNumericAttribute(UBaseAttributeSet::GetCooldownReductionAttribute());
			// 공식: 최종 쿨타임 = 기본 쿨타임 / (1 + (스킬가속 / 100))
			Duration /= (1.0f + (FMath::Max(Haste, 0.0f) / 100.0f));
			Duration = FMath::Max(Duration, 0.1f); // 최소 쿨타임 보장

			SpecHandle.Data.Get()->SetSetByCallerMagnitude(ProjectER::Skill::Data::CoolTime, Duration);
			if (const FGameplayTagContainer* CooldownTags = CachedConfig->GetCooldownTags())
			{
				SpecHandle.Data.Get()->DynamicGrantedTags.AppendTags(*CooldownTags);
			}

			ApplyGameplayEffectSpecToOwner(Handle, ActorInfo, ActivationInfo, SpecHandle);
		}
	}
}

const FGameplayTagContainer* USkillBase::GetCooldownTags() const
{
	// 데이터 에셋에 쿨타임 태그가 설정되어 있다면 그것을 우선적으로 반환합니다.
	if (CachedConfig && CachedConfig->GetCooldownTags() && CachedConfig->GetCooldownTags()->IsValid())
	{
		return CachedConfig->GetCooldownTags();
	}

	return nullptr;
}

UGameplayEffect* USkillBase::GetCostGameplayEffect() const
{
	if (IsValid(DynamicCostGE))
	{
		return DynamicCostGE;
	}

	if (IsValid(CachedConfig))
	{
		USkillBase* MutableThis = const_cast<USkillBase*>(this);
		MutableThis->DynamicCostGE = CachedConfig->CreateCostGameplayEffect(MutableThis);

		if (IsValid(DynamicCostGE))
		{
			return DynamicCostGE;
		}
	}

	return Super::GetCostGameplayEffect();
}

void USkillBase::ApplyCost(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo) const
{
	// 1. 동적으로 만든 GE가 있는지 확인
	if (DynamicCostGE && ActorInfo->AbilitySystemComponent.IsValid())
	{
		// 2. 엔진 함수를 거치지 않고 직접 Spec 인스턴스 생성 (중요!)
		// 생성자 파라미터: (UGameplayEffect 인스턴스, Context, 레벨)
		FGameplayEffectSpec* NewSpec = new FGameplayEffectSpec(DynamicCostGE, MakeEffectContext(Handle, ActorInfo), GetAbilityLevel(Handle, ActorInfo));
		FGameplayEffectSpecHandle SpecHandle(NewSpec);

		if (SpecHandle.IsValid())
		{
			FGameplayAbilitySpec* AbilitySpec = ActorInfo->AbilitySystemComponent->FindAbilitySpecFromHandle(Handle);
			ApplyAbilityTagsToGameplayEffectSpec(*SpecHandle.Data.Get(), AbilitySpec);
			ApplyGameplayEffectSpecToOwner(Handle, ActorInfo, ActivationInfo, SpecHandle);
			return;
		}
	}

	
	Super::ApplyCost(Handle, ActorInfo, ActivationInfo);
}

void USkillBase::ExecuteSkill()
{
	auto* ASC = GetASC();
	auto* Avatar = GetAvatar();
	
	if (!ensure(CachedConfig) || !IsValid(ASC) || !IsValid(Avatar))
	{
		EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, true);
		return;
	}

	// 1. 현재 페이즈의 자신 효과 적용
	ApplyExecutionEffects();

	// 2. 권한 서버에서 처리할 공통 로직 (예: 이동 정지)
	if (HasAuthority(&CurrentActivationInfo))
	{
		if (ABaseCharacter* Character = Cast<ABaseCharacter>(Avatar))
		{
			Character->StopMove();
		}
	}

	// 3. 스킬 발동(Execute) 이벤트 발송
	SendExecuteEvent();

	// 4. 디버그 로그 및 클라이언트 전용 이벤트
	if (ASC->IsNetMode(NM_Client))
	{
		UE_LOG(LogTemp, Log, TEXT("[SkillBase] ExecuteSkill on Client. HasValidPredictionKey: %s"), 
			ASC->ScopedPredictionKey.IsValidKey() ? TEXT("True") : TEXT("False"));
	}

	if (IsLocallyControlled())
	{
		OnExecuteSkill_InClient();
	}
}

void USkillBase::ApplyExecutionEffects()
{
	const TArray<FSkillExecutionPhase>& Phases = CachedConfig->GetExecutionPhases();
	if (Phases.IsValidIndex(CurrentPhaseIndex))
	{
		UAbilitySystemComponent* const ASC = GetASC();
		//FGameplayEffectContextHandle ContextHandle = IsValid(ASC) ? ASC->MakeEffectContext() : FGameplayEffectContextHandle();
		ApplyExcutionEffectToSelf(Phases[CurrentPhaseIndex].Effects, Phases[CurrentPhaseIndex].MagnitudeCalculators);
	}
}

void USkillBase::OnSkillAnimationEventReceived(FGameplayEventData Payload)
{
	FGameplayTag EventTag = Payload.EventTag;

	if (EventTag == CastingTag)
	{
		if (CachedConfig)
		{
			ChangeSkillState(ESkillAbilityState::Casting);
		}
	}
	else if (EventTag == ActiveTag)
	{
		// [수정] 매 타격(Active) 시점마다 TryExecuteSkill을 호출하여 상태를 철저히 검증합니다.
		if (!TryExecuteSkill())
		{
			// [V7.4 수정] 검증 실패 시 즉시 EndAbility를 부르지 않고 해당 타격만 무시합니다.
			// 이를 통해 데이터 불일치 등으로 인한 보스 스킬의 '뚝 끊김' 현상을 방지합니다.
			return;
		}

		// 상태 변경 (태그 토글용)
		ChangeSkillState(ESkillAbilityState::Active);
		
		// 클라이언트가 전달한 정확한 시전 시작 시간을 저장
		this->SyncedActivationTime = Payload.EventMagnitude;

		ExecuteSkill();
		
		// 실행 완료 후 다음 페이즈로 인덱스 증가
		CurrentPhaseIndex++;
	}
	else if (EventTag == BackswingTag)
	{
		ChangeSkillState(ESkillAbilityState::Backswing);
	}
}

void USkillBase::ChangeSkillState(ESkillAbilityState NewState)
{
	// [V7.4 최적화] 동일한 상태로 전이할 경우 태그 플래핑(0->1)을 방지하여 AI 안정성을 높입니다.
	if (CurrentState == NewState && NewState != ESkillAbilityState::None)
	{
		return;
	}

	// 모든 애니메이션 관련 태그 초기화
	SetSkillTagCount(CastingTag, 0);
	SetSkillTagCount(ActiveTag, 0);
	SetSkillTagCount(BackswingTag, 0);

	switch (NewState)
	{
	case ESkillAbilityState::Casting:   SetSkillTagCount(CastingTag, 1);   break;
	case ESkillAbilityState::Active:    SetSkillTagCount(ActiveTag, 1);    break;
	case ESkillAbilityState::Backswing: SetSkillTagCount(BackswingTag, 1); break;
	default: break;
	}


	CurrentState = NewState;
}

void USkillBase::OnMontageInterrupted()
{
	if (IsActive())
	{
		EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, true);
	}
}

void USkillBase::OnMontageCancelled()
{
	if (IsActive())
	{
		EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, true);
	}
}

void USkillBase::OnMontageCompleted()
{
	CompleteFinishSkill();
}

void USkillBase::PlayAnimMontage()
{
	UAnimMontage* SkillMontage = CachedConfig->GetAnimMontage();
	if (!IsValid(CachedConfig) || !IsValid(SkillMontage)) return;
	FName TaskName = FName(*SkillMontage->GetName());
    UAbilityTask_PlayMontageAndWait* PlayTask = UAbilityTask_PlayMontageAndWait::CreatePlayMontageAndWaitProxy(this, TaskName, SkillMontage, 1.f, NAME_None, true, 1.f, 0.f, true);
	if (!IsValid(PlayTask)) return;

	PlayTask->OnInterrupted.AddDynamic(this, &USkillBase::OnMontageInterrupted);
	PlayTask->OnCancelled.AddDynamic(this, &USkillBase::OnMontageCancelled);
	PlayTask->OnCompleted.AddDynamic(this, &USkillBase::OnMontageCompleted);
	PlayTask->ReadyForActivation();

	ABaseCharacter* BaseCharacter = Cast<ABaseCharacter>(GetAvatar());
	if (!IsValid(BaseCharacter)) return;
	BaseCharacter->StopMove();
}

void USkillBase::SetWaitAnimationEvents()
{
	// 커스텀 태스크가 계층형 매칭을 지원하지 않으므로, 각 태그에 대해 개별 태스크를 생성합니다.
	// 대신 콜백 함수는 하나로 통합하여 상태 머신 로직을 유지합니다.

	// 1. Casting
	UAbilityTask_WaitGameplayEventSyn* WaitCasting = UAbilityTask_WaitGameplayEventSyn::WaitEventClientToServer(this, CastingTag);
	WaitCasting->OnEventReceived.AddDynamic(this, &USkillBase::OnSkillAnimationEventReceived);
	WaitCasting->ReadyForActivation();

	// 2. Active
	UAbilityTask_WaitGameplayEventSyn* WaitActive = UAbilityTask_WaitGameplayEventSyn::WaitEventClientToServer(this, ActiveTag);
	WaitActive->OnEventReceived.AddDynamic(this, &USkillBase::OnSkillAnimationEventReceived);
	WaitActive->ReadyForActivation();

	// 3. Backswing
	UAbilityTask_WaitGameplayEventSyn* WaitBackswing = UAbilityTask_WaitGameplayEventSyn::WaitEventClientToServer(this, BackswingTag);
	WaitBackswing->OnEventReceived.AddDynamic(this, &USkillBase::OnSkillAnimationEventReceived);
	WaitBackswing->ReadyForActivation();
}

void USkillBase::PrepareToActiveSkill()
{
	if (!IsValid(CachedConfig)) return;

	if (!CommitAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo))
	{
		EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
		return;
	}

	SetWaitAnimationEvents();
	PlayAnimMontage();
	//if (IsLocallyControlled() || HasAuthority(&CurrentActivationInfo)) PlayAnimMontage();
}

void USkillBase::ApplyExcutionEffectToSelf(const TArray<TSubclassOf<UBaseGameplayEffect>>& SkillEffectDataAssets, FGameplayEffectContextHandle ContextHandle)
{
	ApplyEffectToTargetInternal(GetASC(), SkillEffectDataAssets, TArray<FSkillMagnitudeCalculation>(), ContextHandle);
}

void USkillBase::ApplyExcutionEffectToSelf(const TArray<TSubclassOf<UBaseGameplayEffect>>& SkillEffectDataAssets, const TArray<FSkillMagnitudeCalculation>& Calculators, FGameplayEffectContextHandle ContextHandle)
{
	ApplyEffectToTargetInternal(GetASC(), SkillEffectDataAssets, Calculators, ContextHandle);
}

void USkillBase::ApplyEffectToTargetInternal(UAbilitySystemComponent* TargetASC, const TArray<TSubclassOf<UBaseGameplayEffect>>& Effects, FGameplayEffectContextHandle ContextHandle)
{
	ApplyEffectToTargetInternal(TargetASC, Effects, TArray<FSkillMagnitudeCalculation>(), ContextHandle);
}

void USkillBase::ApplyEffectToTargetInternal(UAbilitySystemComponent* TargetASC, const TArray<TSubclassOf<UBaseGameplayEffect>>& Effects, const TArray<FSkillMagnitudeCalculation>& Calculators, FGameplayEffectContextHandle ContextHandle)
{
	UAbilitySystemComponent* const SourceASC = GetASC();
	AActor* const Avatar = GetAvatar();

	if (!ensure(SourceASC) || !ensure(Avatar) || !IsValid(TargetASC) || Effects.Num() <= 0)
	{
		return;
	}

	// 1. 컨텍스트 초기화 및 커스텀 데이터(시각 동기화) 주입
	if (ContextHandle.IsValid() == false)
	{
		ContextHandle = SourceASC->MakeEffectContext();
		ContextHandle.AddInstigator(Avatar, Avatar);
	}

	if (ContextHandle.GetInstigator() == nullptr || ContextHandle.GetInstigator()->IsA<AER_PlayerState>())
	{
		ContextHandle.AddInstigator(Avatar, Avatar);
	}

	if (ContextHandle.GetAbility() == nullptr)
	{
		ContextHandle.SetAbility(this);
	}

	if(ContextHandle.HasOrigin() == false){
		ContextHandle.AddOrigin(Avatar->GetActorLocation());
	}

	if (FProjectERGameplayEffectContext* ERContext = ProjectERContextUtils::GetMutableProjectERContext(ContextHandle))
	{
		if (this->SyncedActivationTime > 0.0f)
		{
			ERContext->ClientActivationTime = this->SyncedActivationTime;
		}
		else if (UWorld* World = GetWorld())
		{
			if (AGameStateBase* GameState = World->GetGameState())
			{
				ERContext->ClientActivationTime = GameState->GetServerWorldTimeSeconds();
			}
		}
	}

	// [Hook 1] 컨텍스트가 막 생성되고 Instigator 및 동기화 설정이 끝난 직후 호출
	OnEffectContextCreated(ContextHandle);

	// 2. 각 이팩트 순회하며 적용
	for (const TSubclassOf<UBaseGameplayEffect>& EffectClass : Effects)
	{
		if (!IsValid(EffectClass)) continue;

		// 개별 이펙트용 컨텍스트 독립화 (오염 방지)
		FGameplayEffectContextHandle EffectSpecificContext = ContextHandle.Duplicate();
		FGameplayEffectSpecHandle SpecHandle = SourceASC->MakeOutgoingSpec(EffectClass, GetAbilityLevel(), EffectSpecificContext);
		
		if (SpecHandle.IsValid())
		{
			// [Hook 2] 개별 이펙트의 Spec이 MakeOutgoingSpec으로 막 생성된 직후 호출
			OnEffectSpecCreated(SpecHandle);

			// 페이즈의 계산기들 중 현재 이펙트와 매칭되는 대상이 있으면 SetByCaller 주입
			for (const FSkillMagnitudeCalculation& CalcInfo : Calculators)
			{
				if (CalcInfo.TargetGameplayEffect == EffectClass && CalcInfo.Calculator && CalcInfo.SetByCallerTag.IsValid())
				{
					float CalcValue = CalcInfo.Calculator->CalculateValue(SourceASC, TargetASC);
					SpecHandle.Data.Get()->SetSetByCallerMagnitude(CalcInfo.SetByCallerTag, CalcValue);
				}
			}

			// 스킬 입력키에 맞는 피격 태그 추가
			const FGameplayTag SkillHitTag = ResolveSkillEventTag(
				GetInputTag(),
				ProjectER::Event::Action::Hit::Skill::Q,
				ProjectER::Event::Action::Hit::Skill::W,
				ProjectER::Event::Action::Hit::Skill::E,
				ProjectER::Event::Action::Hit::Skill::R,
				ProjectER::Event::Action::Hit::Skill::Passive);

			if (SkillHitTag.IsValid())
			{
				SpecHandle.Data.Get()->DynamicGrantedTags.AddTag(SkillHitTag);
			}

			// [GEC Hook] Phase 1, 2: 좌표 보정 및 클라이언트 예측 비주얼 생성
			if (const UBaseGameplayEffect* GEInstance = Cast<UBaseGameplayEffect>(EffectClass->GetDefaultObject()))
			{
				for (const TObjectPtr<UGameplayEffectComponent>& Component : GEInstance->GetGEComponents())
				{
					if (const UBaseGEC* BaseGEC = Cast<UBaseGEC>(Component))
					{
						FGameplayEffectContextHandle SpecContext = SpecHandle.Data.Get()->GetContext();

						BaseGEC->PreApplyEffect(SourceASC, SpecContext, *SpecHandle.Data.Get());
						
						// [Fix] 리슨 서버 호스트의 경우 권한을 가지고 있으므로 예측 로직을 실행하지 않도록 조건을 강화합니다.
						if (IsLocallyControlled() && !SourceASC->IsOwnerActorAuthoritative())
						{
							BaseGEC->OnExecutePredictive(SourceASC, SpecContext, *SpecHandle.Data.Get());
						}
					}
				}
			}

			// Phase 3: 최종 적용
			// [Hook 3] 모든 처리(태그, 예측 등)가 끝나고 타겟에게 최종 적용되기 직전 호출
			OnPreApplyEffectSpec(SpecHandle, TargetASC);

			// [Fix] 서버에서 Target에게 예측 키를 강제로 전파하여 GEC까지 전달되도록 합니다.
			// ScopedPK가 유실된 경우 Ability의 ActivationPK를 백업으로 사용하여 랜덤성을 해결합니다.
			FPredictionKey BestPK = SourceASC->ScopedPredictionKey;
			if (!BestPK.IsValidKey()) 
			{
				BestPK = GetCurrentActivationInfo().GetActivationPredictionKey();
			}
			
			FScopedPredictionWindow TargetScopedWindow(TargetASC, BestPK);
			SourceASC->ApplyGameplayEffectSpecToTarget(*SpecHandle.Data.Get(), TargetASC);
		}
	}
}

bool USkillBase::TryExecuteSkill()
{
	auto* ASC = GetASC();
	if (!IsValid(ASC) || !IsValid(CachedConfig)) {
		return false;
	}

	const TArray<FSkillExecutionPhase>& Phases = CachedConfig->GetExecutionPhases();
	
	// [수정] 해당 페이즈에 캐스팅 태그가 필수인 경우에만 체크합니다.
	// 페이즈 데이터 자체가 없다면 캐스팅 체크를 건너뜁니다. (타겟 효과만 있는 스킬 대응)
	if (Phases.IsValidIndex(CurrentPhaseIndex))
	{
		if (Phases[CurrentPhaseIndex].bRequireCastingTag && ASC->HasMatchingGameplayTag(CastingTag) == false) {
			return false;
		}
	}

	FGameplayTagContainer RelevantTags;
	if (!DoesAbilitySatisfyTagRequirements(*ASC, nullptr, nullptr, &RelevantTags)) {
		// 만약 차단된 원인이 오직 CastingTag나 ActiveTag뿐이라면 (자기 자신에 의한 차단), 통과시킵니다.
		RelevantTags.RemoveTag(UAbilitySystemGlobals::Get().ActivateFailTagsBlockedTag);
		
		bool bOnlySelfBlocked = true;
		for (const FGameplayTag& Tag : RelevantTags) {
			if (Tag != CastingTag && Tag != ActiveTag) {
				bOnlySelfBlocked = false;
				break;
			}
		}

		if (bOnlySelfBlocked) {
			return true;
		}
		
		return false;
	}

	return true;
}

void USkillBase::CompleteFinishSkill()
{
	//SetSkillTagCount(ActiveTag, 0);
	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
}

FGameplayTag USkillBase::GetInputTag() const
{
	return CachedConfig ? CachedConfig->GetInputKeyTag() : FGameplayTag();
}

void USkillBase::SendExecuteEvent() const
{
	const FGameplayTag EventTag = ResolveSkillEventTag(
		GetInputTag(),
		ProjectER::Event::Action::Skill::Execute::Q,
		ProjectER::Event::Action::Skill::Execute::W,
		ProjectER::Event::Action::Skill::Execute::E,
		ProjectER::Event::Action::Skill::Execute::R,
		ProjectER::Event::Action::Skill::Execute::Passive);

	SendSkillEvent(EventTag);
}

void USkillBase::SendEndEvent() const
{
	const FGameplayTag EventTag = ResolveSkillEventTag(
		GetInputTag(),
		ProjectER::Event::Action::Skill::End::Q,
		ProjectER::Event::Action::Skill::End::W,
		ProjectER::Event::Action::Skill::End::E,
		ProjectER::Event::Action::Skill::End::R,
		ProjectER::Event::Action::Skill::End::Passive);

	SendSkillEvent(EventTag);
}

FGameplayTag USkillBase::ResolveSkillEventTag(
	const FGameplayTag& InputTag,
	const FGameplayTag& QTag,
	const FGameplayTag& WTag,
	const FGameplayTag& ETag,
	const FGameplayTag& RTag,
	const FGameplayTag& PassiveTag) const
{
	if (InputTag.MatchesTag(ProjectER::Ability::Input::Skill::Q))       return QTag;
	if (InputTag.MatchesTag(ProjectER::Ability::Input::Skill::W))       return WTag;
	if (InputTag.MatchesTag(ProjectER::Ability::Input::Skill::E))       return ETag;
	if (InputTag.MatchesTag(ProjectER::Ability::Input::Skill::R))       return RTag;
	if (InputTag.MatchesTag(ProjectER::Ability::Input::Skill::Passive)) return PassiveTag;
	return FGameplayTag();
}

void USkillBase::SendSkillEvent(const FGameplayTag& EventTag) const
{
	if (!EventTag.IsValid())
	{
		return;
	}

	AActor* const Avatar = GetAvatar();
	if (!IsValid(Avatar))
	{
		return;
	}

	FGameplayEventData Payload;
	Payload.EventTag = GetInputTag();
	Payload.Instigator = Avatar;
	Payload.Target = Avatar;

	UAbilitySystemBlueprintLibrary::SendGameplayEventToActor(Avatar, EventTag, Payload);
}

bool USkillBase::IsValidRelationship(AActor* Instigator, AActor* Target, ETargetRelationship Relationship)
{
	if (!IsValid(Instigator) || !IsValid(Target)) return false;

	auto* I_Instigator = Cast<ITargetableInterface>(Instigator);
	auto* I_Target = Cast<ITargetableInterface>(Target);

	bool IsInstigatorImplementsInterface = Instigator->GetClass()->ImplementsInterface(UTargetableInterface::StaticClass());
	bool TargetImplementsInterface = Target->GetClass()->ImplementsInterface(UTargetableInterface::StaticClass());

	if (!IsInstigatorImplementsInterface || !TargetImplementsInterface)
	{
		return false;
	}

	if (!I_Instigator || !I_Target) return false;

	bool bIsSameTeam = (I_Instigator->GetTeamType() == I_Target->GetTeamType());

	if (Relationship == ETargetRelationship::Friend) return bIsSameTeam;
	if (Relationship == ETargetRelationship::Enemy)  return !bIsSameTeam && I_Target->IsTargetable();

	return false;
}

void USkillBase::OnCancelAbility()
{

}

void USkillBase::OnExecuteSkill_InClient()
{

}

/*
void USkillBase::NotifyActivationFailed(const FGameplayTag& ReasonTag, const FString& DebugMessage)
{
	UE_LOG(LogTemp, Warning, TEXT("[SkillBase] Activation Failed: %s, Reason: %s"), *GetName(), *DebugMessage);

	if (ReasonTag.IsValid())
	{
		UAbilitySystemComponent* ASC = GetASC();
		if (IsValid(ASC))
		{
			// 실패 이벤트를 브로드캐스팅하여 AI나 다른 시스템이 실패를 인지할 수 있도록 함
			FGameplayEventData Payload;
			Payload.EventTag = ReasonTag;
			Payload.Instigator = GetAvatar();
			Payload.Target = GetAvatar();
			
			UAbilitySystemBlueprintLibrary::SendGameplayEventToActor(GetAvatar(), ReasonTag, Payload);
		}
	}
}
*/