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

USTRUCT(BlueprintType)
struct FSkillExecutionPhase {
    GENERATED_BODY()
    
    // 이 페이즈 발동 시 시전자에게 적용될 효과들
    UPROPERTY(EditDefaultsOnly, Category = "Phase")
    TArray<TSubclassOf<UBaseGameplayEffect>> Effects;

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
};
