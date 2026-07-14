// Fill out your copyright notice in the Description page of Project Settings.


#include "UI/UI_ToolTip.h"
#include "Components/TextBlock.h"
#include "Components/Image.h"

// [김현수 추가분] 매개변수 끝에 이름 폰트 색상(NameColor) 추가 및 색상 적용
void UUI_ToolTip::UpdateTooltip(FText Name, FText ShortDesc, FText DetailDesc, FText CostDesc, FLinearColor NameColor)
{
    if (IsValid(txtName)) 
    {
        txtName->SetText(Name);
        txtName->SetColorAndOpacity(FSlateColor(NameColor)); // [김현수 추가분] 폰트 색상 적용
    }
    if (IsValid(txtShortDesc)) txtShortDesc->SetText(ShortDesc);
    if (IsValid(txtLongDesc)) txtLongDesc->SetText(DetailDesc);
	if (IsValid(txtCostDesc)) txtCostDesc->SetText(CostDesc);

    // 레이아웃을 재계산
    ForceLayoutPrepass();
}
