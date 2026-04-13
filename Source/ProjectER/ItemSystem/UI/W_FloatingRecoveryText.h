#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "W_FloatingRecoveryText.generated.h"

class UTextBlock;
class UWidgetAnimation;

UCLASS()
class PROJECTER_API UW_FloatingRecoveryText : public UUserWidget
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category = "RecoveryText")
	void SetRecoveryText(int32 Amount, bool bIsMana);

protected:
	virtual void NativeConstruct() override;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> TXT_Value;

	UPROPERTY(Transient, meta = (BindWidgetAnimOptional))
	TObjectPtr<UWidgetAnimation> ANIM_FloatFade;
};