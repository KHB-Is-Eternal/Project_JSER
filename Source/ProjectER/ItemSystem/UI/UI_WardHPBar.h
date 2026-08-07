#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "UI_WardHPBar.generated.h"

class UBorder;

/**
 * 와드 머리 위 HP 바 (4칸 분절) — 로직은 전부 C++.
 * WBP는 이름이 정확히 일치하는 Border 4개(Segment_0~3)만 배치하면 자동 바인딩된다.
 * (그래프 노드 작성 불필요)
 */
UCLASS()
class PROJECTER_API UUI_WardHPBar : public UUserWidget
{
	GENERATED_BODY()

public:
	// C++(BaseWardActor::RefreshHPBar)가 호출. 남은 칸/전체 칸 + 아군 여부로 4칸을 칠한다.
	void UpdateBar(int32 Current, int32 Max, bool bIsAlly);

protected:
	// WBP에 아래 이름의 Border 4개를 배치하면 자동 연결됨 (BindWidget)
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UBorder> Segment_0;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UBorder> Segment_1;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UBorder> Segment_2;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UBorder> Segment_3;

	// 색상 — 기본값은 여기(코드)에서 정하고, 필요 시 WBP Class Defaults에서 조정 가능
	UPROPERTY(EditAnywhere, Category = "Ward|HPBar")
	FLinearColor AllyColor = FLinearColor(0.35f, 0.90f, 0.25f, 1.0f);   // 연두

	UPROPERTY(EditAnywhere, Category = "Ward|HPBar")
	FLinearColor EnemyColor = FLinearColor(0.90f, 0.12f, 0.12f, 1.0f);  // 붉은

	UPROPERTY(EditAnywhere, Category = "Ward|HPBar")
	FLinearColor EmptyColor = FLinearColor(0.06f, 0.06f, 0.06f, 0.7f);  // 깎인 칸

private:
	void ApplySegment(UBorder* Segment, int32 Index, int32 Current, const FLinearColor& FillColor) const;
};
