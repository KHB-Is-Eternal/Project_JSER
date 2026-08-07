// Fill out your copyright notice in the Description page of Project Settings.


#include "UI/UI_HUDController.h"
#include "CharacterSystem/Player/BasePlayerState.h"
#include "CharacterSystem/GAS/AttributeSet/BaseAttributeSet.h"

void UUI_HUDController::SetParams(const FWidgetControllerParams& Params)
{
	PlayerController = Params.PlayerController;
	PlayerState = Params.PlayerState;
	AbilitySystemComponent = Params.AbilitySystemComponent;
	AttributeSet = Params.AttributeSet;
}

void UUI_HUDController::BindCallbacksToDependencies()
{
    UBaseAttributeSet* BaseAS = CastChecked<UBaseAttributeSet>(AttributeSet);

    // AddLambda([this]...) 대신 AddUObject를 사용합니다.
    // AddUObject는 내부적으로 TWeakObjectPtr를 사용하여 this가 GC로 파괴되면
    // 자동으로 바인딩을 해제하므로 댕글링 포인터 크래시를 원천 방지합니다.
    AbilitySystemComponent->GetGameplayAttributeValueChangeDelegate(BaseAS->GetLevelAttribute()).AddUObject(this, &UUI_HUDController::OnLevelAttributeChanged);
    AbilitySystemComponent->GetGameplayAttributeValueChangeDelegate(BaseAS->GetXPAttribute()).AddUObject(this, &UUI_HUDController::OnXPAttributeChanged);
    AbilitySystemComponent->GetGameplayAttributeValueChangeDelegate(BaseAS->GetHealthAttribute()).AddUObject(this, &UUI_HUDController::OnHealthAttributeChanged);
    AbilitySystemComponent->GetGameplayAttributeValueChangeDelegate(BaseAS->GetMaxHealthAttribute()).AddUObject(this, &UUI_HUDController::OnMaxHealthAttributeChanged);
    AbilitySystemComponent->GetGameplayAttributeValueChangeDelegate(BaseAS->GetStaminaAttribute()).AddUObject(this, &UUI_HUDController::OnStaminaAttributeChanged);
    AbilitySystemComponent->GetGameplayAttributeValueChangeDelegate(BaseAS->GetMaxStaminaAttribute()).AddUObject(this, &UUI_HUDController::OnMaxStaminaAttributeChanged);
    AbilitySystemComponent->GetGameplayAttributeValueChangeDelegate(BaseAS->GetAttackPowerAttribute()).AddUObject(this, &UUI_HUDController::OnAttackPowerAttributeChanged);
    AbilitySystemComponent->GetGameplayAttributeValueChangeDelegate(BaseAS->GetAttackSpeedAttribute()).AddUObject(this, &UUI_HUDController::OnAttackSpeedAttributeChanged);
    AbilitySystemComponent->GetGameplayAttributeValueChangeDelegate(BaseAS->GetDefenseAttribute()).AddUObject(this, &UUI_HUDController::OnDefenseAttributeChanged);
    AbilitySystemComponent->GetGameplayAttributeValueChangeDelegate(BaseAS->GetSkillAmpAttribute()).AddUObject(this, &UUI_HUDController::OnSkillAmpAttributeChanged);
    AbilitySystemComponent->GetGameplayAttributeValueChangeDelegate(BaseAS->GetCriticalChanceAttribute()).AddUObject(this, &UUI_HUDController::OnCriticalChanceAttributeChanged);
    AbilitySystemComponent->GetGameplayAttributeValueChangeDelegate(BaseAS->GetMoveSpeedAttribute()).AddUObject(this, &UUI_HUDController::OnMoveSpeedAttributeChanged);
    AbilitySystemComponent->GetGameplayAttributeValueChangeDelegate(BaseAS->GetCooldownReductionAttribute()).AddUObject(this, &UUI_HUDController::OnCooldownReductionAttributeChanged);
    AbilitySystemComponent->GetGameplayAttributeValueChangeDelegate(BaseAS->GetAttackRangeAttribute()).AddUObject(this, &UUI_HUDController::OnAttackRangeAttributeChanged);
    AbilitySystemComponent->GetGameplayAttributeValueChangeDelegate(BaseAS->GetSkillPointAttribute()).AddUObject(this, &UUI_HUDController::OnSkillPointAttributeChanged);
    // 차후 위 함수를 모든 스탯에 대해 반복 해야함~
}

