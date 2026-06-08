#include "ItemSystem/UI/ItemNameWidget.h"
#include "Components/TextBlock.h"

void UItemNameWidget::SetItemName(FText NewName)
{
	if (Txt_ItemName)
	{
		Txt_ItemName->SetText(NewName);
	}
}
