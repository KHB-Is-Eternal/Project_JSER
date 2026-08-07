// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "UI_ToolTip.generated.h"


class UImage;
class UTextBlock;
/**
 * 
 */
UCLASS(Blueprintable, BlueprintType) // 블루프린트에 넣으려면 이거 꼭 넣어야됨;
class PROJECTER_API UUI_ToolTip : public UUserWidget
{
	GENERATED_BODY()
public:
	// [김현수 추가분] 매개변수 끝에 이름 폰트 색상(NameColor) 추가 (기본값 설정으로 기존 코드 안정성 확보)
	void UpdateTooltip(FText Name, FText ShortDesc, FText DetailDesc, FText CostDesc, FLinearColor NameColor = FLinearColor(0.0f, 0.0f, 0.0f, 1.0f));

protected:
	UPROPERTY(meta = (BindWidget))
	UTextBlock* txtName;

	UPROPERTY(meta = (BindWidget))
	UTextBlock* txtShortDesc;

	UPROPERTY(meta = (BindWidget))
	UTextBlock* txtLongDesc;

	UPROPERTY(meta = (BindWidget))
	UTextBlock* txtCostDesc;
};
