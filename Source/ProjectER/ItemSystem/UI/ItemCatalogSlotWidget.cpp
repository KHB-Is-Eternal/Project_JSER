// Fill out your copyright notice in the Description page of Project Settings.

#include "ItemSystem/UI/ItemCatalogSlotWidget.h"
#include "ItemSystem/Data/BaseItemData.h"
#include "Components/Image.h"
#include "Components/Button.h"
#include "UI/UI_ToolTip.h"
#include "UI/UI_ToolTipManager.h"
#include "UObject/ConstructorHelpers.h"

void UItemCatalogSlotWidget::NativeConstruct()
{
	Super::NativeConstruct();

	if (ItemButton)
	{
		ItemButton->OnHovered.AddDynamic(this, &UItemCatalogSlotWidget::OnItemHovered);
		ItemButton->OnUnhovered.AddDynamic(this, &UItemCatalogSlotWidget::OnItemUnhovered);
	}
}

void UItemCatalogSlotWidget::InitializeSlot(UBaseItemData* InItemData)
{
	CachedItemData = InItemData;

	if (CachedItemData && ItemIcon)
	{
		// Soft reference load for icon
		UTexture2D* LoadedIcon = CachedItemData->ItemIcon.LoadSynchronous();
		if (LoadedIcon)
		{
			ItemIcon->SetBrushFromTexture(LoadedIcon);
			ItemIcon->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
		}
		
		if (ItemButton)
		{
			ItemButton->SetIsEnabled(true);
			ItemButton->SetBackgroundColor(CachedItemData->GetRarityColor());
		}
	}
	else
	{
		// 아이템 데이터가 없는 빈 슬롯의 경우
		if (ItemIcon)
		{
			ItemIcon->SetVisibility(ESlateVisibility::Collapsed); // 이미지는 숨김 (버튼 바탕만 남음)
		}
		if (ItemButton)
		{
			ItemButton->SetIsEnabled(false); // 빈 슬롯은 클릭 불가
			ItemButton->SetBackgroundColor(FLinearColor(0.15f, 0.15f, 0.15f, 1.0f)); // 기본 바탕색
		}
	}
}

void UItemCatalogSlotWidget::OnItemHovered()
{
	if (!CachedItemData) return;

	// Create tooltip instance if not exists
	if (!TooltipInstance && TooltipClass)
	{
		TooltipInstance = Cast<UUI_ToolTip>(CreateWidget<UUserWidget>(GetWorld(), TooltipClass));
		if (TooltipInstance)
		{
			TooltipInstance->SetVisibility(ESlateVisibility::Collapsed);
			TooltipInstance->AddToViewport(100); // UI priority
		}
	}

	// Create manager if not exists
	if (!TooltipManager && TooltipInstance)
	{
		TooltipManager = NewObject<UUI_ToolTipManager>(this);
		TooltipManager->setTooltipInstance(TooltipInstance);
	}

	if (TooltipManager && TooltipInstance)
	{
		TooltipInstance->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
		TooltipManager->ShowTooltip(
			ItemButton,
			CachedItemData->ItemName,
			CachedItemData->ItemShortDesc,
			CachedItemData->ItemLongDesc,
			FText::GetEmpty(),
			true
		);
	}
}

void UItemCatalogSlotWidget::OnItemUnhovered()
{
	if (IsValid(TooltipInstance))
	{
		TooltipInstance->SetVisibility(ESlateVisibility::Collapsed);
	}
}
