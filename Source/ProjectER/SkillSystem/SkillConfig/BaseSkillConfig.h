// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "SkillSystem/SkillData.h"
#include "SkillSystem/GameplayEffect/BaseGameplayEffect.h"
#include "BaseSkillConfig.generated.h"

class USkillBase;
class USkillDataAsset;
class USkillMagnitudeCalculator;

USTRUCT(BlueprintType)
struct FSkillCostInfo
{
	GENERATED_BODY()

public:
	// 소모할 스탯 (예: Mana, Stamina 등)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cost")
	FGameplayAttribute Attribute;

	// 소모량 (FScalableFloat를 사용하여 레벨/커브 대응)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cost")
	FScalableFloat CostValue;
};

/**
 * 프로젝트 내 모든 스킬/패시브 설정의 최상위 공통 베이스 클래스입니다.
 * USkillBase 및 USkillDataAsset과의 결합도를 낮추기 위해 다형성 가상 인터페이스를 제공합니다.
 */
UCLASS(BlueprintType, Abstract, EditInlineNew, DefaultToInstanced)
class PROJECTER_API UBaseSkillConfig : public UObject
{
	GENERATED_BODY()
	
public:
	UBaseSkillConfig();

	/** 어빌리티 실행 시 인스턴스화할 실제 GameplayAbility(USkillBase) 클래스 */
	UPROPERTY(VisibleAnywhere, Category = "Config", meta = (AllowPrivateAccess = "true"))
	TSubclassOf<USkillBase> AbilityClass;

	// ==========================================
	// 다형성 가상 인터페이스 (Virtual Getters)
	// ==========================================
	virtual FGameplayTag GetInputKeyTag() const { return FGameplayTag(); }
	virtual const FGameplayTagContainer* GetCooldownTags() const { return nullptr; }
	virtual float GetBaseCooldownDuration(float InLevel) const { return 0.0f; }
	virtual UAnimMontage* GetAnimMontage() const { return nullptr; }
	virtual const TArray<FSkillExecutionPhase>& GetExecutionPhases() const;

	virtual UGameplayEffect* CreateCostGameplayEffect(UObject* Outer) { return nullptr; }
	virtual FText BuildCostDescription(float InLevel = 1.0f) const { return FText::GetEmpty(); }
};

/**
 * 액티브 스킬용 공통 설정 베이스 클래스입니다.
 * 애니메이션, 입력 키, 마나 소모량, 실행 페이즈 등 액티브 고유의 속성을 모두 보유합니다.
 */
UCLASS(BlueprintType, Abstract, EditInlineNew, DefaultToInstanced)
class PROJECTER_API UActiveSkillConfig : public UBaseSkillConfig
{
	GENERATED_BODY()

public:
	UActiveSkillConfig();

	UPROPERTY(EditDefaultsOnly, Category = "DefaultData")
	FSkillDefaultData Data;

	UPROPERTY(EditDefaultsOnly, Category = "DefaultData|Cost")
	TArray<FSkillCostInfo> SkillCosts;

	// ==========================================
	// 다형성 가상 인터페이스 오버라이드
	// ==========================================
	virtual FGameplayTag GetInputKeyTag() const override { return Data.InputKeyTag; }
	virtual const FGameplayTagContainer* GetCooldownTags() const override { return &Data.CoolTimeTags; }
	virtual float GetBaseCooldownDuration(float InLevel) const override { return Data.BaseCoolTime.GetValueAtLevel(InLevel); }
	virtual UAnimMontage* GetAnimMontage() const override { return Data.AnimMontage; }
	virtual const TArray<FSkillExecutionPhase>& GetExecutionPhases() const override { return ExecutionPhases; }

	virtual UGameplayEffect* CreateCostGameplayEffect(UObject* Outer) override;
	virtual FText BuildCostDescription(float InLevel = 1.0f) const override;

protected:
	UPROPERTY(EditDefaultsOnly, Category = "Config")
	TArray<FSkillExecutionPhase> ExecutionPhases;
};

UCLASS(BlueprintType, EditInlineNew, DefaultToInstanced)
class PROJECTER_API UMouseTargetSkillConfig : public UActiveSkillConfig
{
	GENERATED_BODY()

public:
	UMouseTargetSkillConfig();
	FORCEINLINE float GetRange() const { return Range; }
	FORCEINLINE ETargetRelationship GetApplyTo() const { return ApplyTo; }
	FORCEINLINE const TArray<FTargetExecutionPhase>& GetTargetPhases() const { return TargetPhases; }
protected:
	UPROPERTY(EditDefaultsOnly, Category = "Config", meta = (AllowPrivateAccess = "true"))
	float Range;

	/** 이 스킬이 적용될 대상 (Enemy: 적, Friend: 아군) */
	UPROPERTY(EditDefaultsOnly, Category = "Config")
	ETargetRelationship ApplyTo = ETargetRelationship::Enemy;

