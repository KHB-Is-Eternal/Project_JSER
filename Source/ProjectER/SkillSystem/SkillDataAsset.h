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

USTRUCT(BlueprintType)
struct FSkillIndicatorConfig
{
    GENERATED_BODY()

    /** 조준 시 띄울 인디케이터 블루프린트 (소프트 레퍼런스) */
    UPROPERTY(EditDefaultsOnly, Category = "Indicator")
    TSoftClassPtr<class ASkillIndicatorActor> IndicatorClass;

    /** 
     * 조준선의 절대 크기 (FVector를 통해 길이, 폭, 높이를 개별적으로 통제) 
     * 블루프린트 원본 크기와 무관하게, 데칼 컴포넌트의 실제 투영 크기(DecalSize)를 이 수치로 정확하게 고정(Set)시킵니다.
     */
    UPROPERTY(EditDefaultsOnly, Category = "Indicator")
    FVector IndicatorSize = FVector(100.f, 100.f, 100.f);

    /** 조준선 생성 시의 로컬 위치 오프셋 (X: 전방, Y: 우측, Z: 상방) */
    UPROPERTY(EditDefaultsOnly, Category = "Indicator")
    FVector LocationOffset = FVector::ZeroVector;

    /** 아티스트가 제작한 텍스처 방향 오정렬을 보정하기 위한 회전 오프셋 */
    UPROPERTY(EditDefaultsOnly, Category = "Indicator")
    FRotator RotationOffset = FRotator::ZeroRotator;
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

    const FSkillIndicatorConfig& GetIndicatorConfig() const { return IndicatorConfig; }

public:
    UPROPERTY(EditDefaultsOnly, Category = "Indicator")
    FSkillIndicatorConfig IndicatorConfig;

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
