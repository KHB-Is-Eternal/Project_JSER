#include "ItemSystem/UI/UI_WardHPBar.h"
#include "Components/Border.h"

void UUI_WardHPBar::UpdateBar(int32 Current, int32 Max, bool bIsAlly)
{
	const FLinearColor FillColor = bIsAlly ? AllyColor : EnemyColor;

	// 칸 i는 i < Current 이면 팀색, 아니면 EmptyColor (오른쪽부터 깎임)
	ApplySegment(Segment_0, 0, Current, FillColor);
	ApplySegment(Segment_1, 1, Current, FillColor);
	ApplySegment(Segment_2, 2, Current, FillColor);
	ApplySegment(Segment_3, 3, Current, FillColor);
}

void UUI_WardHPBar::ApplySegment(UBorder* Segment, int32 Index, int32 Current, const FLinearColor& FillColor) const
{
	if (!Segment)
	{
		return;
	}
	Segment->SetBrushColor(Index < Current ? FillColor : EmptyColor);
}
