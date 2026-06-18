#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "AbilitySystemInterface.h"
#include "GameplayTagContainer.h"
#include "CharacterSystem/Interface/TargetableInterface.h"
#include "Monster/Data/MonsterTags.h"
#include "BaseMonster.generated.h"

class UGameplayAbility;
class UStateTreeComponent;
class USphereComponent;
class UBoxComponent;
class UMonsterRangeComponent;
class UWidgetComponent;
class UUserWidget;
class UBaseMonsterAttributeSet;
class UGameplayEffect;
class ABaseCharacter;
class UMonsterDataAsset;
class ULootableComponent;
struct FOnAttributeChangeData;



UCLASS()
class PROJECTER_API ABaseMonster : public ACharacter, public IAbilitySystemInterface, public ITargetableInterface
{
	GENERATED_BODY()

public:

	ABaseMonster();

	virtual UAbilitySystemComponent* GetAbilitySystemComponent() const override;
	
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	//UStateTreeComponent* GetStateTreeComponent();
	void SetTargetPlayer(AActor* Target);
	void SetbIsCombat(bool Target);
	void SetbIsDead(bool Target);
	void SetIsFirstAttack(bool bIsFirst);
	void SetAttackCount(uint8 Count);

	AActor* GetTargetPlayer() const;
	bool GetbIsCombat() const;
	bool GetbIsDead() const;
	bool GetIsFirstAttack() const;
	uint8 GetAttackCount() const;
	FVector GetStartLocation() const;
	FRotator GetStartRotator() const;
	UMonsterRangeComponent* GetMonsterRangeComp() const;
	FMonsterTags GetMonsterTags() const;
	FPrimaryAssetId GetMonsterId() const;
	UBaseMonsterAttributeSet* GetAttributeSet() const;
	

	UFUNCTION(NetMulticast, Reliable)
	void Multicast_SetCollisionProfileName(FName ProfileName);

	void Death();

	
protected:

	virtual void PossessedBy(AController* newController) override;

	virtual void BeginPlay() override;
	

private:
	// 이동 속도 변경값 적용
	UFUNCTION()
	void OnMoveSpeedChangedHandle(float OldSpeed, float NewSpeed);

	void RewardMonsterXP(AActor* Player, FGameplayTag Tag, float Amount);


#pragma region ASC

private:

	//UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "GAS", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<class UProjectERASC> ASC;

	//UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "GAS", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UBaseMonsterAttributeSet> AttributeSet;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "GAS", meta = (AllowPrivateAccess = "true"))
	FMonsterTags MonsterTags;

#pragma endregion


#pragma region StateTree

private:

	//UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "StateTree", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UStateTreeComponent> StateTreeComp;

public:

	void SendStateTreeEvent(FGameplayTag InputTag);

	void SendAttackRangeEvent(float AttackRange);

	UFUNCTION()
	void MonsterGroupHitCall(AActor* Target);

	UFUNCTION()
	void SendHitEvent(AActor* Target);

	UFUNCTION()
	void SendDeathEvent(AActor* Target);

	UFUNCTION()
	void SendBeginSearchEvent();

	UFUNCTION()
	void SendEndSearchEvent();

	UFUNCTION()
	void SendTargetOffEvent();

#pragma endregion


#pragma region InitMonsterData

public:
	
	UPROPERTY(BlueprintReadOnly, Category = "MonsterData")
	TObjectPtr<UMonsterDataAsset> MonsterData;

private:
	UPROPERTY(ReplicatedUsing = OnRep_MonsterID)
	FPrimaryAssetId MonsterID;

	UPROPERTY(Replicated)
	float MonsterLevel;

public:

	UFUNCTION(BlueprintImplementableEvent, Category = "Monster|Event")
	void OnMonsterDataLoadedEvent(FPrimaryAssetId MonsterAssetId, float Level);

	void InitMonsterData(FPrimaryAssetId MonsterAssetId, float Level);
	
private:

	void InitMonsterDataLoading(FPrimaryAssetId MonsterAssetId, float Level);

	void OnMonsterDataLoaded(FPrimaryAssetId LoadedId, float Level);

	void InitGiveAbilities();

	void InitAttributes(float Level);

	void InitVisuals();

	void InitCollision();

	void InitStateTree();

	void InitHPBar();

#pragma endregion


#pragma region CooldownTag

private:

	TMap<FGameplayTag, FTimerHandle> CooldownTimerMap;

public:

	void OnCooldown(FGameplayTag CooldownTag, float Cooldown);

private:

	void AddCooldownTag(FGameplayTag CooldownTag);

	void RemoveCooldownTag(FGameplayTag CooldownTag);

#pragma endregion


#pragma region HPWidget

private:

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "UI", meta = (AllowprivateAccess = "true"))
	TObjectPtr<UWidgetComponent> HPBarWidgetComp;

	UFUNCTION()
	void OnHealthChangedHandle(float CurrentHP, float MaxHP);

#pragma endregion


#pragma region Item

public:
	UPROPERTY(VisibleAnywhere)
	TObjectPtr<ULootableComponent> LootableComp;

#pragma endregion


#pragma region OnRep

private:

	UFUNCTION()
	void OnRep_IsCombat();

	UFUNCTION()
	void OnRep_IsDead();

	UFUNCTION()
	void OnRep_MonsterID();

	UFUNCTION()
	void OnRep_TeamID();

#pragma endregion


#pragma region TargetableInterface

private:

	UPROPERTY(ReplicatedUsing = OnRep_TeamID, EditAnywhere, BlueprintReadWrite, Category = "Team", meta = (AllowprivateAccess = "true"))
	ETeamType TeamID;

public:

	virtual ETeamType GetTeamType() const override;

	virtual bool IsTargetable() const override;

	virtual void HighlightActor(bool bIsHighlight, int32 StencilValue = 0) override;

private:

	UFUNCTION(BlueprintCallable, Server, Reliable)
	void Server_SetTeamID(ETeamType NewTeamID);

#pragma endregion


#pragma region TargetableInterface

private:

	int32 SpawnPoint = 0;

public:

	void SetSpawnPoint(int32 point) { SpawnPoint = point; }

	int32 GetSpawnPoint() { return SpawnPoint; }

#pragma endregion


#pragma region CC

private:
	// Test용
	void OnCCChanged(FGameplayTag Tag, int32 NewCount);

public:

	void OffCCChanged();

#pragma endregion




private:

	//UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "StateTree", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UMonsterRangeComponent> MonsterRangeComp;

	//UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Collision", meta = (AllowprivateAccess = "true"))
	TObjectPtr<UBoxComponent> HitBoxComp;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Sound", meta = (AllowprivateAccess = "true"))
	TObjectPtr<UAudioComponent> SoundComp;

	//UPROPERTY(BlueprintReadOnly, Category = "StateTree", meta = (AllowPrivateAccess = "true"))
	FVector StartLocation;

	//UPROPERTY(BlueprintReadOnly, Category = "StateTree", meta = (AllowPrivateAccess = "true"))
	FRotator StartRotator;

	//UPROPERTY(BlueprintReadWrite, Category = "StateTree", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<AActor> TargetPlayer;

	UPROPERTY(ReplicatedUsing = OnRep_IsCombat, VisibleAnywhere, BlueprintReadWrite, Category = "StateTree", meta = (AllowPrivateAccess = "true"))
	bool bIsCombat;

	UPROPERTY(ReplicatedUsing = OnRep_IsDead, VisibleAnywhere, BlueprintReadWrite, Category = "StateTree", meta = (AllowPrivateAccess = "true"))
	bool bIsDead;

	bool bIsFirstAttack;

	uint8 AttackCount;





};



