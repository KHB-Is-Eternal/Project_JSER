// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimNotifies/AnimNotify.h"
#include "GameplayTagContainer.h"
#include "AnimNotify_AddTagSkillBackswing.generated.h"

/**
 * 스킬의 후딜레이(Backswing) 단계 진입을 알리는 노티파이입니다.
 * 이 노티파이가 실행되면 캐릭터는 이동이나 다른 스킬 사용이 가능한 상태가 됩니다.
 */
UCLASS()
class PROJECTER_API UAnimNotify_AddTagSkillBackswing : public UAnimNotify
{
	GENERATED_BODY()
public:
	UAnimNotify_AddTagSkillBackswing();

	virtual void Notify(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, const FAnimNotifyEventReference& EventReference) override;

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GameplayAbility")
	float EventMagnitude = 1.0f;

private:
	UPROPERTY(VisibleAnywhere, Category = "Tag")
	FGameplayTag BackswingTag;
};