// --- 어트리뷰트 변경 콜백 멤버 함수 구현 ---
// 기존 AddLambda 내부 로직을 각 함수로 분리하여 가독성과 안전성을 동시에 확보합니다.

void UUI_HUDController::OnLevelAttributeChanged(const FOnAttributeChangeData& Data)
{
    BroadcastLVChanges(Data.NewValue);
}

void UUI_HUDController::OnXPAttributeChanged(const FOnAttributeChangeData& Data)
{
    const float CurrentMaxXP = CastChecked<UBaseAttributeSet>(AttributeSet)->GetMaxXP();
    BroadcastXPChanges(Data.NewValue, CurrentMaxXP);
}

void UUI_HUDController::OnHealthAttributeChanged(const FOnAttributeChangeData& Data)
{
    // UE_LOG(LogTemp, Log, TEXT("[Delegate] HP 변경 감지됨: %f"), Data.NewValue);
    const float CurrentMaxHP = CastChecked<UBaseAttributeSet>(AttributeSet)->GetMaxHealth();
    BroadcastHPChanges(Data.NewValue, CurrentMaxHP);
}

void UUI_HUDController::OnMaxHealthAttributeChanged(const FOnAttributeChangeData& Data)
{
    const float CurrentHP = CastChecked<UBaseAttributeSet>(AttributeSet)->GetHealth();
    BroadcastHPChanges(CurrentHP, Data.NewValue);
}

void UUI_HUDController::OnStaminaAttributeChanged(const FOnAttributeChangeData& Data)
{
    const float CurrentMaxMP = CastChecked<UBaseAttributeSet>(AttributeSet)->GetMaxStamina();
    BroadcastStaminaChanges(Data.NewValue, CurrentMaxMP);
}

void UUI_HUDController::OnMaxStaminaAttributeChanged(const FOnAttributeChangeData& Data)
{
    const float CurrentMP = CastChecked<UBaseAttributeSet>(AttributeSet)->GetStamina();
    BroadcastStaminaChanges(CurrentMP, Data.NewValue);
}

void UUI_HUDController::OnAttackPowerAttributeChanged(const FOnAttributeChangeData& Data)
{
    BroadcastATKChanges(Data.NewValue);
}

void UUI_HUDController::OnAttackSpeedAttributeChanged(const FOnAttributeChangeData& Data)
{
    BroadcastASChanges(Data.NewValue);
}

void UUI_HUDController::OnDefenseAttributeChanged(const FOnAttributeChangeData& Data)
{
    BroadcastDEFChanges(Data.NewValue);
}

void UUI_HUDController::OnSkillAmpAttributeChanged(const FOnAttributeChangeData& Data)
{
    BroadcastSPChanges(Data.NewValue);
}

void UUI_HUDController::OnCriticalChanceAttributeChanged(const FOnAttributeChangeData& Data)
{
    BroadcastCCChanges(Data.NewValue);
}

void UUI_HUDController::OnMoveSpeedAttributeChanged(const FOnAttributeChangeData& Data)
{
    BroadcastSpeedChanges(Data.NewValue);
}

void UUI_HUDController::OnCooldownReductionAttributeChanged(const FOnAttributeChangeData& Data)
{
    BroadcastCooldownReduction(Data.NewValue);
}

void UUI_HUDController::OnAttackRangeAttributeChanged(const FOnAttributeChangeData& Data)
{
    BroadcastARChanges(Data.NewValue);
}

void UUI_HUDController::OnSkillPointAttributeChanged(const FOnAttributeChangeData& Data)
{
    BroadcastSkillPointChanges(Data.NewValue);
}

void UUI_HUDController::BroadcastLVChanges(float CurrentLV)
{
    if (IsValid(MainHUDWidget))
    {
        MainHUDWidget->Update_LV(CurrentLV);
    }
    else
    {
        UE_LOG(LogTemp, Warning, TEXT("[Broadcast] 실패: MainHUDWidget이 유효하지 않음!"));
    }
}

// 실제 체력 변화 브로드캐스트
void UUI_HUDController::BroadcastHPChanges(float CurrentHP, float MaxHP)
{
    if (IsValid(MainHUDWidget))
    {
        // UE_LOG(LogTemp, Warning, TEXT("[Broadcast] 위젯으로 전송 중: HP(%f), MaxHP(%f)"), CurrentHP, MaxHP);
        MainHUDWidget->Update_HP(CurrentHP, MaxHP);
    }
    else
    {
        UE_LOG(LogTemp, Warning, TEXT("[Broadcast] 실패: MainHUDWidget이 유효하지 않음!"));
    }
}

