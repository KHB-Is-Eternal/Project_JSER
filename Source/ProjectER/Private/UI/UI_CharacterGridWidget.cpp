#include "UI/UI_CharacterGridWidget.h"
#include "UI/UI_CharacterSelectSlot.h"
#include "CharacterSystem/Data/CharacterData.h"
#include "GameModeBase/State/ER_GameState.h"
#include "GameModeBase/State/ER_PlayerState.h"
#include "CharacterSystem/Player/BasePlayerController.h"
#include "Components/UniformGridPanel.h"
#include "Components/UniformGridSlot.h"
#include "Kismet/GameplayStatics.h"

void UUI_CharacterGridWidget::NativeConstruct()
{
	Super::NativeConstruct();

	if (AER_GameState* GameState = Cast<AER_GameState>(UGameplayStatics::GetGameState(this)))
	{
		InitGrid(GameState->GetAvailableCharacterData());
	}

	if (AER_PlayerState* ERPS = GetOwningPlayerState<AER_PlayerState>())
	{
		ERPS->OnCharacterDataChanged.AddDynamic(this, &UUI_CharacterGridWidget::OnPlayerStateCharacterChanged);
	}
}

void UUI_CharacterGridWidget::NativeDestruct()
{
	// 위젯 재구성 시 중복 바인딩(ensure) 방지를 위해 해제
	if (AER_PlayerState* ERPS = GetOwningPlayerState<AER_PlayerState>())
	{
		ERPS->OnCharacterDataChanged.RemoveDynamic(this, &UUI_CharacterGridWidget::OnPlayerStateCharacterChanged);
	}

	Super::NativeDestruct();
}

void UUI_CharacterGridWidget::InitGrid(const TArray<TSoftObjectPtr<UCharacterData>>& InAvailableCharacters)
{
	if (!GridPanel_Characters || !SlotWidgetClass) return;

	GridPanel_Characters->ClearChildren();
	CreatedSlots.Empty();
	AvailableCharacters = InAvailableCharacters;

	const int32 Columns = 8;
	const int32 TotalNum = AvailableCharacters.Num();

	for (int32 i = 0; i < TotalNum; ++i)
	{
		UCharacterData* CharData = AvailableCharacters[i].LoadSynchronous();
		if (!CharData) continue;

		UUI_CharacterSelectSlot* NewSlot = CreateWidget<UUI_CharacterSelectSlot>(GetOwningPlayer(), SlotWidgetClass);
		if (NewSlot)
		{
			// 그리드 편입 시점에 슬롯의 NativeConstruct(레디 상태 바인딩)가 먼저 실행되도록 InitSlot보다 앞서 호출
			int32 Row = i / Columns;
			int32 Col = i % Columns;
			GridPanel_Characters->AddChildToUniformGrid(NewSlot, Row, Col);
			NewSlot->InitSlot(i, CharData, this);
			CreatedSlots.Add(NewSlot);
		}
	}

	// 슬롯 생성이 완료되면 0번 슬롯에 자동으로 하이라이트 외곽선 켜기
	if (!CreatedSlots.IsEmpty())
	{
		SetSelectedHighlight(0);
	}
}

void UUI_CharacterGridWidget::SetSelectedHighlight(int32 SelectedIndex)
{
	CurrentSelectedIndex = SelectedIndex;
	for (int32 i = 0; i < CreatedSlots.Num(); ++i)
	{
		if (CreatedSlots[i])
		{
			CreatedSlots[i]->SetHighlight(i == SelectedIndex);
		}
	}
}

void UUI_CharacterGridWidget::OnPlayerStateCharacterChanged(TSoftObjectPtr<UCharacterData> NewCharacterData)
{
	if (AvailableCharacters.IsEmpty()) return;

	for (int32 i = 0; i < AvailableCharacters.Num(); ++i)
	{
		if (AvailableCharacters[i] == NewCharacterData)
		{
			if (CurrentSelectedIndex != i)
			{
				SetSelectedHighlight(i);
			}
			break;
		}
	}
}

void UUI_CharacterGridWidget::OnSlotSelected(int32 SlotIndex)
{
	ABasePlayerController* BasePC = Cast<ABasePlayerController>(GetOwningPlayer());
	AER_PlayerState* ERPS = BasePC ? BasePC->GetPlayerState<AER_PlayerState>() : nullptr;

	// 레디 상태이면 캐릭터 선택 변경 강제 차단
	if (ERPS && (ERPS->bIsReady || ERPS->GetIsReady()))
	{
		return;
	}

	// 1. 하이라이트 즉시 갱신
	SetSelectedHighlight(SlotIndex);

	// 2. 플레이어 컨트롤러에 캐릭터 선택 서버 전송 (이후 ER_PlayerState 네트워크 복제에 의해 두 위젯 자동 반응)
	if (BasePC && AvailableCharacters.IsValidIndex(SlotIndex))
	{
		BasePC->Server_SelectCharacter(AvailableCharacters[SlotIndex]);
	}

	// 3. 외부 수신용 델리게이트 알림
	OnCharacterSlotSelected.Broadcast(SlotIndex);
}
