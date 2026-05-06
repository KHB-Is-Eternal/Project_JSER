// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimNotifies/AnimNotifyState.h"
#include "GameplayCueInterface.h"
#include "GameplayCueManager.h"
#include "AnimNotifyState_SkillSoundGameplayCue.generated.h"

class USkillSoundSpawnConfig;

/**
 * USkillSoundSpawnConfig를 지원하며 엔진의 기본 로직을 미러링한 사운드 관련 GameplayCue AnimNotifyState입니다.
 */
UCLASS(editinlinenew, meta=(DisplayName = "Skill Sound GameplayCue (Looping)"))
class PROJECTER_API UAnimNotifyState_SkillSoundGameplayCue : public UAnimNotifyState
{
	GENERATED_BODY()

public:
	UAnimNotifyState_SkillSoundGameplayCue();

	// UAnimNotifyState 인터페이스
	virtual void NotifyBegin(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, float TotalDuration, const FAnimNotifyEventReference& EventReference) override;
	virtual void NotifyTick(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, float FrameDeltaTime, const FAnimNotifyEventReference& EventReference) override;
	virtual void NotifyEnd(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, const FAnimNotifyEventReference& EventReference) override;

	virtual FString GetNotifyName_Implementation() const override;

#if WITH_EDITOR
	virtual bool CanBePlaced(UAnimSequenceBase* Animation) const override;
#endif

protected:
	/** 사운드 효과 설정을 담은 데이터 에셋입니다. 내부에 GameplayCue 태그 정보를 포함하고 있습니다. */
	UPROPERTY(EditAnywhere, Category = "Skill", meta = (DisplayName = "Spawn Config"))
	TObjectPtr<USkillSoundSpawnConfig> SpawnConfig;

#if WITH_EDITORONLY_DATA
	FGameplayCueProxyTick PreviewProxyTick;
#endif
};
