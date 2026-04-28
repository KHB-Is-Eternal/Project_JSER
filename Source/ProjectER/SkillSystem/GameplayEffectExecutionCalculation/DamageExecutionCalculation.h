// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameplayEffectExecutionCalculation.h"
#include "DamageExecutionCalculation.generated.h"

/**
 * 
 */
class UBaseAttributeSet;
UCLASS(Abstract)
class PROJECTER_API UDamageExecutionCalculation : public UGameplayEffectExecutionCalculation
{
	GENERATED_BODY()
	

public:
	UDamageExecutionCalculation();
	virtual void Execute_Implementation(const FGameplayEffectCustomExecutionParameters& ExecutionParams, FGameplayEffectCustomExecutionOutput& OutExecutionOutput) const override;

protected:
    /** 자식 클래스에서 오버라이드하여 데미지 계산식을 구현합니다. */
    virtual float CalculateFinalDamage(float RawDamage, float Defense, const FGameplayEffectSpec& Spec) const { return RawDamage; }

	float FindValueByAttribute(const FGameplayEffectCustomExecutionParameters& ExecutionParams, const FGameplayAttribute& Attribute, const TMap<FGameplayAttribute, FGameplayEffectAttributeCaptureDefinition>& TargetMap) const;

private:
};
