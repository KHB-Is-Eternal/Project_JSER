#include "ItemSystem/UI/W_FloatingRecoveryText.h"
#include "Components/TextBlock.h"

void UW_FloatingRecoveryText::NativeConstruct()
{
	Super::NativeConstruct();

	if (ANIM_FloatFade)
	{
		PlayAnimation(ANIM_FloatFade);
	}
}

void UW_FloatingRecoveryText::SetRecoveryText(int32 Amount, bool bIsMana)
{
	if (!TXT_Value)
	{
		return;
	}

	TXT_Value->SetText(FText::FromString(FString::Printf(TEXT("+%d"), Amount)));

	if (bIsMana)
	{
		TXT_Value->SetColorAndOpacity(FSlateColor(FLinearColor(0.2f, 0.6f, 1.0f, 1.0f)));
	}
	else
	{
		TXT_Value->SetColorAndOpacity(FSlateColor(FLinearColor(0.2f, 1.0f, 0.2f, 1.0f)));
	}
}