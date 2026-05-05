// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
 
class UWorld;
class AActor;
class USceneComponent;

class UAudioComponent;
class USkillSoundSpawnConfig;
struct FSkillSoundSpawnSettings;

namespace SkillSoundSpawnHelper
{
	/**
	 * Settings에 정의된 설정대로 사운드를 재생합니다.
	 * @return 생성된 오디오 컴포넌트 (AtLocation 시에도 반환 가능)
	 */
	UAudioComponent* PlaySoundBySettings(UWorld* World, const FSkillSoundSpawnSettings& Settings, const FTransform& SourceTransform, const AActor* SourceActor = nullptr, const FVector* OptionalLookAtTarget = nullptr, USceneComponent* AttachTarget = nullptr);

	/**
	 * Config 객체에 정의된 설정대로 사운드를 재생합니다.
	 */
	UAudioComponent* SpawnSound(UWorld* World, const USkillSoundSpawnConfig* Config, const FTransform& SourceTransform, const AActor* SourceActor = nullptr, const FVector* OptionalLookAtTarget = nullptr, USceneComponent* AttachTarget = nullptr);

	/**
	 * 이미 생성된 컴포넌트를 새로운 부모에게 설정을 유지하며 부착합니다. (핸드셰이크용)
	 */
	void AttachSoundByConfig(UAudioComponent* Component, USceneComponent* NewParent, const USkillSoundSpawnConfig* Config);
}