void UUI_HUDController::BroadcastStaminaChanges(float CurrentST, float MaxST)
{
    if (IsValid(MainHUDWidget))
    {
        MainHUDWidget->UPdate_MP(CurrentST, MaxST);
    }
    else
    {
        UE_LOG(LogTemp, Warning, TEXT("[Broadcast] 실패: MainHUDWidget이 유효하지 않음!"));
    }
}

void UUI_HUDController::BroadcastXPChanges(float CurrentXP, float MaxXP)
{
    if (IsValid(MainHUDWidget))
    {
        MainHUDWidget->Update_XP(CurrentXP, MaxXP);
    }
    else
    {
        UE_LOG(LogTemp, Warning, TEXT("[Broadcast] 실패: MainHUDWidget이 유효하지 않음!"));
    }
}

void UUI_HUDController::BroadcastATKChanges(float CurrentATK)
{    
    if (IsValid(MainHUDWidget))
    {
        MainHUDWidget->setStat(ECharacterStat::AD, CurrentATK);
    }
    else
    {
        UE_LOG(LogTemp, Warning, TEXT("[Broadcast] 실패: MainHUDWidget이 유효하지 않음!"));
    }
}

void UUI_HUDController::BroadcastSPChanges(float CurrentSP)
{
    if (IsValid(MainHUDWidget))
    {
        MainHUDWidget->setStat(ECharacterStat::AP, CurrentSP);
    }
    else
    {
        UE_LOG(LogTemp, Warning, TEXT("[Broadcast] 실패: MainHUDWidget이 유효하지 않음!"));
    }
}

void UUI_HUDController::BroadcastASChanges(float CurrentAS)
{
    if (IsValid(MainHUDWidget))
    {
        MainHUDWidget->setStat(ECharacterStat::AS, CurrentAS);
    }
    else
    {
        UE_LOG(LogTemp, Warning, TEXT("[Broadcast] 실패: MainHUDWidget이 유효하지 않음!"));
    }
}

void UUI_HUDController::BroadcastARChanges(float CurrentAR)
{
    if (IsValid(MainHUDWidget))
    {
        MainHUDWidget->setStat(ECharacterStat::ATRAN, CurrentAR);
    }
    else
    {
        UE_LOG(LogTemp, Warning, TEXT("[Broadcast] 실패: MainHUDWidget이 유효하지 않음!"));
    }
}

void UUI_HUDController::BroadcastCCChanges(float CurrentCC)
{
    if (IsValid(MainHUDWidget))
    {
        MainHUDWidget->setStat(ECharacterStat::CC, CurrentCC);
    }
    else
    {
        UE_LOG(LogTemp, Warning, TEXT("[Broadcast] 실패: MainHUDWidget이 유효하지 않음!"));
    }
}

void UUI_HUDController::BroadcastDEFChanges(float CurrentDEF)
{
    if (IsValid(MainHUDWidget))
    {
        MainHUDWidget->setStat(ECharacterStat::DEF, CurrentDEF);
    }
    else
    {
        UE_LOG(LogTemp, Warning, TEXT("[Broadcast] 실패: MainHUDWidget이 유효하지 않음!"));
    }
}

void UUI_HUDController::BroadcastSpeedChanges(float CurrentSpeed)
{
    if (IsValid(MainHUDWidget))
    {
        MainHUDWidget->setStat(ECharacterStat::SPD, CurrentSpeed);
    }
    else
    {
        UE_LOG(LogTemp, Warning, TEXT("[Broadcast] 실패: MainHUDWidget이 유효하지 않음!"));
    }
}
void UUI_HUDController::BroadcastCooldownReduction(float Cooldown)
{
    if (IsValid(MainHUDWidget))
    {
        MainHUDWidget->setStat(ECharacterStat::COOL, Cooldown);
    }
    else
    {
        UE_LOG(LogTemp, Warning, TEXT("[Broadcast] 실패: MainHUDWidget이 유효하지 않음!"));
    }
}

void UUI_HUDController::BroadcastSkillPointChanges(float _SkillPoint)
{
    if (IsValid(MainHUDWidget))
    {
        MainHUDWidget->UpdateSkillPoint(_SkillPoint);
    }
    else
    {
        UE_LOG(LogTemp, Warning, TEXT("[Broadcast] 실패: MainHUDWidget이 유효하지 않음!"));
    }
}
