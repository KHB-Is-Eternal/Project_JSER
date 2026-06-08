// Fill out your copyright notice in the Description page of Project Settings.

#include "ItemSystem/UI/ItemCatalogWidget.h"
#include "ItemSystem/UI/ItemCatalogSlotWidget.h"
#include "ItemSystem/Data/BaseItemData.h"
#include "Components/UniformGridPanel.h"
#include "Components/UniformGridSlot.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "UObject/UObjectIterator.h"

void UItemCatalogWidget::NativeConstruct()
{
	Super::NativeConstruct();

	LoadAllItems();
}

void UItemCatalogWidget::LoadAllItems()
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

	int32 ItemCount = 0;
	const int32 MaxItemsPerPage = 24; // 4x6 = 24
	const int32 Columns = 4;

	for (int32 i = 0; i < MaxItemsPerPage; ++i)
	{
		UBaseItemData* ItemData = nullptr;
		if (i < AssetDataList.Num())
		{
			ItemData = Cast<UBaseItemData>(AssetDataList[i].GetAsset());
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
