#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "UI_CharacterSelectSlot.generated.h"

class UImage;
class UBorder;
class UButton;
class UCharacterData;
class UUI_CharacterGridWidget;
class USizeBox;

UCLASS()
class PROJECTER_API UUI_CharacterSelectSlot : public UUserWidget
{
	GENERATED_BODY()

public:
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;

	void InitSlot(int32 InSlotIndex, UCharacterData* InCharacterData, UUI_CharacterGridWidget* InGridWidget);

	void SetHighlight(bool bIsHighlighted);

	void SetSlotSquareSize(float InSquareSize);

	UFUNCTION()
	void OnReadyStateChanged(bool bNewReadyState);

protected:
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<USizeBox> SizeBox_Root;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UButton> SlotButton;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UImage> IconImage;

	// 하이라이트 외곽선을 위한 Border
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UBorder> HighlightBorder;

	UFUNCTION()
	void OnSlotButtonClicked();

private:
	int32 SlotIndex = -1;

	UPROPERTY(Transient)
	TObjectPtr<UCharacterData> CharacterData;

	UPROPERTY(Transient)
	TObjectPtr<UUI_CharacterGridWidget> GridWidget;

	bool bIsReadyLocal = false;
};
