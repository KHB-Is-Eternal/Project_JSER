// Fill out your copyright notice in the Description page of Project Settings.


#include "UI/UI_Scoreboard.h"
#include "Components/TextBlock.h"
#include "Components/Button.h"
#include "Components/ProgressBar.h"
#include "Components/Image.h"
#include "Components/ScrollBox.h"

#include "Kismet/GameplayStatics.h" // gamestate용
#include "GameModeBase/State/ER_GameState.h" // gamestate
#include "GameModeBase/State/ER_PlayerState.h"

#include "UI/UI_ScoreboardPlayerRow.h"

// CPU 미니맵용
#include "Components/CanvasPanel.h"
#include "UI/UI_AMiniMapCapture.h"
#include "CharacterSystem/Character/BaseCharacter.h"
#include "Engine/Texture2D.h"
#include "EngineUtils.h" // TActorIterator
#include "GameFramework/PlayerState.h"

void UUI_Scoreboard::NativeConstruct()
{
    Super::NativeConstruct();
    GS = GetWorld()->GetGameState<AER_GameState>();

}

void UUI_Scoreboard::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
    Super::NativeTick(MyGeometry, InDeltaTime);

    // 스코어보드가 화면에 보일 때만 틱이 돌므로 별도 게이트 불필요
    UpdateMinimapIcons();
}

void UUI_Scoreboard::UpdateMinimapIcons()
{
    if (!IsValid(MinimapIconCanvas) || !IsValid(GS))
    {
        return;
    }

    const FVector2D CanvasSize = MinimapIconCanvas->GetCachedGeometry().GetLocalSize();
    if (CanvasSize.IsNearlyZero())
    {
        return;
    }

    // 전체맵 1회 캡처 액터 캐싱 (BasePlayerController와 동일 패턴)
    if (!IsValid(MinimapCaptureActor))
    {
        for (TActorIterator<AUI_AMiniMapCapture> It(GetWorld()); It; ++It)
        {
            MinimapCaptureActor = *It;
            break;
        }
        if (!IsValid(MinimapCaptureActor))
        {
            return;
        }
    }

    const float MapWidth = MinimapCaptureActor->GetMapOrthoWidth();
    if (MapWidth <= 0.f)
    {
        return;
    }

    const ABaseCharacter* LocalChar = Cast<ABaseCharacter>(GetOwningPlayerPawn());
    if (!IsValid(LocalChar))
    {
        return;
    }

    // 전부 숨김 후 이번 프레임에 유효한 아이콘만 다시 표시 (사망/리스폰 잔상 방지)
    for (auto& Pair : MinimapIcons)
    {
        if (Pair.Value.Face) Pair.Value.Face->SetVisibility(ESlateVisibility::Collapsed);
        if (Pair.Value.Ring) Pair.Value.Ring->SetVisibility(ESlateVisibility::Collapsed);
    }

    for (APlayerState* PS : GS->PlayerArray)
    {
        if (!IsValid(PS))
        {
            continue;
        }

        ABaseCharacter* Character = Cast<ABaseCharacter>(PS->GetPawn());
        if (!IsValid(Character))
        {
            continue;
        }

        FMinimapIconPair* Icons = MinimapIcons.Find(Character);
        if (!Icons)
        {
            FMinimapIconPair NewPair = FUI_MinimapProjection::CreateIconPair(this, MinimapIconCanvas, Character,
                LocalChar, MinimapRingTexture.LoadSynchronous(), MinimapRingIconSize, MinimapFaceIconSize);
            if (!NewPair.Face)
            {
                continue;
            }
            Icons = &MinimapIcons.Add(Character, NewPair);
        }

        // HeroData가 늦게 리플리케이션된 경우 얼굴 텍스처 지연 적용
        FUI_MinimapProjection::RefreshFaceTexture(*Icons, Character);

        // 시야 판정 (HUD와 동일 규칙)
        if (!FUI_MinimapProjection::IsCharacterVisibleOnMinimap(Character, LocalChar))
        {
            continue;
        }

        // 전체맵 고정 매핑
        const FVector2D Offset = FUI_MinimapProjection::WorldToViewOffset(
            Character->GetActorLocation(), MinimapCaptureActor->GetMapCenter(), MapWidth, MinimapViewRotationDeg);
        if (FMath::Abs(Offset.X) > 0.5f || FMath::Abs(Offset.Y) > 0.5f)
        {
            continue;
        }

        FUI_MinimapProjection::PlaceIconPair(*Icons, FVector2D((Offset.X + 0.5f) * CanvasSize.X, (Offset.Y + 0.5f) * CanvasSize.Y));

        // 아이콘 그림 자체의 시각적 회전 보정 (위치와 무관)
        if (Icons->Face)
        {
            Icons->Face->SetRenderTransformAngle(MinimapIconRotationDeg);
        }
    }

    // 파괴된 캐릭터의 아이콘 정리
    for (auto It = MinimapIcons.CreateIterator(); It; ++It)
    {
        if (!IsValid(It.Key()))
        {
            if (It.Value().Face) It.Value().Face->RemoveFromParent();
            if (It.Value().Ring) It.Value().Ring->RemoveFromParent();
            It.RemoveCurrent();
        }
    }
}


void UUI_Scoreboard::UpdateScoreboard()
{
    SB_PlayerRow->ClearChildren();

    if (GS)
    {
        for (APlayerState* PS : GS->PlayerArray)
        {
			AER_PlayerState* MyPS = Cast<AER_PlayerState>(PS);
            if (MyPS)
            {
                
                UUI_ScoreboardPlayerRow* Row = CreateWidget<UUI_ScoreboardPlayerRow>(GetWorld(), RowWidgetClass);
                if (Row)
                {
                    
                    Row->Init(MyPS);                    
                    SB_PlayerRow->AddChild(Row);
                }
            }
        }
    }
}
