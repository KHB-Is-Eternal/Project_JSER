// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "UI/UI_MinimapProjection.h" // CPU 미니맵 아이콘 구조체
#include "UI_Scoreboard.generated.h"

class UUI_ScoreboardPlayerRow; // << 핵심

class UScrollBox;
class UTextBlock;
class UButton;
class UProgressBar;
class UImage;
class UCharacterData;
class UAbilitySystemComponent;
class AER_GameState;
class AER_PlayerState;

// CPU 미니맵용
class UCanvasPanel;
class AUI_AMiniMapCapture;
class ABaseCharacter;
class UTexture2D;

UCLASS()
class PROJECTER_API UUI_Scoreboard : public UUserWidget
{
	GENERATED_BODY()

protected:
	UPROPERTY(meta = (BindWidget))
	UScrollBox* SB_PlayerRow;
	UPROPERTY(meta = (BindWidget))
	UImage* TEX_FullMap;

	UPROPERTY(EditAnywhere, Category = "UI")
	TSubclassOf<UUI_ScoreboardPlayerRow> RowWidgetClass;

	void NativeConstruct() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override; // CPU 미니맵 아이콘 갱신용

	// === CPU 미니맵 아이콘 (신규) — 전체맵 고정 매핑 ===

	// 아이콘을 올릴 캔버스 (WBP에서 TEX_FullMap 위에 겹쳐 배치)
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UCanvasPanel> MinimapIconCanvas;

	// 전체맵 뷰 회전각 (캡처 카메라 Yaw와 일치, 기본 0°)
	UPROPERTY(EditDefaultsOnly, Category = "UI|Minimap")
	float MinimapViewRotationDeg = 0.f;

	// 아이콘 그림 자체의 시각적 회전 보정 (위치는 그대로, 그림만 회전)
	UPROPERTY(EditDefaultsOnly, Category = "UI|Minimap")
	float MinimapIconRotationDeg = -45.f;

	// 팀색 링 텍스처 (화이트 링 — 팀색은 틴트로 적용)
	UPROPERTY(EditDefaultsOnly, Category = "UI|Minimap")
	TSoftObjectPtr<UTexture2D> MinimapRingTexture;

	// 얼굴 아이콘 픽셀 크기
	UPROPERTY(EditDefaultsOnly, Category = "UI|Minimap")
	float MinimapFaceIconSize = 24.f;

	// 팀색 링 픽셀 크기
	UPROPERTY(EditDefaultsOnly, Category = "UI|Minimap")
	float MinimapRingIconSize = 30.f;

public:
	UFUNCTION()
	void UpdateScoreboard();

private:
	UPROPERTY()
	AER_GameState* GS;

	UPROPERTY()
	TObjectPtr<AUI_AMiniMapCapture> MinimapCaptureActor; // 전체맵 1회 캡처 액터 (맵 기준 정보 제공)

	UPROPERTY()
	TMap<TObjectPtr<ABaseCharacter>, FMinimapIconPair> MinimapIcons; // 캐릭터별 아이콘 풀

	void UpdateMinimapIcons();
};
