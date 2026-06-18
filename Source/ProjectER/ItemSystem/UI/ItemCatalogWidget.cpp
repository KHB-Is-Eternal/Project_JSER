// Fill out your copyright notice in the Description page of Project Settings.

#include "ItemSystem/UI/ItemCatalogWidget.h"
#include "ItemSystem/UI/ItemCatalogSlotWidget.h"
#include "ItemSystem/Data/BaseItemData.h"
#include "ItemSystem/Data/Usableitemdata.h"
#include "Components/UniformGridPanel.h"
#include "Components/UniformGridSlot.h"
#include "Components/Button.h"
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

	FilterItems(ECatalogFilter::All);
}

void UItemCatalogWidget::FilterItems(ECatalogFilter FilterType)
{
	LoadAllItems(FilterType);
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

	int32 ItemCount = 0;
	const int32 MaxItemsPerPage = 24; // 4x6 = 24
	const int32 Columns = 4;

	for (int32 i = 0; i < MaxItemsPerPage; ++i)
	{
		UBaseItemData* ItemData = nullptr;
		if (i < FilteredItems.Num())
		{
			ItemData = FilteredItems[i];
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
}
