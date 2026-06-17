// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameplayCueNotify_Static.h"
#include "GCN_DamageText.generated.h"

class UNiagaraSystem;

/**
 * 데미지가 발생했을 때 플로팅 텍스트(Niagara)를 소환하는 GameplayCueNotify_Static 클래스입니다.
 */
UCLASS()
class PROJECTER_API UGCN_DamageText : public UGameplayCueNotify_Static
{
	GENERATED_BODY()

public:
	UGCN_DamageText();

	virtual bool OnExecute_Implementation(AActor* MyTarget, const FGameplayCueParameters& Parameters) const override;

protected:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Niagara")
	TSoftObjectPtr<UNiagaraSystem> DamageTextNiagaraSystem;
};
