#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "ItemSystem/Actor/BaseBoxActor.h"
#include "ItemSystem/Interface/I_ItemInteractable.h"
#include "ItemSystem/Data/BaseItemData.h"
#include "LootableComponent.generated.h"

class UBaseItemData;

// enum for looting reaction
UENUM(BlueprintType)
enum class ELootReactionType : uint8
{
	None =255 UMETA(DisplayName="None"),

	LootSuccess =0 UMETA(DisplayName="LootSuccess"),
	LootFail =1 UMETA(DisplayName="LootFail"),
};


//Delegate to call bp exposed reaction
DECLARE_DYNAMIC_DELEGATE_TwoParams(FOnLootableItemClicked, UBaseItemData*, ItemData, bool, bDidSuccess);// loot reaction

//TODO -> need multi player case sync for reaction

UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class PROJECTER_API ULootableComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	ULootableComponent();

protected:
	virtual void BeginPlay() override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	virtual void PickupItem();


public:

	//========= Delegate Interaction =======//

	/*UPROPERTY(BlueprintAssignable, Category="Lootable|BPInteraction")
	FOnLootableItemClicked OnLootableItemClicked;*/

	
	// ========================================
	// BaseBoxActor 호환 인터페이스
	// ========================================

	/**
	 * 현재 루트 슬롯 리스트 가져오기
	 */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Lootable")
	TArray<FLootSlot> GetCurrentItemList() const { return CurrentItemList; }

	/**
	 * 특정 슬롯의 아이템 데이터 가져오기
	 */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Lootable")
	UBaseItemData* GetItemData(int32 SlotIndex) const;

	/**
	 * 슬롯 개수 감소 (BasePlayerController에서 호출)
	 */
	UFUNCTION(BlueprintCallable, Category = "Lootable")
	void ReduceItem(int32 SlotIndex);

	// ========================================
	// 루팅 초기화
	// ========================================

	/**
	 * 랜덤 아이템으로 루트 테이블 생성 (균등 확률, DropItemPool 미설정 시 fallback)
	 */
	UFUNCTION(BlueprintCallable, Category = "Lootable")
	void InitializeRandomLoot();

	/**
	 * 가중치/등급확률/등급캡 기반 루트 생성 (몬스터 가챠와 동일 로직)
	 * DropItemPool 이 세팅돼 있을 때 사용. 내부적으로 InitializeWithItems 로 결과 주입.
	 */
	UFUNCTION(BlueprintCallable, Category = "Lootable")
	void InitializeWeightedLoot();

	/**
	 * [김현수 추가분] 몬스터 가챠와 LootableComponent 가 공유하는 가중치 드랍 생성기.
	 * 등급확률(정규화) → 등급별 캡/하위강등 → 등급 내 가중치 경쟁 순으로 굴린다.
	 */
	static TArray<UBaseItemData*> GenerateWeightedDrops(
		const TArray<FDropItemInfo>& InDropPool,
		const TMap<EItemRarity, float>& InRarityDropRates,
		const TMap<EItemRarity, int32>& InMaxRarityDropCounts,
		int32 InMinDropCount,
		int32 InMaxDropCount);

	/**
	 * 특정 아이템 리스트로 루트 테이블 생성
	 */
	UFUNCTION(BlueprintCallable, Category = "Lootable")
	void InitializeWithItems(const TArray<UBaseItemData*>& Items);

	/**
	 * 루트 테이블 초기화 (빈 상태로)
	 */
	UFUNCTION(BlueprintCallable, Category = "Lootable")
	void ClearLoot();

	/**
	 * 루트 가능한 아이템이 남아있는지 확인
	 */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Lootable")
	bool HasLootRemaining() const;

	// ========================================
	// 아이템 가져가기
	// ========================================

	/**
	 * 특정 슬롯의 아이템 가져가기
	 * @return 성공 여부
	 */
	UFUNCTION(BlueprintCallable, Category = "Lootable")
	bool TakeItem(int32 SlotIndex, class APawn* Taker);
	

	// ========================================
	// 델리게이트
	// ========================================

	/** 루트 테이블이 변경될 때 브로드캐스트 */
	DECLARE_MULTICAST_DELEGATE(FOnLootChanged);
	FOnLootChanged OnLootChanged;

	/** 모든 아이템이 루팅되었을 때 브로드캐스트 */
	DECLARE_MULTICAST_DELEGATE(FOnLootDepleted);
	FOnLootDepleted OnLootDepleted;

protected:
	/**
	 * 아이템 압축 정렬 (빈 슬롯을 뒤로)
	 */
	void CompactItemList();

	/**
	 * 리플리케이션 콜백
	 */
	UFUNCTION()
	void OnRep_CurrentItemList();

	UFUNCTION()
	void OnRep_ItemPool();


public:
	// ========================================
	// 사운드
	// ========================================

	/** 상자 등이 열릴 때 로컬에서만 재생되는 사운드 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Lootable|Sound")
	TSoftObjectPtr<class USoundBase> OpenSound;

	/** 상호작용 시 사운드 로컬 재생 (클라이언트에서 호출) */
	UFUNCTION(BlueprintCallable, Category = "Lootable|Sound")
	void PlayOpenSoundLocally(const UObject* WorldContextObject);

	// ========================================
	// 설정 가능한 프로퍼티
	// ========================================

	/** 루팅 가능한 아이템 풀 (균등 랜덤용) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, ReplicatedUsing = OnRep_ItemPool, Category = "Lootable|Setup")
	TArray<TObjectPtr<UBaseItemData>> ItemPool;

	// ========================================
	// [김현수 추가분] 가중치 드랍 설정 (몬스터 데이터에셋과 동일 구조)
	// DropItemPool 이 하나라도 세팅되면 균등 랜덤 대신 가중치 생성이 사용된다.
	// ========================================

	/** 가중치 드랍 풀 (아이템 + 등급 내 가중치) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Lootable|Weighted")
	TArray<FDropItemInfo> DropItemPool;

	/** 등급별 드랍 확률 (합이 100%가 아니어도 내부에서 정규화) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Lootable|Weighted")
	TMap<EItemRarity, float> RarityDropRates;

	/** 등급별 최대 드랍 개수 제한 (0이거나 미설정이면 무제한) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Lootable|Weighted")
	TMap<EItemRarity, int32> MaxRarityDropCounts;

	/** 최대 슬롯 개수 (기본 10칸) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Lootable|Setup")
	int32 MaxSlots = 10;

	/** 최소 드롭 아이템 개수 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Lootable|Setup")
	int32 MinLootCount = 1;

	/** 최대 드롭 아이템 개수 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Lootable|Setup")
	int32 MaxLootCount = 3;

	/** 자동 초기화 여부 (BeginPlay 시 InitializeRandomLoot 자동 호출) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Lootable|Setup")
	bool bAutoInitialize = false;

	/** 루팅 완료 시 오너 액터 자동 삭제 여부 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Replicated, Category = "Lootable|Behavior")
	bool bDestroyOwnerWhenEmpty = false;

	UFUNCTION(BlueprintCallable, Category = "Lootable")
	void InitializeWithItemStacks(const TArray<UBaseItemData*>& Items, const TArray<int32>& Counts);

protected:
	/** 현재 루트 슬롯 리스트 (리플리케이션) */
	UPROPERTY(ReplicatedUsing = OnRep_CurrentItemList, VisibleAnywhere, BlueprintReadOnly, Category = "Lootable|Runtime")
	TArray<FLootSlot> CurrentItemList;
};