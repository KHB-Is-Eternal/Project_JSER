// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameplayEffect.h"
#include "BaseGameplayEffect.generated.h"

/**
 * 
 */
UCLASS()
class PROJECTER_API UBaseGameplayEffect : public UGameplayEffect
{
	GENERATED_BODY()

public:
	UBaseGameplayEffect();
#if WITH_EDITOR
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
#endif

public:
	/** 자식 클래스로서 부모의 protected 멤버인 GEComponents에 접근할 수 있게 해줍니다. */
	const TArray<TObjectPtr<UGameplayEffectComponent>>& GetGEComponents() const { return GEComponents; }
protected:

private:

public:
	/*UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "GameplayEffect|GameplayModifier", meta = (FilterMetaTag = "HideFromModifiers"))
	FGameplayAttribute SourceAttribute;	*/

protected:

private:
	
};
