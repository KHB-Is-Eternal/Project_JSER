#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "W_FloatingRecoveryText.generated.h"

class UTextBlock;

UCLASS()
class PROJECTER_API UW_FloatingRecoveryText : public UUserWidget
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category = "RecoveryText")
	void SetRecoveryText(int32 Amount, bool bIsMana);

	UFUNCTION(BlueprintCallable, Category = "RecoveryText")
	void SetRecoveryAlpha(float Alpha);

protected:
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> TXT_Value;

private:
	bool bCachedIsMana = false;
};