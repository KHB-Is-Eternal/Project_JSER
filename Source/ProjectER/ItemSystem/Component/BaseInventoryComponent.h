// File: 5th_6th-Team6-CH6-Project/Source/ProjectER/ItemSystem/Component/BaseInventoryComponent.h

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "GameplayTagContainer.h"
#include "ItemSystem/Data/UsableItemData.h"
#include "ActiveGameplayEffectHandle.h"
#include "GameplayEffectTypes.h"
#include "BaseInventoryComponent.generated.h"

class UAbilitySystemComponent;
class UBaseItemData;
class ABaseItemActor;

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnInventoryUpdatedSignature);

// 시작 지급 아이템 1종 + 개수
USTRUCT(BlueprintType)
struct FStartingItemEntry
{
	GENERATED_BODY()

	// 지급할 아이템
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Inventory|Startup")
	TObjectPtr<UBaseItemData> Item = nullptr;

	// 지급 개수 (슬롯 스택 초과 시 다음 빈 슬롯으로 분산)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Inventory|Startup", meta = (ClampMin = "1"))
	int32 Count = 1;
};

UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class PROJECTER_API UBaseInventoryComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UBaseInventoryComponent();
	bool CanUseItemsInCurrentLifeState() const;

	// 아이템 추가 (서버에서만 실행되도록 내부 로직 수정)
	UFUNCTION(BlueprintCallable, Category = "Inventory")
	bool AddItem(UBaseItemData* InData);

	// 클라이언트가 호출하는 서버 요청용 RPC
	UFUNCTION(Server, Reliable, WithValidation)
	void Server_AddItem(UBaseItemData* InData);

	// 아이템 사용
	UFUNCTION(BlueprintCallable, Category = "Inventory")
	void UseItem(int32 SlotIndex);

	UFUNCTION(Server, Reliable, WithValidation)
	void Server_UseItem(int32 SlotIndex);

	// [김현수 추가분] 지정한 월드 위치에 와드 배치 (서버 권한). 컨트롤러의 좌클릭 배치 흐름에서 호출.
	void PlaceWardAtLocation(int32 SlotIndex, const FVector& TargetLocation);

	// 인벤토리 정보 가져오기
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Inventory")
	int32 GetInventoryCount() const;

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Inventory")
	UBaseItemData* GetItemAt(int32 SlotIndex) const;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Inventory")
	int32 MaxSlots = 8;

	// 스폰 시 서버에서 자동 지급할 시작 아이템 목록 (아이템 + 개수, 에디터에서 지정)
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Inventory|Startup")
	TArray<FStartingItemEntry> StartingItems;

	// 멀티플레이어 동기화를 위해 Replicated 추가
	UPROPERTY(ReplicatedUsing = OnRep_InventoryContents, VisibleAnywhere, BlueprintReadOnly, Category = "Inventory")
	TArray<UBaseItemData*> InventoryContents;

protected:
	UPROPERTY(ReplicatedUsing = OnRep_InventoryContents, VisibleAnywhere, BlueprintReadOnly, Category = "Inventory")
	TArray<int32> InventoryStackCounts;

	UFUNCTION()
	void OnRep_InventoryContents();

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:
	struct FPendingFoodHealEffect
	{
		FString ItemName;
		float TotalHealAmount = 0.0f;
		float DurationSeconds = 0.0f;
		float TickInterval = 1.0f;
	};

	UAbilitySystemComponent* ResolveOwnerAbilitySystemComponent() const;
	FGameplayTag GetSetByCallerTagFromStatType(EItemStatType StatType) const;

	bool ApplyItemEffect(UUsableItemData* ItemData);
	bool ApplyStatIncrease(UAbilitySystemComponent* ASC, UUsableItemData* ItemData);
	// [김현수 추가분] bUseTargetLocation=true면 TargetLocation에, 아니면 기존 소유자 앞 방향에 스폰.
	bool ApplyPlaceWard(UAbilitySystemComponent* ASC, UUsableItemData* ItemData, bool bUseTargetLocation = false, const FVector& TargetLocation = FVector::ZeroVector);
	// [김현수 추가분] 사용 성공 후 슬롯 스택 1 소모 (UseItem / PlaceWardAtLocation 공용)
	void ConsumeUsedItem(int32 SlotIndex, UUsableItemData* UsableItem);
	bool EnqueueFoodHeal(UUsableItemData* ItemData);
	void StartNextFoodHealEffect();
	void EnsureInventoryArraysValid();

	FActiveGameplayEffectHandle ActiveFoodGEHandle;
	TArray<FPendingFoodHealEffect> PendingFoodHealQueue;

	void OnFoodGERemoved(const FGameplayEffectRemovalInfo& RemovalInfo);

	struct FPendingDrinkManaEffect
	{
		FString ItemName;
		float TotalManaAmount = 0.0f;
		float DurationSeconds = 0.0f;
		float TickInterval = 1.0f;
	};

	FActiveGameplayEffectHandle ActiveDrinkManaGEHandle;
	TArray<FPendingDrinkManaEffect> PendingDrinkManaQueue;

	bool EnqueueDrinkMana(UUsableItemData* ItemData);
	void StartNextDrinkManaEffect();
	
	void OnManaGERemoved(const FGameplayEffectRemovalInfo& RemovalInfo);
	void ShowRecoveryFloatingText(float Amount, bool bIsMana);

public:
	UPROPERTY(BlueprintAssignable, Category = "Inventory")
	FOnInventoryUpdatedSignature OnInventoryUpdated;

	void ClearFoodHealEffects();
	void ClearDrinkManaEffects();

	// 슬롯 이동 / 교환
	UFUNCTION(BlueprintCallable, Category = "Inventory")
	bool SwapSlots(int32 FromIndex, int32 ToIndex);

	UFUNCTION(Server, Reliable, WithValidation)
	void Server_SwapSlots(int32 FromIndex, int32 ToIndex);

	// 슬롯 아이템을 월드에 떨어뜨리기
	bool DropItemFromSlot(int32 SlotIndex, const FVector& SpawnLocation, TSubclassOf<ABaseItemActor> ItemActorClass, APawn* DropperPawn);

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Inventory")
	int32 GetStackCountAt(int32 SlotIndex) const;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Inventory|Stack")
	int32 MaxStackPerSlot = 5;

	// 아이템 조합용: 특정 슬롯의 아이템 1개 소모
	UFUNCTION(BlueprintCallable, Category = "Inventory|Crafting")
	bool ConsumeItemAtSlot(int32 SlotIndex);

	// 아이템 조합용: 특정 슬롯에 아이템 추가
	UFUNCTION(BlueprintCallable, Category = "Inventory|Crafting")
	bool AddItemToSlot(int32 SlotIndex, UBaseItemData* Item);

private:
	// 각 아이템별 마지막 사용 시간을 저장하는 맵 (쿨타임 관리용)
	UPROPERTY()
	TMap<UUsableItemData*, float> LastItemUseTimes;
};