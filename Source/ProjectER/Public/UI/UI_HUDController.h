// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "UObject/NoExportTypes.h"
#include "AbilitySystemComponent.h"
#include "AttributeSet.h"
#include "UI/UI_MainHUD.h"
#include "UI_HUDController.generated.h"

USTRUCT(BlueprintType)
struct FWidgetControllerParams
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TObjectPtr<APlayerController> PlayerController = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TObjectPtr<APlayerState> PlayerState = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TObjectPtr<UAbilitySystemComponent> AbilitySystemComponent = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TObjectPtr<UAttributeSet> AttributeSet = nullptr;
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnAttributeChangedSignature, float, NewValue);

UCLASS()
class PROJECTER_API UUI_HUDController : public UObject
{
	GENERATED_BODY()

public:
    UPROPERTY()
    TObjectPtr<UUI_MainHUD> MainHUDWidget;

    void BroadcastLVChanges(float CurrentLV);
    void BroadcastHPChanges(float CurrentHP, float MaxHP);
    void BroadcastStaminaChanges(float CurrentST, float MaxST);
    void BroadcastXPChanges(float CurrentXP, float MaxXP);
    void BroadcastATKChanges(float CurrentATK);
    void BroadcastSPChanges(float CurrentSP); // 스증
    void BroadcastASChanges(float CurrentAS);
    void BroadcastARChanges(float CurrentAR); // 사거리
    void BroadcastCCChanges(float CurrentCC); // 크리티컬 찬스 = CC
    void BroadcastDEFChanges(float CurrentDEF);
    void BroadcastSpeedChanges(float CurrentSpeed);
    void BroadcastCooldownReduction(float Cooldown);
    void BroadcastSkillPointChanges(float CurrentAR); // 스킬 포인트





    // 초기 설정!
    void SetParams(const FWidgetControllerParams& Params);

    // 바인딩하기
    virtual void BindCallbacksToDependencies();

    // 블루프린트용
    UPROPERTY(BlueprintAssignable, Category = "GAS|Attributes")
    FOnAttributeChangedSignature OnHealthChanged;

    UPROPERTY(BlueprintAssignable, Category = "GAS|Attributes")
    FOnAttributeChangedSignature OnMaxHealthChanged;

protected:
    // 데이터 보관용
    TObjectPtr<APlayerController> PlayerController;
    TObjectPtr<APlayerState> PlayerState;
    TObjectPtr<UAbilitySystemComponent> AbilitySystemComponent;
    TObjectPtr<UAttributeSet> AttributeSet;

    // AddUObject 어트리뷰트 변경 콜백 함수 선언
    // AddLambda([this]...) 대신 AddUObject(this, &...) 로 바인딩하여 댕글링 포인터 크래시 방지
    void OnLevelAttributeChanged(const FOnAttributeChangeData& Data);
    void OnXPAttributeChanged(const FOnAttributeChangeData& Data);
    void OnHealthAttributeChanged(const FOnAttributeChangeData& Data);
    void OnMaxHealthAttributeChanged(const FOnAttributeChangeData& Data);
    void OnStaminaAttributeChanged(const FOnAttributeChangeData& Data);
    void OnMaxStaminaAttributeChanged(const FOnAttributeChangeData& Data);
    void OnAttackPowerAttributeChanged(const FOnAttributeChangeData& Data);
    void OnAttackSpeedAttributeChanged(const FOnAttributeChangeData& Data);
    void OnDefenseAttributeChanged(const FOnAttributeChangeData& Data);
    void OnSkillAmpAttributeChanged(const FOnAttributeChangeData& Data);
    void OnCriticalChanceAttributeChanged(const FOnAttributeChangeData& Data);
    void OnMoveSpeedAttributeChanged(const FOnAttributeChangeData& Data);
    void OnCooldownReductionAttributeChanged(const FOnAttributeChangeData& Data);
    void OnAttackRangeAttributeChanged(const FOnAttributeChangeData& Data);
    void OnSkillPointAttributeChanged(const FOnAttributeChangeData& Data);
	
};
