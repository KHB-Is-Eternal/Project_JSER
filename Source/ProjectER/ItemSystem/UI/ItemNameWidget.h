#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "ItemNameWidget.generated.h"

class UTextBlock;

/**
 * 바닥 아이템의 이름을 표시하기 위한 위젯의 C++ 베이스 클래스입니다.
 * 블루프린트 위젯에서 부모 클래스를 이 클래스로 설정하면 C++에서 직접 데이터를 제어할 수 있습니다.
 */
UCLASS()
class PROJECTER_API UItemNameWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	// 블루프린트 위젯 내부에 'Txt_ItemName'이라는 이름의 TextBlock이 있어야 자동 바인딩됩니다.
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> Txt_ItemName;

	/** 아이템 이름을 텍스트 블록에 설정합니다. */
	UFUNCTION(BlueprintCallable, Category = "Item|UI")
	void SetItemName(FText NewName);
};
