// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Abilities/GameplayAbility.h"
#include "SkillBase.generated.h"

/**
 * */

enum class ETargetRelationship : uint8;
class USkillDataAsset;
class UBaseGameplayEffect;
class UBaseSkillConfig;

UENUM(BlueprintType)
enum class ESkillAbilityState : uint8
{
	None,
	Casting,
	Active,
	Backswing
};

UCLASS()
class PROJECTER_API USkillBase : public UGameplayAbility
{
	GENERATED_BODY()

public:
	USkillBase();
	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;
	virtual void EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled) override;
	virtual void OnGiveAbility(const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilitySpec& Spec) override;
	
	/** 엔진 시전 로직 진입 전 데이터 유효성(사거리 등)을 검증합니다. */
	virtual bool ShouldAbilityRespondToEvent(const FGameplayAbilityActorInfo* ActorInfo, const FGameplayEventData* Payload) const override;
	
	FGameplayTag GetInputTag() const;
	virtual const FGameplayTagContainer* GetCooldownTags() const override;

protected:
	virtual void ApplyCooldown(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo) const override;
	virtual UGameplayEffect* GetCostGameplayEffect() const override;
	virtual void ApplyCost(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo) const;
	//virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
	virtual void ExecuteSkill();
	virtual void ApplyExecutionEffects();
	virtual void OnCancelAbility();
	virtual void OnExecuteSkill_InClient();
	virtual void CompleteFinishSkill();

	/** 스킬 활성화 실패 시 로그 출력 및 태그 브로드캐스팅 */
	//void NotifyActivationFailed(const FGameplayTag& ReasonTag, const FString& DebugMessage);

	/** 스킬 효과 적용 핵심 로직 - 중복 코드 제거를 위해 통합됨 */
	void ApplyEffectToTargetInternal(UAbilitySystemComponent* TargetASC, const TArray<TSubclassOf<UBaseGameplayEffect>>& Effects, FGameplayEffectContextHandle ContextHandle = FGameplayEffectContextHandle());

	void SetSkillTagCount(FGameplayTag Tag, int32 Count);
	void PlayAnimMontage();
	void SetWaitAnimationEvents();
	void PrepareToActiveSkill();
	
	/** 자신에게 효과 적용 (ApplyEffectToTargetInternal 호출) */
	void ApplyExcutionEffectToSelf(const TArray<TSubclassOf<UBaseGameplayEffect>>& SkillEffectDataAssets, FGameplayEffectContextHandle ContextHandle = FGameplayEffectContextHandle());
	bool TryExecuteSkill();

	/** 스킬 발동 시 대응되는 Gameplay Event를 발송합니다. */
	void SendExecuteEvent() const;

	/** 스킬 종료 시 대응되는 Gameplay Event를 발송합니다. */
	void SendEndEvent() const;

public:
	/** 피아 식별 여부를 체크하는 정적 유틸리티 함수 */
	UFUNCTION(BlueprintCallable, Category = "Skill|Targeting")
	static bool IsValidRelationship(AActor* Instigator, AActor* Target, ETargetRelationship Relationship);

protected:
	UFUNCTION()
	void OnSkillAnimationEventReceived(FGameplayEventData Payload);

	void ChangeSkillState(ESkillAbilityState NewState);

	UFUNCTION()
	void OnMontageInterrupted();

	UFUNCTION()
	void OnMontageCancelled();

	UFUNCTION()
	void OnMontageCompleted();

protected:
	FORCEINLINE UAbilitySystemComponent* GetASC() const { return GetAbilitySystemComponentFromActorInfo(); }
	FORCEINLINE AActor* GetAvatar() const { return GetAvatarActorFromActorInfo(); }

public:

protected:
	/** 클라이언트로부터 직렬화되어 전달된 정확한 시전 시작 시간 (동기화 및 렉보상용) */
	float SyncedActivationTime = 0.0f;

	UPROPERTY(BlueprintReadOnly)
	TObjectPtr<UBaseSkillConfig> CachedConfig;

	UPROPERTY(VisibleAnywhere, Category = "Skill|Tags")
	FGameplayTag CastingTag;

	UPROPERTY(VisibleAnywhere, Category = "Skill|Tags")
	FGameplayTag ActiveTag;

	UPROPERTY(VisibleAnywhere, Category = "Skill|Tags")
	FGameplayTag BackswingTag;

	//UPROPERTY(VisibleAnywhere, Category = "Skill|Tags")
	//FGameplayTag FailedOutOfRangeTag;

	UPROPERTY(BlueprintReadOnly, Category = "Skill")
	ESkillAbilityState CurrentState = ESkillAbilityState::None;

	/** 현재 다단 히트(Active)의 페이즈 인덱스를 추적합니다. */
	UPROPERTY(BlueprintReadOnly, Category = "Skill")
	int32 CurrentPhaseIndex = 0;

	UPROPERTY()
	TObjectPtr<UGameplayEffect> DynamicCostGE;

protected:
	void SetWaitEventBackswingTag();
};