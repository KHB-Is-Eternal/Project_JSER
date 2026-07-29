#include "UI/UI_CharacterSelectWidget.h"
#include "UI/UI_CharacterSelectSlot.h"
#include "CharacterSystem/Data/CharacterData.h"
#include "GameModeBase/State/ER_GameState.h"
#include "CharacterSystem/Player/BasePlayerController.h"
#include "Components/UniformGridPanel.h"
#include "Components/UniformGridSlot.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/VerticalBoxSlot.h"
#include "Components/Image.h"
#include "Components/Button.h"
#include "Components/TextBlock.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Kismet/GameplayStatics.h"

void UUI_CharacterSelectWidget::NativeConstruct()
{
	Super::NativeConstruct();

	if (Button_Prev)
	{
		Button_Prev->OnClicked.AddDynamic(this, &UUI_CharacterSelectWidget::OnPrevClicked);
	}
	if (Button_Next)
	{
		Button_Next->OnClicked.AddDynamic(this, &UUI_CharacterSelectWidget::OnNextClicked);
	}
	if (Button_SelectConfirm)
	{
		Button_SelectConfirm->OnClicked.AddDynamic(this, &UUI_CharacterSelectWidget::OnSelectConfirmClicked);
	}

	AER_GameState* GameState = Cast<AER_GameState>(UGameplayStatics::GetGameState(this));
	if (GameState && SlotWidgetClass)
	{
		AvailableCharacters = GameState->GetAvailableCharacterData();
		
		if (GridPanel_Characters)
		{
			GridPanel_Characters->ClearChildren();
			CreatedSlots.Empty();

			const int32 Columns = 8;
			const int32 TotalNum = AvailableCharacters.Num();

			// GridPanel의 고정 영역 크기 획득 (CanvasPanelSlot 우선, 미적용 시 CachedGeometry)
			FVector2D GridSize = FVector2D::ZeroVector;
			if (UCanvasPanelSlot* CanvasSlot = Cast<UCanvasPanelSlot>(GridPanel_Characters->Slot))
			{
				GridSize = CanvasSlot->GetSize();
			}
			if (GridSize.IsZero())
			{
				GridSize = GridPanel_Characters->GetCachedGeometry().GetLocalSize();
			}

			// 정사각형 슬롯 크기 산출 (가로/세로 공간 N등분 중 최소값)
			float SquareSize = 0.0f;
			if (TotalNum > 0 && GridSize.X > 0.0f && GridSize.Y > 0.0f)
			{
				const int32 Rows = FMath::CeilToInt((float)TotalNum / (float)Columns);
				const float CellWidth = GridSize.X / (float)Columns;
				const float CellHeight = GridSize.Y / (float)Rows;
				SquareSize = FMath::Min(CellWidth, CellHeight);
			}

			for (int32 i = 0; i < TotalNum; ++i)
			{
				UCharacterData* CharData = AvailableCharacters[i].LoadSynchronous();
				if (CharData)
				{
					UUI_CharacterSelectSlot* NewSlot = CreateWidget<UUI_CharacterSelectSlot>(GetOwningPlayer(), SlotWidgetClass);
					if (NewSlot)
					{
						NewSlot->InitSlot(i, CharData, this);
						int32 Row = i / Columns;
						int32 Col = i % Columns;
						GridPanel_Characters->AddChildToUniformGrid(NewSlot, Row, Col);
						CreatedSlots.Add(NewSlot);
					}
				}
			}

			// 슬롯 배치 후 크기 갱신
			RefreshSlotSizes();
		}
	}

	if (Image_CenterCard)
	{
		BaseCenterScale = Image_CenterCard->GetRenderTransform().Scale;
		BaseCenterOpacity = Image_CenterCard->GetRenderOpacity();
	}
	if (Image_LeftCard)
	{
		// 좌우 카드의 기준 스케일은 동일할 것이라 가정
		BaseSideScale = Image_LeftCard->GetRenderTransform().Scale;
		BaseSideOpacity = Image_LeftCard->GetRenderOpacity();
	}

	SelectCharacter(0);
}

