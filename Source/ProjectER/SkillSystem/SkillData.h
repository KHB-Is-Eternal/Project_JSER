// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "GameplayEffect.h"
#include "SkillData.generated.h"

/**
 * 
 */

//UENUM(BlueprintType)
//enum class ESkillActivationType : uint8 {
//    Instant    UMETA(DisplayName = "Instant"),
//    Targeted   UMETA(DisplayName = "Targeted"),
//    PointClick UMETA(DisplayName = "PointClick"),
//    ClickAndDrag       UMETA(DisplayName = "ClickAndDrag"),
//    Holding    UMETA(DisplayName = "Holding")
//};

UENUM(BlueprintType)
enum class ETargetRelationship : uint8 {
    None UMETA(DisplayName = "None", Hidden),
    Friend     UMETA(DisplayName = "Friend"),
    Enemy   UMETA(DisplayName = "Enemy")
    //FriendAndEnemy   UMETA(DisplayName = "EnemyAndFriend")
};

class UAnimMontage; 

USTRUCT(BlueprintType)
struct FSkillDefaultData {
    GENERATED_BODY()

    /*UPROPERTY(EditDefaultsOnly, Category = "Skill")
    ESkillActivationType SkillActivationType;*/

    UPROPERTY(EditDefaultsOnly, Category = "Skill|Animation")
    TObjectPtr<UAnimMontage> AnimMontage;

    UPROPERTY(EditDefaultsOnly, Category = "Skill|Status|CoolDown")
    FScalableFloat BaseCoolTime;

    UPROPERTY(EditDefaultsOnly, Category = "Skill|Status|CoolDown", meta = (Categories = "Cooldown.Skill"))
    FGameplayTagContainer CoolTimeTags;

    UPROPERTY(EditDefaultsOnly, Category = "Skill|InputKey", meta = (Categories = "Input"))
    FGameplayTag InputKeyTag;
};

class UBaseGameplayEffect;
class USkillMagnitudeCalculator;

USTRUCT(BlueprintType)
struct FSkillMagnitudeCalculation
{
    GENERATED_BODY()

    // 이 계산값을 주입할 대상 GameplayEffect 클래스 (페이즈 내 적용할 GE 목록 중 하나 지정)
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Magnitude")
    TSubclassOf<class UBaseGameplayEffect> TargetGameplayEffect;

    // 이 GameplayEffect에 적용할 SetByCaller 태그
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Magnitude")
    FGameplayTag SetByCallerTag;

    // 이 GameplayEffect에 적용할 수식 계산기
    UPROPERTY(EditAnywhere, Instanced, BlueprintReadWrite, Category = "Magnitude")
    TObjectPtr<USkillMagnitudeCalculator> Calculator;
};

USTRUCT(BlueprintType)
struct FSkillExecutionPhase {
    GENERATED_BODY()
    
    // 이 페이즈 발동 시 시전자에게 적용될 효과들
    UPROPERTY(EditDefaultsOnly, Category = "Phase")
    TArray<TSubclassOf<UBaseGameplayEffect>> Effects;

    // 이 페이즈의 특정 GE에 적용할 SetByCaller 매그니튜드 설정 목록
    UPROPERTY(EditDefaultsOnly, Category = "Phase")
    TArray<FSkillMagnitudeCalculation> MagnitudeCalculators;

    // 이 페이즈 실행 시 Casting 태그가 유지되고 있어야 하는지 여부
    UPROPERTY(EditDefaultsOnly, Category = "Phase")
    bool bRequireCastingTag = false;
};

USTRUCT(BlueprintType)
struct FTargetExecutionPhase {
    GENERATED_BODY()
    
    // 이 페이즈 발동 시 타겟에게 적용될 효과들
    UPROPERTY(EditDefaultsOnly, Category = "Phase")
    TArray<TSubclassOf<UBaseGameplayEffect>> TargetEffects;

    // 이 페이즈의 특정 GE에 적용할 SetByCaller 매그니튜드 설정 목록
    UPROPERTY(EditDefaultsOnly, Category = "Phase")
    TArray<FSkillMagnitudeCalculation> MagnitudeCalculators;
};
