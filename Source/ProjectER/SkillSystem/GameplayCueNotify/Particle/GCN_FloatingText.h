// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameplayCueNotify_Static.h"
#include "GCN_FloatingText.generated.h"

class UNiagaraSystem;

/**
 * 데미지 및 회복 발생 시 플로팅 텍스트(Niagara)를 소환하는 GameplayCueNotify_Static 클래스입니다.
 */
UCLASS()
class PROJECTER_API UGCN_FloatingText : public UGameplayCueNotify_Static
{
	GENERATED_BODY()

public:
	UGCN_FloatingText();

	virtual bool OnExecute_Implementation(AActor* MyTarget, const FGameplayCueParameters& Parameters) const override;

protected:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Niagara")
	TSoftObjectPtr<UNiagaraSystem> FloatingTextNiagaraSystem;
};