void UUI_CharacterSelectWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);

	// NativeConstruct 시점에 지오메트리 패스가 완료되지 않아 크기가 0이었던 경우 1번째 Tick에서 재갱신
	if (!bSizeUpdated)
	{
		RefreshSlotSizes();
	}

	if (TransitionAlpha < 1.0f)
	{
		TransitionAlpha = FMath::Clamp(TransitionAlpha + InDeltaTime * TransitionSpeed, 0.0f, 1.0f);
		
		// 블루프린트에서 세팅한 원본 스케일에 배율(Multiplier)을 곱함
		if (Image_LeftCard && Image_CenterCard && Image_RightCard)
		{
			float CenterAnimScale = FMath::Lerp(0.8f, 1.0f, TransitionAlpha);
			float CenterAnimOpacity = FMath::Lerp(0.6f, 1.0f, TransitionAlpha);
			Image_CenterCard->SetRenderScale(BaseCenterScale * CenterAnimScale);
			Image_CenterCard->SetRenderOpacity(BaseCenterOpacity * CenterAnimOpacity);

			// 좌우 카드 (100% 였던 카드가 옆으로 가면서 80% 스케일, 0.6 Opacity로 퇴장)
			float SideAnimScale = FMath::Lerp(1.0f, 0.8f, TransitionAlpha);
			float SideAnimOpacity = FMath::Lerp(1.0f, 0.6f, TransitionAlpha);
			Image_LeftCard->SetRenderScale(BaseSideScale * SideAnimScale);
			Image_LeftCard->SetRenderOpacity(BaseSideOpacity * SideAnimOpacity);
			Image_RightCard->SetRenderScale(BaseSideScale * SideAnimScale);
			Image_RightCard->SetRenderOpacity(BaseSideOpacity * SideAnimOpacity);
		}
	}
}

void UUI_CharacterSelectWidget::RefreshSlotSizes()
{
	if (!GridPanel_Characters || CreatedSlots.IsEmpty()) return;

	// Slate 레이아웃 패스 강제 업데이트 시도
	GridPanel_Characters->ForceLayoutPrepass();

	FVector2D GridSize = FVector2D::ZeroVector;
	if (UCanvasPanelSlot* CanvasSlot = Cast<UCanvasPanelSlot>(GridPanel_Characters->Slot))
	{
		GridSize = CanvasSlot->GetSize();
	}
	else if (UVerticalBoxSlot* VertSlot = Cast<UVerticalBoxSlot>(GridPanel_Characters->Slot))
	{
		GridSize = GridPanel_Characters->GetCachedGeometry().GetLocalSize();
	}

	// CanvasPanelSlot이 아니거나 1px 미만인 경우 CachedGeometry 픽셀 크기 획득 시도
	if (GridSize.X < 1.0f || GridSize.Y < 1.0f)
	{
		GridSize = GridPanel_Characters->GetCachedGeometry().GetLocalSize();
	}

	// 유효한 픽셀 크기(최소 1px)가 확보되지 않았다면 이번 프레임 연산 중단 (bSizeUpdated = false 상태 유지)
	if (GridSize.X < 1.0f || GridSize.Y < 1.0f)
	{
		return;
	}

	const int32 Columns = 8;
	const int32 TotalNum = CreatedSlots.Num();
	const int32 Rows = FMath::CeilToInt((float)TotalNum / (float)Columns);
	const int32 UsedColumns = FMath::Min(Columns, TotalNum);

	// 세로 공간을 넘지 않는 안전한 1:1 정사각형 슬롯 기준 크기 산출
	const float MaxCellWidth = GridSize.X / (float)UsedColumns;
	const float MaxCellHeight = GridSize.Y / (float)Rows;
	const float BaseSquareSize = FMath::Min(MaxCellWidth, MaxCellHeight);

	// SlotScaleMultiplier 적용하여 최종 정사각형 슬롯 크기 결정
	const float FinalSquareSize = FMath::Max(1.0f, BaseSquareSize * SlotScaleMultiplier);

	if (FinalSquareSize >= 1.0f)
	{
		// 1. 모든 슬롯의 1:1 정사각형 크기 지정
		for (UUI_CharacterSelectSlot* CharSlot : CreatedSlots)
		{
			if (CharSlot)
			{
				CharSlot->SetSlotSquareSize(FinalSquareSize);
			}
		}

		// 2. 전체 슬롯 그룹의 가로 총 폭 및 중앙 정렬 오프셋 연산
		const float EffectiveGap = FMath::Max(0.0f, SlotGap);
		const float TotalGroupWidth = ((float)UsedColumns * FinalSquareSize) + ((float)(UsedColumns - 1) * EffectiveGap);
		const float StartOffset = (GridSize.X - TotalGroupWidth) * 0.5f;

		const float DefaultCellWidth = GridSize.X / (float)UsedColumns;

		for (int32 i = 0; i < TotalNum; ++i)
		{
			if (CreatedSlots.IsValidIndex(i) && CreatedSlots[i])
			{
				int32 Col = i % Columns;
				// 슬롯 그룹이 패널 중앙에 오도록 목표 위치(TargetX)를 설정하고 오프셋 차이 적용
				float TargetX = StartOffset + ((float)Col * (FinalSquareSize + EffectiveGap));
				float GridCellX = (float)Col * DefaultCellWidth;
				float XOffset = TargetX - GridCellX;

				CreatedSlots[i]->SetRenderTranslation(FVector2D(XOffset, 0.0f));
			}
		}

		bSizeUpdated = true;
	}
}