	UPROPERTY(EditDefaultsOnly, Category = "Config")
	TArray<FTargetExecutionPhase> TargetPhases;
};

UCLASS(BlueprintType, EditInlineNew, DefaultToInstanced)
class PROJECTER_API UMouseClickSkillConfig : public UActiveSkillConfig
{
	GENERATED_BODY()

public:
	UMouseClickSkillConfig();
	FORCEINLINE float GetRange() const { return Range; }
protected:
	UPROPERTY(EditDefaultsOnly, Category = "Config", meta = (AllowPrivateAccess = "true"))
	float Range;
};

UCLASS(BlueprintType, EditInlineNew, DefaultToInstanced)
class PROJECTER_API UInstantSkillConfig : public UActiveSkillConfig
{
	GENERATED_BODY()

public:
	UInstantSkillConfig();
protected:
};

/** 태그 쿼리 조건 검사를 수행할 대상 액터를 지정합니다. */
UENUM(BlueprintType)
enum class EPassiveQueryTarget : uint8
{
	/** 어빌리티 소유자 자신 (일반적인 패시브 조건에 사용) */
	Self        UMETA(DisplayName = "Self"),
	/** 이벤트를 발생시킨 주체 (Payload.Instigator - 예: 나를 공격한 적) */
	Instigator  UMETA(DisplayName = "Instigator"),
	/** 이벤트를 적용받은 대상 (Payload.Target - 예: 내가 공격한 적) */
	Target      UMETA(DisplayName = "Target"),
};

/** 스탯(Attribute) 조건 비교 연산자 */
UENUM(BlueprintType)
enum class EAttributeCompareType : uint8
{
	GreaterThan			UMETA(DisplayName = "Greater Than (>)"),
	GreaterThanOrEqual	UMETA(DisplayName = "Greater Than Or Equal (>=)"),
	LessThan			UMETA(DisplayName = "Less Than (<)"),
	LessThanOrEqual		UMETA(DisplayName = "Less Than Or Equal (<=)"),
	Equal				UMETA(DisplayName = "Equal (==)")
};

/** 패시브 발동을 위한 개별 스탯(Attribute) 조건 설정 */
USTRUCT(BlueprintType)
struct PROJECTER_API FAttributeCondition
{
	GENERATED_BODY()

	/** 이 스탯 조건을 검사할 대상을 지정합니다. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Condition")
	EPassiveQueryTarget QueryTarget = EPassiveQueryTarget::Self;

	/** 시뮬레이션된 결과값을 어떻게 비교할지 지정합니다. (예: >, >=, <, <=, ==) */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Condition")
	EAttributeCompareType CompareType = EAttributeCompareType::GreaterThanOrEqual;

	/** 
	 * 시뮬레이션에 사용할 Modifier 정보.
	 * SourceTags/TargetTags를 통한 태그 검사가 수행되며, 
	 * Attribute가 설정된 경우 대상의 **현재 스탯(버프/디버프 반영)**에 
	 * ModifierOp와 Magnitude를 연산(시뮬레이션)하여 최종 비교에 사용합니다.
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Condition")
	FGameplayModifierInfo Modifier;

	/** 시뮬레이션 값과 최종 비교할 기준값 (Threshold) */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Condition")
	FScalableFloat ThresholdValue;
};

/**
 * 패시브 트리거 어빌리티(UWatchTagAbility_Base)의 공통 설정을 보관하는 추상 설정 클래스입니다.
 * 애니메이션, 입력 키 등 액티브 전용 필드들이 물리적으로 완전히 배제되어 있습니다.
 */
UCLASS(BlueprintType, Abstract, EditInlineNew, DefaultToInstanced)
class PROJECTER_API UPassiveSkillConfig : public UBaseSkillConfig
{
	GENERATED_BODY()

public:
	UPassiveSkillConfig();

	/** 구독할 게임플레이 이벤트 태그 컨테이너. 이 중 어떤 이벤트라도 발생하면 조건 검사를 수행합니다. */
	UPROPERTY(EditDefaultsOnly, Category = "Passive|Watch", meta = (Categories = "Event"))
	FGameplayTagContainer EventTagsToWatch;

	/** RequiredTagQuery를 어느 액터에게 실행할지 대상을 지정합니다. */
	UPROPERTY(EditDefaultsOnly, Category = "Passive|Condition")
	EPassiveQueryTarget QueryTarget = EPassiveQueryTarget::Self;

	/** QueryTarget 액터가 만족해야 하는 태그 조건. 비어있으면 항상 통과합니다. */
	UPROPERTY(EditDefaultsOnly, Category = "Passive|Condition")
	FGameplayTagQuery RequiredTagQuery;

	/** 추가적인 스탯(Attribute) 발동 조건 목록. 지정된 모든 조건을 만족해야(AND) 발동됩니다. */
	UPROPERTY(EditDefaultsOnly, Category = "Passive|Condition")
	TArray<FAttributeCondition> RequiredAttributeConditions;

