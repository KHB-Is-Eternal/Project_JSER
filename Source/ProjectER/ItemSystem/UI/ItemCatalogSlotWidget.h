// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "ItemCatalogSlotWidget.generated.h"

class UBaseItemData;
class UImage;
class UButton;
class UUI_ToolTipManager;
class UUI_ToolTip;

UCLASS()
class PROJECTER_API UItemCatalogSlotWidget : public UUserWidget
{
	GENERATED_BODY()
	
public:
	// 초기화 시 아이템 데이터 할당
	UFUNCTION(BlueprintCallable, Category = "Item Catalog")
	void InitializeSlot(UBaseItemData* InItemData);

protected:
	virtual void NativeConstruct() override;

	UPROPERTY(meta = (BindWidget))
	UImage* ItemIcon;

	UPROPERTY(meta = (BindWidget))
	UButton* ItemButton;

	// [김현수 추가분] 아이콘 뒤 레어도 솔리드 배경 (팝업처럼 배경을 레어도색으로 채움)
	UPROPERTY(meta = (BindWidgetOptional))
	UImage* RarityBG;

	// [김현수 추가분] 최상단 테두리 프레임 (중앙 투명). 채움이 테두리를 침범하지 않도록 위에서 덮는다.
	UPROPERTY(meta = (BindWidgetOptional))
	UImage* SlotFrame;

	// 툴팁 클래스 (블루프린트에서 할당)
	UPROPERTY(EditAnywhere, Category = "Tooltip")
	TSubclassOf<UUI_ToolTip> TooltipClass;

private:
	UPROPERTY()
	UBaseItemData* CachedItemData;

	UPROPERTY()
	UUI_ToolTipManager* TooltipManager;

	UPROPERTY()
	UUI_ToolTip* TooltipInstance;

	UFUNCTION()
	void OnItemHovered();

	UFUNCTION()
	void OnItemUnhovered();
};
