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
