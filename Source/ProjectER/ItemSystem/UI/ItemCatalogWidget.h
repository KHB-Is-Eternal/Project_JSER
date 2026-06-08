// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "ItemCatalogWidget.generated.h"

class UUniformGridPanel;
class UItemCatalogSlotWidget;
class UBaseItemData;

UCLASS()
class PROJECTER_API UItemCatalogWidget : public UUserWidget
{
	GENERATED_BODY()
	
protected:
	virtual void NativeConstruct() override;

	// 아이템 아이콘들이 배치될 그리드 패널 (4x6 배치용)
	UPROPERTY(meta = (BindWidget))
	UUniformGridPanel* ItemGridContainer;

	// 동적으로 생성할 개별 슬롯 위젯 클래스
	UPROPERTY(EditAnywhere, Category = "Catalog")
	TSubclassOf<UItemCatalogSlotWidget> SlotWidgetClass;

private:
	// 모든 아이템 데이터를 불러와 컨테이너에 추가하는 함수
	void LoadAllItems();
};
