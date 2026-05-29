// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "SkillData.h"
#include "SkillDataAsset.generated.h"

/**
 * 
 */


USTRUCT(BlueprintType)
struct FSkillTooltipData
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly, Category = "Skill")
    FText SkillName;

    UPROPERTY(BlueprintReadOnly, Category = "Skill")
    FText ShortDescription;

    UPROPERTY(BlueprintReadOnly, Category = "Skill")
    FText DetailedDescription;

    UPROPERTY(BlueprintReadOnly, Category = "Skill")
    FText SkillEffectDescription;

    UPROPERTY(BlueprintReadOnly, Category = "Skill")
    float CooldownSeconds = 0.0f;

    UPROPERTY(BlueprintReadOnly, Category = "Skill")
    FText CostDescription;

    UPROPERTY(BlueprintReadOnly, Category = "UI")
    TObjectPtr<UTexture2D> SKillIcon;
};

class UAbilitySystemComponent;
class USkillBase;
class UBaseSkillConfig;

UCLASS()
class PROJECTER_API USkillDataAsset : public UDataAsset
{
	GENERATED_BODY()
public:
    FGameplayAbilitySpec MakeSpec();

    UFUNCTION(BlueprintPure, Category = "Skill|UI")
    FSkillTooltipData GetSkillTooltipData(int32 InLevel = 1) const;

    static FString GetTargetingStyleText(TSubclassOf<class USkillBase> AbilityClass);

    UFUNCTION(BlueprintPure, Category = "Skill|UI")
	UTexture2D* GetSkillIcon() const { return SKillIcon; }
private:
    
public:
    UPROPERTY(EditDefaultsOnly, Instanced)
    TObjectPtr<UBaseSkillConfig> SkillConfig;

protected:
    UPROPERTY(EditDefaultsOnly, Category = "UI")
    FText SkillName;

    UPROPERTY(EditDefaultsOnly, Category = "UI", meta = (MultiLine = "true"))
    FText ShortDescription;

    UPROPERTY(EditDefaultsOnly, Category = "UI", meta = (MultiLine = "true"))
    FText DetailedDescription;

    UPROPERTY(EditDefaultsOnly, Category = "UI", meta = (MultiLine = "true"))
    TObjectPtr<UTexture2D> SKillIcon;
};
