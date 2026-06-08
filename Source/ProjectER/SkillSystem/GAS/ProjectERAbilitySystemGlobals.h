// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "AbilitySystemGlobals.h"
#include "ProjectERAbilitySystemGlobals.generated.h"

/**
 * ProjectER 전용 Ability System Globals
 * 모든 MakeEffectContext() 호출 시 엔진 기본 Context 대신
 * FProjectERGameplayEffectContext를 반환하도록 강제합니다.
 */
UCLASS()
class PROJECTER_API UProjectERAbilitySystemGlobals : public UAbilitySystemGlobals
{
	GENERATED_BODY()
	
public:
	/** 새로운 게임플레이 이펙트 컨텍스트 구조체를 할당합니다. */
	virtual FGameplayEffectContext* AllocGameplayEffectContext() const override;
};
