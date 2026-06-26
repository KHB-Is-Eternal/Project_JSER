// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "UI/UI_ToolTip.h"
#include "Components/Widget.h"
#include "UI_ToolTipManager.generated.h"

UCLASS() // 언리얼 시스템에 등록
class PROJECTER_API UUI_ToolTipManager : public UObject
{
    GENERATED_BODY() // 언리얼 코드 생성 매크로

public:
    UUI_ToolTipManager();


    void setTooltipInstance(UUI_ToolTip* InTooltipInstance) { TooltipInstance = InTooltipInstance; }

    // [김현수 추가분] 매개변수 끝에 이름 폰트 색상(NameColor) 추가 (기본값 설정으로 기존 코드 안정성 확보)
    void ShowTooltip(UWidget* AnchorWidget,FText Name, FText ShortDesc, FText DetailDesc, FText CostDesc, bool showUpper, FLinearColor NameColor = FLinearColor(0.0f, 0.0f, 0.0f, 1.0f));

private:
    UPROPERTY()
    UUI_ToolTip* TooltipInstance;
};