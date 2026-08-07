#include "ItemSystem/UI/W_FloatingRecoveryText.h"
#include "Components/TextBlock.h"

void UW_FloatingRecoveryText::SetRecoveryText(int32 Amount, bool bIsMana)
{
	if (!TXT_Value)
	{
		return;
	}

	bCachedIsMana = bIsMana;

	TXT_Value->SetText(FText::FromString(FString::Printf(TEXT("+%d"), Amount)));

	const FLinearColor BaseColor = bIsMana
		? FLinearColor(0.2f, 0.6f, 1.0f, 1.0f)
		: FLinearColor(0.2f, 1.0f, 0.2f, 1.0f);

	TXT_Value->SetColorAndOpacity(FSlateColor(BaseColor));
}

void UW_FloatingRecoveryText::SetRecoveryAlpha(float Alpha)
{
	if (!TXT_Value)
	{
		return;
	}

	Alpha = FMath::Clamp(Alpha, 0.0f, 1.0f);

	const FLinearColor BaseColor = bCachedIsMana
		? FLinearColor(0.2f, 0.6f, 1.0f, Alpha)
		: FLinearColor(0.2f, 1.0f, 0.2f, Alpha);

	TXT_Value->SetColorAndOpacity(FSlateColor(BaseColor));
}