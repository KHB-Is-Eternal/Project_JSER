// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimNotifies/AnimNotify.h"
#include "GameplayCueInterface.h"
#include "AnimNotify_SkillSoundGameplayCue.generated.h"

class USkillSoundSpawnConfig;

/**
 * USkillSoundSpawnConfig를 지원하며 엔진의 기본 로직을 미러링한 사운드 관련 GameplayCue AnimNotify입니다.
 */
UCLASS(editinlinenew, meta=(DisplayName = "Skill Sound GameplayCue (Burst)"))
class PROJECTER_API UAnimNotify_SkillSoundGameplayCue : public UAnimNotify
{
	GENERATED_BODY()

public:
	UAnimNotify_SkillSoundGameplayCue();

	// UAnimNotify 인터페이스
	virtual void Notify(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, const FAnimNotifyEventReference& EventReference) override;

	virtual FString GetNotifyName_Implementation() const override;

#if WITH_EDITOR
	virtual bool CanBePlaced(UAnimSequenceBase* Animation) const override;
#endif

protected:
	/** 사운드 효과 설정을 담은 데이터 에셋입니다. 내부에 GameplayCue 태그 정보를 포함하고 있습니다. */
	UPROPERTY(EditAnywhere, Category = "Skill", meta = (DisplayName = "Spawn Config"))
	TObjectPtr<USkillSoundSpawnConfig> SpawnConfig;
};
