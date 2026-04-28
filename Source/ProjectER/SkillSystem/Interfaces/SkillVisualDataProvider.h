// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "SkillVisualDataProvider.generated.h"

class USkillNiagaraSpawnConfig;

UINTERFACE(MinimalAPI)
class USkillVisualDataProvider : public UInterface
{
	GENERATED_BODY()
};

/**
 * GCN 액터(비주얼)에게 데이터를 제공하기 위한 인터페이스
 */
class PROJECTER_API ISkillVisualDataProvider
{
	GENERATED_BODY()

public:
	/** 비주얼(GCN) 액터 초기화를 위한 니아가라 설정 데이터 제공 */
	virtual class USkillNiagaraSpawnConfig* GetAGCN_NiagaraConfig() const { return nullptr; }
};
