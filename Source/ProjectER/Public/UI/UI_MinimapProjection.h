#pragma once

#include "CoreMinimal.h"
#include "UI_MinimapProjection.generated.h"

class ABaseCharacter;
class UCanvasPanel;
class UImage;
class UTexture2D;
class UUserWidget;

// CPU 미니맵 아이콘 한 쌍 (팀색 링 + 얼굴 아이콘) — HUD/스코어보드 공용
USTRUCT()
struct FMinimapIconPair
{
    GENERATED_BODY()

    UPROPERTY()
    TObjectPtr<UImage> Ring = nullptr;

    UPROPERTY()
    TObjectPtr<UImage> Face = nullptr;
};

/**
 * 월드 ↔ 미니맵 좌표 변환 + 아이콘 공통 헬퍼 (HUD/스코어보드 공용)
 * - 캡처 기준: -90° 피치 탑다운 직교 캡처 (U+ = 월드 +Y, V+ = 월드 -X)
 * - RotationDeg: 미니맵 뷰 회전각 (HUD = 45°, 전체맵 = 0°)
 */
struct PROJECTER_API FUI_MinimapProjection
{
    // === 좌표 변환 ===

    // 월드 좌표 → 뷰 중심 기준 오프셋 (-0.5 ~ 0.5, 뷰 안일 때)
    static FVector2D WorldToViewOffset(const FVector& WorldPos, const FVector& ViewCenter, float ViewWidth, float RotationDeg);

    // 뷰 오프셋 (-0.5 ~ 0.5) → 월드 좌표 (클릭 역변환, Z는 ViewCenter.Z 유지)
    static FVector ViewOffsetToWorld(const FVector2D& ViewOffset, const FVector& ViewCenter, float ViewWidth, float RotationDeg);

    // 월드 좌표 → 전체맵 UV (0 ~ 1, 회전 없음)
    static FVector2D WorldToMapUV(const FVector& WorldPos, const FVector& MapCenter, float MapOrthoWidth);

    // 전체맵 UV (0 ~ 1) → 월드 좌표
    static FVector MapUVToWorld(const FVector2D& UV, const FVector& MapCenter, float MapOrthoWidth);

    // === 아이콘 공통 유틸 ===

    // 본인/아군 항상 표시, 적군은 시야 내(GetVisibilityAlpha > 0)일 때만 (UpdateCraftingUIVisibility와 동일 패턴)
    static bool IsCharacterVisibleOnMinimap(const ABaseCharacter* Character, const ABaseCharacter* LocalChar);

    // 기존 M_MinimapLine 팀색 로직 (본인 Green / A Red / B Blue / C Yellow)
    static FLinearColor GetTeamColor(const ABaseCharacter* Character, const ABaseCharacter* LocalChar);

    // 캔버스에 링 + 얼굴 아이콘 위젯을 생성 (초기 상태 Collapsed). 실패 시 Face가 nullptr
    static FMinimapIconPair CreateIconPair(UUserWidget* OwnerWidget, UCanvasPanel* Canvas, ABaseCharacter* Character,
        const ABaseCharacter* LocalChar, UTexture2D* RingTexture, float RingSize, float FaceSize);

    // HeroData 지연 리플리케이션 대응 — 얼굴 텍스처가 아직 없으면 재적용 시도
    static void RefreshFaceTexture(FMinimapIconPair& Icons, const ABaseCharacter* Character);

    // 아이콘 쌍을 캔버스 좌표에 배치하고 표시
    static void PlaceIconPair(FMinimapIconPair& Icons, const FVector2D& CanvasPos);
};
