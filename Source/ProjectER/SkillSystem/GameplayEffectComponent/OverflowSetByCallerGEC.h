// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "SkillSystem/GameplayEffectComponent/BaseGEC.h"
#include "OverflowSetByCallerGEC.generated.h"

class UBaseGameplayEffect;

/**
 * 스택 한계(Stack Limit) 도달 시, 원본 GE의 SetByCaller 수치를 복사하여 
 * 오버플로우 이펙트(Overflow Effect)에 전달하는 커스텀 GEC입니다.
 * 
 * 기본 언리얼 GAS의 OverflowEffects는 SetByCaller를 복사하지 않으므로,
 * 이 컴포넌트가 CanGameplayEffectApply 시점에 스택을 검사하여 직접 오버플로우를 처리합니다.
 */
UCLASS(DisplayName="Overflow SetByCaller GEC")
class PROJECTER_API UOverflowSetByCallerGEC : public UBaseGEC
{
	GENERATED_BODY()

public:
	UOverflowSetByCallerGEC();

	// 오버플로우가 발생했을 때 적용할 이펙트
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Overflow")
	TSubclassOf<class UBaseGameplayEffect> OverflowEffect;


	// 오버플로우 발생 시 원본 GE의 적용을 거부(취소)할지 여부
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Overflow")
	bool bDenyOverflowApplication = false;

	virtual bool CanGameplayEffectApply(const FActiveGameplayEffectsContainer& ActiveGEContainer, const FGameplayEffectSpec& GESpec) const override;
};
