#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "UI_CharacterGridWidget.generated.h"

class UUniformGridPanel;
class UCharacterData;
class UUI_CharacterSelectSlot;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnCharacterSlotSelected, int32, SlotIndex);

UCLASS()
class PROJECTER_API UUI_CharacterGridWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	virtual void NativeConstruct() override;

	void InitGrid(const TArray<TSoftObjectPtr<UCharacterData>>& InAvailableCharacters);
	void SetSelectedHighlight(int32 SelectedIndex);
	void OnSlotSelected(int32 SlotIndex);

	UFUNCTION()
	void OnPlayerStateCharacterChanged(TSoftObjectPtr<UCharacterData> NewCharacterData);

	UPROPERTY(BlueprintAssignable, Category = "ProjectER|UI")
	FOnCharacterSlotSelected OnCharacterSlotSelected;

protected:
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UUniformGridPanel> GridPanel_Characters;

	UPROPERTY(EditAnywhere, Category = "ProjectER|UI")
	TSubclassOf<UUI_CharacterSelectSlot> SlotWidgetClass;

private:
	TArray<TSoftObjectPtr<UCharacterData>> AvailableCharacters;

	UPROPERTY(Transient)
	TArray<TObjectPtr<UUI_CharacterSelectSlot>> CreatedSlots;

	int32 CurrentSelectedIndex = 0;
};
