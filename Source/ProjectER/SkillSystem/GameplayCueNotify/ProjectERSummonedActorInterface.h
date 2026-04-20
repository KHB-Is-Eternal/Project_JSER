// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "ProjectERSummonedActorInterface.generated.h"

class UNiagaraComponent;
class UProjectileMovementComponent;
class USkillNiagaraSpawnConfig;

/**
 * 소환된 비주얼 액터(GCN)와 판정 액터 사이의 지연 바인딩(Late Binding)을 지원하기 위한 인터페이스
 */
UINTERFACE(MinimalAPI)
class UProjectERSummonedActorInterface : public UInterface
{
	GENERATED_BODY()
};

class PROJECTER_API IProjectERSummonedActorInterface
{
	GENERATED_BODY()

public:
	/** 
	 * 비주얼 액터(VFX)가 판정 액터에 핸드셰이크되어 부착되었을 때 호출됩니다.
	 */
	UFUNCTION(BlueprintNativeEvent, Category = "ProjectER|Summoned")
	void OnVfxHandshakeCompleted(AActor* VfxActor);

	/** GCN 액터(AGCN_SummonedActor) 초기화를 위한 니아가라 설정 데이터 제공 */
	virtual class USkillNiagaraSpawnConfig* GetAGCN_NiagaraConfig() const { return nullptr; }
};
