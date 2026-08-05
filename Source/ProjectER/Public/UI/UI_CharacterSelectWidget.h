#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "UI_CharacterSelectWidget.generated.h"

class UImage;
class UButton;
class UTextBlock;
class UCharacterData;
class UUI_CharacterGridWidget;

UCLASS()
class PROJECTER_API UUI_CharacterSelectWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

	UFUNCTION()
	void OnPlayerStateCharacterChanged(TSoftObjectPtr<UCharacterData> NewCharacterData);

protected:

	UPROPERTY(meta = (BindWidget), BlueprintReadOnly, Category = "ProjectER|UI")
	TObjectPtr<UImage> Image_LeftCard;

	UPROPERTY(meta = (BindWidget), BlueprintReadOnly, Category = "ProjectER|UI")
	TObjectPtr<UImage> Image_CenterCard;

	UPROPERTY(meta = (BindWidget), BlueprintReadOnly, Category = "ProjectER|UI")
	TObjectPtr<UImage> Image_RightCard;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UButton> Button_Prev;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UButton> Button_Next;

	// UPROPERTY(meta = (BindWidget))
	// TObjectPtr<UButton> Button_SelectConfirm;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> Text_CharacterName;

	// UPROPERTY(meta = (BindWidget))
	// TObjectPtr<UTextBlock> Text_ConfirmButton;

	UFUNCTION()
	void OnPrevClicked();

	UFUNCTION()
	void OnNextClicked();

	// UFUNCTION()
	// void OnSelectConfirmClicked();

private:
	TArray<TSoftObjectPtr<UCharacterData>> AvailableCharacters;

	int32 CurrentIndex = 0;
	int32 TargetIndex = 0;
	
	// 애니메이션용 보간 변수
	float TransitionAlpha = 1.0f;
	const float TransitionSpeed = 10.0f;

	// 원본 크기 저장을 위한 변수
	FVector2D BaseCenterScale = FVector2D(1.0f, 1.0f);
	float BaseCenterOpacity = 1.0f;
	FVector2D BaseSideScale = FVector2D(1.0f, 1.0f);
	float BaseSideOpacity = 1.0f;

	void UpdateCarouselImages();
	void SelectCharacter(int32 Index);
	int32 GetWrappedIndex(int32 Index) const;
	
	// 레디 상태 여부 기록
	bool bIsReady = false;
};
