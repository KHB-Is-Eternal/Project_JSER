#include "UI/UI_MinimapProjection.h"

#include "Blueprint/UserWidget.h"
#include "CharacterSystem/Character/BaseCharacter.h"
#include "CharacterSystem/Data/CharacterData.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/Image.h"
#include "Engine/Texture2D.h"
#include "LineOfSight/Management/Subsystem/LOSVisionSubsystem.h" // 시야 연동 (단일 질의 API)

// 수식 근거: 기존 UI_MainHUD::HandleMinimapClicked의 45° 회전 역산 로직과 동일한 매핑.
// 캡처(-90° 피치, 직교)에서 이미지 U+ = 월드 +Y, V+ = 월드 -X 이므로
//   정변환: rx = dY/W, ry = -dX/W → R(-θ) 회전 → 뷰 오프셋
//   역변환: 뷰 오프셋 → R(+θ) 회전 → dX = -ry*W, dY = rx*W

FVector2D FUI_MinimapProjection::WorldToViewOffset(const FVector& WorldPos, const FVector& ViewCenter, const float ViewWidth, const float RotationDeg)
{
    if (ViewWidth <= 0.f)
    {
        return FVector2D::ZeroVector;
    }

    const float RX = (WorldPos.Y - ViewCenter.Y) / ViewWidth;
    const float RY = -(WorldPos.X - ViewCenter.X) / ViewWidth;

    const float Rad = FMath::DegreesToRadians(-RotationDeg);
    const float C = FMath::Cos(Rad);
    const float S = FMath::Sin(Rad);

    return FVector2D(RX * C - RY * S, RX * S + RY * C);
}

FVector FUI_MinimapProjection::ViewOffsetToWorld(const FVector2D& ViewOffset, const FVector& ViewCenter, const float ViewWidth, const float RotationDeg)
{
    const float Rad = FMath::DegreesToRadians(RotationDeg);
    const float C = FMath::Cos(Rad);
    const float S = FMath::Sin(Rad);

    const float RX = ViewOffset.X * C - ViewOffset.Y * S;
    const float RY = ViewOffset.X * S + ViewOffset.Y * C;

    return FVector(ViewCenter.X - RY * ViewWidth, ViewCenter.Y + RX * ViewWidth, ViewCenter.Z);
}

FVector2D FUI_MinimapProjection::WorldToMapUV(const FVector& WorldPos, const FVector& MapCenter, const float MapOrthoWidth)
{
    const FVector2D Offset = WorldToViewOffset(WorldPos, MapCenter, MapOrthoWidth, 0.f);
    return FVector2D(Offset.X + 0.5f, Offset.Y + 0.5f);
}

FVector FUI_MinimapProjection::MapUVToWorld(const FVector2D& UV, const FVector& MapCenter, const float MapOrthoWidth)
{
    return ViewOffsetToWorld(FVector2D(UV.X - 0.5f, UV.Y - 0.5f), MapCenter, MapOrthoWidth, 0.f);
}

bool FUI_MinimapProjection::IsCharacterVisibleOnMinimap(const ABaseCharacter* Character, const ABaseCharacter* LocalChar)
{
    if (!Character || !LocalChar)
    {
        return false;
    }

    // 본인 또는 아군은 항상 표시
    if (Character == LocalChar || Character->GetTeamType() == LocalChar->GetTeamType())
    {
        return true;
    }

    // 적군: 시야 시스템 연동 (단일 질의 API — 컴포넌트 미부착 폴백 정책은 API가 담당)
    if (const UWorld* World = Character->GetWorld())
    {
        if (const ULOSVisionSubsystem* VisionSubsystem = World->GetSubsystem<ULOSVisionSubsystem>())
        {
            return VisionSubsystem->IsActorVisibleToLocalPlayer(Character);
        }
    }

    return true;
}