	/**
	 * 조건 충족 시 발동할 스킬 데이터 에셋입니다.
	 * 설정된 경우 TriggerEffects보다 우선하여 실행됩니다.
	 */
	UPROPERTY(EditDefaultsOnly, Category = "Passive|Action")
	TSoftObjectPtr<class USkillDataAsset> TriggerAbility;

	/** 조건 충족 시 타겟에게 즉시 적용할 게임플레이 이펙트 목록. TriggerAbility가 없을 때 사용합니다. */
	UPROPERTY(EditDefaultsOnly, Category = "Passive|Action")
	TArray<TSubclassOf<class UBaseGameplayEffect>> Effects;

	/** 이 패시브 효과들에 적용할 SetByCaller 매그니튜드 설정 목록 */
	UPROPERTY(EditDefaultsOnly, Category = "Passive|Action")
	TArray<FSkillMagnitudeCalculation> MagnitudeCalculators;

	/** 이벤트 매그니튜드를 이펙트로 전달할 때 사용할 고정 SetByCaller 태그 */
	UPROPERTY(EditDefaultsOnly, Category = "Passive|Action")
	FGameplayTag SetByCallerTag;

	/** 패시브 발동 재사용 대기 시간(초) */
	UPROPERTY(EditDefaultsOnly, Category = "Passive|Cooldown")
	FScalableFloat BaseCoolTime;

	/** 패시브 쿨타임 시작 시 획득할 쿨타임 차단 태그 컨테이너 */
	UPROPERTY(EditDefaultsOnly, Category = "Passive|Cooldown", meta = (Categories = "Cooldown.Skill"))
	FGameplayTagContainer CoolTimeTags;

	// ==========================================
	// 다형성 가상 인터페이스 오버라이드
	// ==========================================
	virtual const FGameplayTagContainer* GetCooldownTags() const override { return &CoolTimeTags; }
	virtual float GetBaseCooldownDuration(float InLevel) const override { return BaseCoolTime.GetValueAtLevel(InLevel); }
};

/**
 * 즉발형 패시브 설정 클래스입니다.
 */
UCLASS(BlueprintType, EditInlineNew, DefaultToInstanced)
class PROJECTER_API UPassiveInstantSkillConfig : public UPassiveSkillConfig
{
	GENERATED_BODY()

public:
	UPassiveInstantSkillConfig();
};

/** 패시브 발동 시 GE로 넘겨줄 매그니튜드의 기준을 정의합니다. */
UENUM(BlueprintType)
enum class EAccumulateMagnitudeType : uint8
{
	/** 그동안 쌓인 누적 총합을 전달합니다. (예: 누적 피해량 폭발) */
	TotalAccumulatedValue UMETA(DisplayName = "Total Accumulated Value"),

	/** 임계치를 달성시킨 마지막 이벤트의 값을 전달합니다. (예: 10번째 타격의 데미지 비례 반사) */
	LastTriggerValue UMETA(DisplayName = "Last Trigger Value")
};

/**
 * 누적형 패시브 설정 클래스입니다.
 */
UCLASS(BlueprintType, EditInlineNew, DefaultToInstanced)
class PROJECTER_API UPassiveAccumulateSkillConfig : public UPassiveSkillConfig
{
	GENERATED_BODY()

public:
	UPassiveAccumulateSkillConfig();

	/** 발동에 필요한 누적 이벤트 횟수. 0이면 이 조건을 비활성화합니다. */
	UPROPERTY(EditDefaultsOnly, Category = "Passive|Accumulator", meta = (ClampMin = "0"))
	int32 RequiredEventCount = 0;

	/** 발동에 필요한 누적 이벤트 Magnitude 총합. 0이면 이 조건을 비활성화합니다. */
	UPROPERTY(EditDefaultsOnly, Category = "Passive|Accumulator", meta = (ClampMin = "0.0"))
	float RequiredTotalValue = 0.0f;

	/** true이면 임계치 초과분을 다음 사이클로 이월합니다. false이면 발동 후 0으로 완전 초기화합니다. */
	UPROPERTY(EditDefaultsOnly, Category = "Passive|Accumulator")
	bool bCarryOverExcess = false;

	/** 마지막 이벤트 수신 후 이 시간(초)이 경과하면 누적치를 초기화합니다. 0이면 영구 유지합니다. */
	UPROPERTY(EditDefaultsOnly, Category = "Passive|Accumulator", meta = (ClampMin = "0.0"))
	float ExpirationTime = 0.0f;

	/** 발동 시, GE의 SetByCaller 매그니튜드로 어떤 값을 넘겨줄지 결정합니다. */
	UPROPERTY(EditDefaultsOnly, Category = "Passive|Accumulator")
	EAccumulateMagnitudeType MagnitudeType = EAccumulateMagnitudeType::TotalAccumulatedValue;
};