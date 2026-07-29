#include "UI/UI_CharacterSelectSlot.h"
#include "UI/UI_CharacterSelectWidget.h"
#include "CharacterSystem/Data/CharacterData.h"
#include "Components/Image.h"
#include "Components/Border.h"
#include "Components/Button.h"
#include "Components/SizeBox.h"

void UUI_CharacterSelectSlot::NativeConstruct()
{
	Super::NativeConstruct();

	if (SlotButton)
	{
		SlotButton->OnClicked.AddDynamic(this, &UUI_CharacterSelectSlot::OnSlotButtonClicked);
	}
}

void UUI_CharacterSelectSlot::InitSlot(int32 InSlotIndex, UCharacterData* InCharacterData, UUI_CharacterSelectWidget* InParentWidget)
{
	SlotIndex = InSlotIndex;
	CharacterData = InCharacterData;
	ParentWidget = InParentWidget;

	if (CharacterData && IconImage)
	{
		if (CharacterData->CharacterIcon)
		{
			IconImage->SetBrushFromTexture(CharacterData->CharacterIcon);
		}
	}
	SetHighlight(false);
}

void UUI_CharacterSelectSlot::SetHighlight(bool bIsHighlighted)
{
	if (HighlightBorder)
	{
		// 투명도 조절로 활성화/비활성화 표시
		FLinearColor Color = HighlightBorder->BrushColor;
		Color.A = bIsHighlighted ? 1.0f : 0.0f;
		HighlightBorder->SetBrushColor(Color);
	}
}

void UUI_CharacterSelectSlot::SetSlotSquareSize(float InSquareSize)
{
	if (SizeBox_Root && InSquareSize > 0.0f)
	{
		SizeBox_Root->SetWidthOverride(InSquareSize);
		SizeBox_Root->SetHeightOverride(InSquareSize);
	}
}

void UUI_CharacterSelectSlot::OnSlotButtonClicked()
{
	if (ParentWidget)
	{
		ParentWidget->OnSlotSelected(SlotIndex);
	}
}