int32 UUI_CharacterSelectWidget::GetWrappedIndex(int32 Index) const
{
	if (AvailableCharacters.IsEmpty()) return 0;
	
	int32 Num = AvailableCharacters.Num();
	int32 Wrapped = Index % Num;
	if (Wrapped < 0)
	{
		Wrapped += Num;
	}
	return Wrapped;
}

void UUI_CharacterSelectWidget::OnSlotSelected(int32 SlotIndex)
{
	if (bIsReady) return; // 이미 레디 상태면 변경 불가
	SelectCharacter(SlotIndex);
}

void UUI_CharacterSelectWidget::OnPrevClicked()
{
	if (bIsReady) return;
	SelectCharacter(GetWrappedIndex(CurrentIndex - 1));
}

void UUI_CharacterSelectWidget::OnNextClicked()
{
	if (bIsReady) return;
	SelectCharacter(GetWrappedIndex(CurrentIndex + 1));
}

void UUI_CharacterSelectWidget::SelectCharacter(int32 Index)
{
	if (AvailableCharacters.IsEmpty()) return;

	CurrentIndex = Index;
	TransitionAlpha = 0.0f; // 애니메이션 시작

	UpdateSlotsHighlight();
	UpdateCarouselImages();

	// 서버 통신 (캐릭터 선택 즉시 갱신)
	if (ABasePlayerController* PC = Cast<ABasePlayerController>(GetOwningPlayer()))
	{
		PC->Server_SelectCharacter(AvailableCharacters[CurrentIndex]);
	}
}

void UUI_CharacterSelectWidget::UpdateSlotsHighlight()
{
	for (int32 i = 0; i < CreatedSlots.Num(); ++i)
	{
		if (CreatedSlots[i])
		{
			CreatedSlots[i]->SetHighlight(i == CurrentIndex);
		}
	}
}

void UUI_CharacterSelectWidget::UpdateCarouselImages()
{
	if (AvailableCharacters.IsEmpty()) return;

	int32 PrevIndex = GetWrappedIndex(CurrentIndex - 1);
	int32 NextIndex = GetWrappedIndex(CurrentIndex + 1);

	auto SetCardImage = [this](UImage* CardImage, int32 DataIndex)
	{
		if (CardImage)
		{
			UCharacterData* CharData = AvailableCharacters[DataIndex].LoadSynchronous();
			if (CharData && CharData->CharacterIcon)
			{
				if (UMaterialInstanceDynamic* DynamicMat = CardImage->GetDynamicMaterial())
				{
					DynamicMat->SetTextureParameterValue(FName("CharacterIcon"), CharData->CharacterIcon);
				}
				else
				{
					CardImage->SetBrushFromTexture(CharData->CharacterIcon);
				}
			}
		}
	};

	SetCardImage(Image_CenterCard, CurrentIndex);
	SetCardImage(Image_LeftCard, PrevIndex);
	SetCardImage(Image_RightCard, NextIndex);

	// 캐릭터 이름 텍스트 갱신
	if (Text_CharacterName)
	{
		UCharacterData* CenterData = AvailableCharacters[CurrentIndex].LoadSynchronous();
		if (CenterData)
		{
			Text_CharacterName->SetText(FText::FromName(CenterData->StatusRowName));
		}
	}
}

void UUI_CharacterSelectWidget::OnSelectConfirmClicked()
{
	if (ABasePlayerController* PC = Cast<ABasePlayerController>(GetOwningPlayer()))
	{
		PC->Server_ToggleReadyState();
		
		bIsReady = !bIsReady;
	}
}
