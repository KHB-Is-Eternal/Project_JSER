#include "UI/UI_CharacterSelectWidget.h"
#include "CharacterSystem/Data/CharacterData.h"
#include "GameModeBase/State/ER_GameState.h"
#include "GameModeBase/State/ER_PlayerState.h"
#include "CharacterSystem/Player/BasePlayerController.h"
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

	AER_GameState* GameState = Cast<AER_GameState>(UGameplayStatics::GetGameState(this));
	if (GameState)
	{
		AvailableCharacters = GameState->GetAvailableCharacterData();
	}

	if (AER_PlayerState* ERPS = GetOwningPlayerState<AER_PlayerState>())
	{
		ERPS->OnCharacterDataChanged.AddDynamic(this, &UUI_CharacterSelectWidget::OnPlayerStateCharacterChanged);
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

void UUI_CharacterSelectWidget::OnPlayerStateCharacterChanged(TSoftObjectPtr<UCharacterData> NewCharacterData)
{
	if (AvailableCharacters.IsEmpty()) return;

	for (int32 i = 0; i < AvailableCharacters.Num(); ++i)
	{
		if (AvailableCharacters[i] == NewCharacterData)
		{
			if (CurrentIndex != i)
			{
				SelectCharacter(i);
			}
			break;
		}
	}
}

void UUI_CharacterSelectWidget::OnPrevClicked()
{
	APlayerController* PC = GetOwningPlayer();
	AER_PlayerState* ERPS = PC ? PC->GetPlayerState<AER_PlayerState>() : nullptr;
	if (ERPS && (ERPS->bIsReady || ERPS->GetIsReady()))
	{
		return;
	}

	SelectCharacter(GetWrappedIndex(CurrentIndex - 1));
}

void UUI_CharacterSelectWidget::OnNextClicked()
{
	APlayerController* PC = GetOwningPlayer();
	AER_PlayerState* ERPS = PC ? PC->GetPlayerState<AER_PlayerState>() : nullptr;
	if (ERPS && (ERPS->bIsReady || ERPS->GetIsReady()))
	{
		return;
	}

	SelectCharacter(GetWrappedIndex(CurrentIndex + 1));
}

void UUI_CharacterSelectWidget::SelectCharacter(int32 Index)
{
	if (AvailableCharacters.IsEmpty()) return;

	CurrentIndex = Index;
	TransitionAlpha = 0.0f; // 애니메이션 시작

	UpdateCarouselImages();

	// 서버 통신 (캐릭터 선택 즉시 갱신)
	if (ABasePlayerController* PC = Cast<ABasePlayerController>(GetOwningPlayer()))
	{
		PC->Server_SelectCharacter(AvailableCharacters[CurrentIndex]);
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
