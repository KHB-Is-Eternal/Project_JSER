// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameplayModMagnitudeCalculation.h"
#include "MMC_TenacityDuration.generated.h"

/**
 * 타겟의 Tenacity(강인함) 수치에 따라 CC 지속 시간을 보정하는 MMC 클래스입니다.
 * 에디터의 'Coefficient' 칸에 기본 지속 시간을 적으면 그에 비례하여 시간을 깎아줍니다.
 */
UCLASS()
class PROJECTER_API UMMC_TenacityDuration : public UGameplayModMagnitudeCalculation
{
	GENERATED_BODY()

public:
    UMMC_TenacityDuration();

    virtual float CalculateBaseMagnitude_Implementation(const FGameplayEffectSpec& Spec) const override;

protected:
    /** 최소 보장 비율 (강인함이 아무리 높아도 원래 시간의 이 비율 이하로는 줄어들지 않음. 예: 0.2 = 20%) */
    UPROPERTY(EditDefaultsOnly, Category = "Tenacity")
    float MinDurationPercent = 0.2f;

private:
    FGameplayEffectAttributeCaptureDefinition TenacityDef;
};
