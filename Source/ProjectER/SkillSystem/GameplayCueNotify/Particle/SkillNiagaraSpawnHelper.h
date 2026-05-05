// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"

/**
 * 
 */
class UNiagaraComponent;
class USkillNiagaraSpawnConfig;
struct FSkillNiagaraSpawnSettings;

namespace SkillNiagaraSpawnHelper
{
	/**
	 * Settings에 정의된 설정대로 나이아가라 시스템을 생성합니다.
	 * @return 생성된 나이아가라 컴포넌트
	 */
	UNiagaraComponent* SpawnNiagaraBySettings(UWorld* World, const FSkillNiagaraSpawnSettings& Settings, const FTransform& SourceTransform, const AActor* SourceActor = nullptr, const FVector* OptionalLookAtTarget = nullptr, USceneComponent* AttachTarget = nullptr);

	/**
	 * Config 객체에 정의된 설정대로 나이아가라 시스템을 생성합니다.
	 */
	UNiagaraComponent* SpawnNiagara(UWorld* World, const USkillNiagaraSpawnConfig* Config, const FTransform& SourceTransform, const AActor* SourceActor = nullptr, const FVector* OptionalLookAtTarget = nullptr, USceneComponent* AttachTarget = nullptr);

	/**
	 * 이미 생성된 컴포넌트를 새로운 부모에게 설정을 유지하며 부착합니다. (핸드셰이크용)
	 */
	void AttachNiagaraByConfig(UNiagaraComponent* Component, USceneComponent* NewParent, const USkillNiagaraSpawnConfig* Config);
}
