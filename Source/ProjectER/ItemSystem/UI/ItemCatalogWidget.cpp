// Fill out your copyright notice in the Description page of Project Settings.

#include "ItemSystem/UI/ItemCatalogWidget.h"
#include "ItemSystem/UI/ItemCatalogSlotWidget.h"
#include "ItemSystem/Data/BaseItemData.h"
#include "ItemSystem/Data/Usableitemdata.h"
#include "Components/UniformGridPanel.h"
#include "Components/UniformGridSlot.h"
#include "Components/Button.h"
#include "Components/TextBlock.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "UObject/UObjectIterator.h"

void UItemCatalogWidget::NativeConstruct()
{
	Super::NativeConstruct();

	if (Btn_FilterAll)
		Btn_FilterAll->OnClicked.AddDynamic(this, &UItemCatalogWidget::OnClickFilterAll);
	if (Btn_FilterConsumable)
		Btn_FilterConsumable->OnClicked.AddDynamic(this, &UItemCatalogWidget::OnClickFilterConsumable);
	if (Btn_FilterRecovery)
		Btn_FilterRecovery->OnClicked.AddDynamic(this, &UItemCatalogWidget::OnClickFilterRecovery);
	if (Btn_FilterMaterial)
		Btn_FilterMaterial->OnClicked.AddDynamic(this, &UItemCatalogWidget::OnClickFilterMaterial);

	if (Btn_PrevPage)
		Btn_PrevPage->OnClicked.AddDynamic(this, &UItemCatalogWidget::OnClickPrevPage);
	if (Btn_NextPage)
		Btn_NextPage->OnClicked.AddDynamic(this, &UItemCatalogWidget::OnClickNextPage);

	FilterItems(ECatalogFilter::All);
}

void UItemCatalogWidget::FilterItems(ECatalogFilter FilterType)
{
	CurrentFilter = FilterType;
	CurrentPage = 0;
	LoadAllItems(CurrentFilter);
}

void UItemCatalogWidget::OnClickPrevPage()
{
	if (CurrentPage > 0)
	{
		CurrentPage--;
		LoadAllItems(CurrentFilter);
	}
}

void UItemCatalogWidget::OnClickNextPage()
{
	if (CurrentPage < MaxPage)
	{
		CurrentPage++;
		LoadAllItems(CurrentFilter);
	}
}

void UItemCatalogWidget::OnClickFilterAll() { FilterItems(ECatalogFilter::All); }
void UItemCatalogWidget::OnClickFilterConsumable() { FilterItems(ECatalogFilter::Consumable); }
void UItemCatalogWidget::OnClickFilterRecovery() { FilterItems(ECatalogFilter::Recovery); }
void UItemCatalogWidget::OnClickFilterMaterial() { FilterItems(ECatalogFilter::Material); }

void UItemCatalogWidget::LoadAllItems(ECatalogFilter FilterType)
{
	if (!ItemGridContainer || !SlotWidgetClass)
	{
		return;
	}

	ItemGridContainer->ClearChildren();

	FAssetRegistryModule& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry");
	TArray<FAssetData> AssetDataList;
	
	FARFilter Filter;
	Filter.ClassPaths.Add(UBaseItemData::StaticClass()->GetClassPathName());
	Filter.bRecursiveClasses = true;

	AssetRegistryModule.Get().GetAssets(Filter, AssetDataList);

	// 카테고리 필터링 적용
	TArray<UBaseItemData*> FilteredItems;
	for (const FAssetData& AssetData : AssetDataList)
	{
		UBaseItemData* ItemData = Cast<UBaseItemData>(AssetData.GetAsset());
		if (!ItemData) continue;

		bool bPassFilter = false;
		switch (FilterType)
		{
		case ECatalogFilter::All:
			bPassFilter = true;
			break;
		case ECatalogFilter::Consumable:
			if (ItemData->ItemCategory == EItemCategory::Consumable)
				bPassFilter = true;
			break;
		case ECatalogFilter::Recovery:
			// 회복 아이템은 소비 아이템 중에서도 Heal/Mana 효과가 있는 것
			if (ItemData->ItemCategory == EItemCategory::Consumable)
			{
				UUsableItemData* UsableData = Cast<UUsableItemData>(ItemData);
				if (UsableData && (UsableData->EffectType == EItemEffectType::HealOverTime || UsableData->EffectType == EItemEffectType::ManaOverTime))
				{
					bPassFilter = true;
				}
			}
			break;
		case ECatalogFilter::Material:
			if (ItemData->ItemCategory == EItemCategory::Material)
				bPassFilter = true;
			break;
		}

		if (bPassFilter)
		{
			FilteredItems.Add(ItemData);
		}
	}

	int32 TotalItems = FilteredItems.Num();
	const int32 MaxItemsPerPage = 24; // 4x6 = 24
	const int32 Columns = 4;

	MaxPage = TotalItems > 0 ? (TotalItems - 1) / MaxItemsPerPage : 0;
	CurrentPage = FMath::Clamp(CurrentPage, 0, MaxPage);

	int32 StartIndex = CurrentPage * MaxItemsPerPage;

	for (int32 i = 0; i < MaxItemsPerPage; ++i)
	{
		UBaseItemData* ItemData = nullptr;
		int32 ItemIndex = StartIndex + i;

		if (ItemIndex < TotalItems)
		{
			ItemData = FilteredItems[ItemIndex];
		}

		UItemCatalogSlotWidget* SlotWidget = CreateWidget<UItemCatalogSlotWidget>(this, SlotWidgetClass);
		if (SlotWidget)
		{
			SlotWidget->InitializeSlot(ItemData);
			UUniformGridSlot* GridSlot = ItemGridContainer->AddChildToUniformGrid(SlotWidget);
			if (GridSlot)
			{
				// 가로 4칸, 세로 6칸 고정 배치
				int32 Row = i / Columns;
				int32 Col = i % Columns;
				GridSlot->SetRow(Row);
				GridSlot->SetColumn(Col);
				
				// 슬롯 자체 내부 정렬
				GridSlot->SetHorizontalAlignment(HAlign_Center);
				GridSlot->SetVerticalAlignment(VAlign_Center);
			}
		}
	}
	
	// 전체 그리드 슬롯의 간격 설정
	ItemGridContainer->SetSlotPadding(FMargin(5.f));

	UpdatePaginationUI();
}

void UItemCatalogWidget::UpdatePaginationUI()
{
	if (Btn_PrevPage)
	{
		Btn_PrevPage->SetIsEnabled(CurrentPage > 0);
	}
	if (Btn_NextPage)
	{
		Btn_NextPage->SetIsEnabled(CurrentPage < MaxPage);
	}
	if (Text_PageInfo)
	{
		FString PageString = FString::Printf(TEXT("%d / %d"), CurrentPage + 1, MaxPage + 1);
		Text_PageInfo->SetText(FText::FromString(PageString));
	}
}
