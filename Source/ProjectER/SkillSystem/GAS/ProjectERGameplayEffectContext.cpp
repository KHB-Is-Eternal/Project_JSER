// Fill out your copyright notice in the Description page of Project Settings.

#include "SkillSystem/GAS/ProjectERGameplayEffectContext.h"
#include "AbilitySystemGlobals.h"

bool FProjectERGameplayEffectContext::NetSerialize(FArchive& Ar, class UPackageMap* Map, bool& bOutSuccess)
{
	// 1. FGameplayEffectContext(부모)의 기존 변수들 직렬화
	bool bOutSuccessLocal = true;
	Super::NetSerialize(Ar, Map, bOutSuccessLocal);
	bOutSuccess &= bOutSuccessLocal;

	// 2. ClientActivationTime 직렬화 (클라이언트 -> 서버 전송)
	Ar << ClientActivationTime;

	// 3. PredictionKey 직렬화 (GCN 핸드셰이크용)
	PredictionKey.NetSerialize(Ar, Map, bOutSuccess);

	return bOutSuccess;
}
