#include "UI/UI_CharacterSelectSlot.h"
#include "UI/UI_CharacterGridWidget.h"
#include "CharacterSystem/Data/CharacterData.h"
#include "GameModeBase/State/ER_PlayerState.h"
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

	if (APlayerController* PC = GetOwningPlayer())
	{
		if (AER_PlayerState* ERPS = PC->GetPlayerState<AER_PlayerState>())
		{
			bIsReadyLocal = ERPS->bIsReady;
			ERPS->OnReadyStateChanged.AddDynamic(this, &UUI_CharacterSelectSlot::OnReadyStateChanged);
		}
	}
}

void UUI_CharacterSelectSlot::InitSlot(int32 InSlotIndex, UCharacterData* InCharacterData, UUI_CharacterGridWidget* InGridWidget)
{
	SlotIndex = InSlotIndex;
	CharacterData = InCharacterData;
	GridWidget = InGridWidget;

	if (CharacterData && IconImage)
	{
		if (CharacterData->CharacterIcon)
		{
			IconImage->SetBrushFromTexture(CharacterData->CharacterIcon);
		}
	}
	SetHighlight(false);

	// 초기화 시점에도 PlayerState 바인딩 시도
	if (APlayerController* PC = GetOwningPlayer())
	{
		if (AER_PlayerState* ERPS = PC->GetPlayerState<AER_PlayerState>())
		{
			bIsReadyLocal = ERPS->bIsReady;
			if (!ERPS->OnReadyStateChanged.IsAlreadyBound(this, &UUI_CharacterSelectSlot::OnReadyStateChanged))
			{
				ERPS->OnReadyStateChanged.AddDynamic(this, &UUI_CharacterSelectSlot::OnReadyStateChanged);
			}
		}
	}
}

void UUI_CharacterSelectSlot::OnReadyStateChanged(bool bNewReadyState)
{
	bIsReadyLocal = bNewReadyState;
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
	// 1. 캐싱된 레디 상태 플래그 검사
	if (bIsReadyLocal)
	{
		return;
	}

	// 2. PlayerState 실시간 상태 이중 검사
	APlayerController* PC = GetOwningPlayer();
	AER_PlayerState* ERPS = PC ? PC->GetPlayerState<AER_PlayerState>() : nullptr;
	if (ERPS && (ERPS->bIsReady || ERPS->GetIsReady()))
	{
		bIsReadyLocal = true;
		return;
	}

	if (GridWidget)
	{
		GridWidget->OnSlotSelected(SlotIndex);
	}
}