FLinearColor FUI_MinimapProjection::GetTeamColor(const ABaseCharacter* Character, const ABaseCharacter* LocalChar)
{
    // 기존 M_MinimapLine 팀색 로직과 동일
    if (Character == LocalChar)
    {
        return FLinearColor::Green;
    }

    if (!Character)
    {
        return FLinearColor::White;
    }

    switch (Character->GetTeamType())
    {
    case ETeamType::Team_A: return FLinearColor::Red;
    case ETeamType::Team_B: return FLinearColor::Blue;
    case ETeamType::Team_C: return FLinearColor::Yellow;
    default:                return FLinearColor::White;
    }
}

FMinimapIconPair FUI_MinimapProjection::CreateIconPair(UUserWidget* OwnerWidget, UCanvasPanel* Canvas, ABaseCharacter* Character,
    const ABaseCharacter* LocalChar, UTexture2D* RingTexture, const float RingSize, const float FaceSize)
{
    FMinimapIconPair NewPair;
    if (!OwnerWidget || !Canvas || !Character)
    {
        return NewPair;
    }

    // 팀색 링 (얼굴 아래 레이어 — 먼저 추가해야 뒤에 깔림)
    if (RingTexture)
    {
        NewPair.Ring = NewObject<UImage>(OwnerWidget);
        NewPair.Ring->SetBrushFromTexture(RingTexture);
        NewPair.Ring->SetColorAndOpacity(GetTeamColor(Character, LocalChar));
        if (UCanvasPanelSlot* RingSlot = Canvas->AddChildToCanvas(NewPair.Ring))
        {
            RingSlot->SetAnchors(FAnchors(0.f, 0.f));
            RingSlot->SetAlignment(FVector2D(0.5f, 0.5f));
            RingSlot->SetAutoSize(false);
            RingSlot->SetSize(FVector2D(RingSize, RingSize));
        }
        NewPair.Ring->SetVisibility(ESlateVisibility::Collapsed);
    }

    // 얼굴 아이콘
    NewPair.Face = NewObject<UImage>(OwnerWidget);
    if (Character->HeroData && Character->HeroData->CharacterIcon)
    {
        NewPair.Face->SetBrushFromTexture(Character->HeroData->CharacterIcon);
    }
    if (UCanvasPanelSlot* FaceSlot = Canvas->AddChildToCanvas(NewPair.Face))
    {
        FaceSlot->SetAnchors(FAnchors(0.f, 0.f));
        FaceSlot->SetAlignment(FVector2D(0.5f, 0.5f));
        FaceSlot->SetAutoSize(false);
        FaceSlot->SetSize(FVector2D(FaceSize, FaceSize));
    }
    NewPair.Face->SetVisibility(ESlateVisibility::Collapsed);

    return NewPair;
}

void FUI_MinimapProjection::RefreshFaceTexture(FMinimapIconPair& Icons, const ABaseCharacter* Character)
{
    if (Icons.Face && !Icons.Face->GetBrush().GetResourceObject()
        && Character && Character->HeroData && Character->HeroData->CharacterIcon)
    {
        Icons.Face->SetBrushFromTexture(Character->HeroData->CharacterIcon);
    }
}

void FUI_MinimapProjection::PlaceIconPair(FMinimapIconPair& Icons, const FVector2D& CanvasPos)
{
    if (Icons.Face)
    {
        Icons.Face->SetVisibility(ESlateVisibility::HitTestInvisible);
        if (UCanvasPanelSlot* FaceSlot = Cast<UCanvasPanelSlot>(Icons.Face->Slot))
        {
            FaceSlot->SetPosition(CanvasPos);
        }
    }

    if (Icons.Ring)
    {
        Icons.Ring->SetVisibility(ESlateVisibility::HitTestInvisible);
        if (UCanvasPanelSlot* RingSlot = Cast<UCanvasPanelSlot>(Icons.Ring->Slot))
        {
            RingSlot->SetPosition(CanvasPos);
        }
    }
}
