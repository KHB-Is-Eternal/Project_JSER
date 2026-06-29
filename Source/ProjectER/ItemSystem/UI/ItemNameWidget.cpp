#include "ItemSystem/UI/ItemNameWidget.h"
#include "Components/TextBlock.h"

void UItemNameWidget::SetItemName(FText NewName, FLinearColor NameColor)
{
	if (Txt_ItemName)
	{
		Txt_ItemName->SetText(NewName);
		Txt_ItemName->SetColorAndOpacity(FSlateColor(NameColor)); // [김현수 추가분] 색상 적용
	}
}
