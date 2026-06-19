// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "ItemCatalogWidget.generated.h"

class UUniformGridPanel;
class UItemCatalogSlotWidget;
class UBaseItemData;
class UButton;
class UTextBlock;

UENUM(BlueprintType)
enum class ECatalogFilter : uint8
{
	All            UMETA(DisplayName = "전체 아이템"),
	Consumable     UMETA(DisplayName = "소비 아이템"),
	Recovery       UMETA(DisplayName = "회복 아이템"),
	Material       UMETA(DisplayName = "재료 아이템")
};

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

	// === Filter Buttons ===
	UPROPERTY(meta = (BindWidgetOptional))
	UButton* Btn_FilterAll;

	UPROPERTY(meta = (BindWidgetOptional))
	UButton* Btn_FilterConsumable;

	UPROPERTY(meta = (BindWidgetOptional))
	UButton* Btn_FilterRecovery;

	UPROPERTY(meta = (BindWidgetOptional))
	UButton* Btn_FilterMaterial;

	// === Pagination UI ===
	UPROPERTY(meta = (BindWidgetOptional))
	UButton* Btn_PrevPage;

	UPROPERTY(meta = (BindWidgetOptional))
	UButton* Btn_NextPage;

	UPROPERTY(meta = (BindWidgetOptional))
	UTextBlock* Text_PageInfo;

public:
	// 필터링 적용 함수
	UFUNCTION(BlueprintCallable, Category = "Catalog")
	void FilterItems(ECatalogFilter FilterType);

private:
	// 버튼 클릭 이벤트 바인딩
	UFUNCTION() void OnClickFilterAll();
	UFUNCTION() void OnClickFilterConsumable();
	UFUNCTION() void OnClickFilterRecovery();
	UFUNCTION() void OnClickFilterMaterial();

	// 페이지 이동 버튼 이벤트 바인딩
	UFUNCTION() void OnClickPrevPage();
	UFUNCTION() void OnClickNextPage();

	// 모든 아이템 데이터를 불러와 컨테이너에 추가하는 함수
	void LoadAllItems(ECatalogFilter FilterType = ECatalogFilter::All);

	// 페이지 버튼 상태 및 텍스트 갱신
	void UpdatePaginationUI();

	// 페이징 상태 변수
	int32 CurrentPage = 0;
	int32 MaxPage = 0;
	ECatalogFilter CurrentFilter = ECatalogFilter::All;
};
