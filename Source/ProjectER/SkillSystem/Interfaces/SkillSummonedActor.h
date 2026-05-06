// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "SkillSummonedActor.generated.h"

UINTERFACE(MinimalAPI)
class USkillSummonedActor : public UInterface
{
	GENERATED_BODY()
};

/**
 * 스킬 등의 로직으로 소환된 액터 들이 상속받는 인터페이스
 */
class PROJECTER_API ISkillSummonedActor
{
	GENERATED_BODY()

public:
	/** 
	 * 비주얼 액터(VFX)가 판정 액터에 핸드셰이크되어 부착되었을 때 호출됩니다.
	 */
	UFUNCTION(BlueprintNativeEvent, Category = "ProjectER|Summoned")
	void OnVfxHandshakeCompleted(AActor* VfxActor);
};
