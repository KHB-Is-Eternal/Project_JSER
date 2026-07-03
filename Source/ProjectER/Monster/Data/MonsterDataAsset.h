#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "GameplayTagContainer.h"
#include "StateTree.h"
#include "ItemSystem/Data/BaseItemData.h"
#include "MonsterDataAsset.generated.h"

class UGameplayAbility;
class UBaseItemData;
class USkillDataAsset;
class UNiagaraSystem;
struct FGameplayTag;


UENUM(BlueprintType)
enum class EMonsterActionType : uint8
{
	Idle,
	Alert,
	Move,
	Death,
	NormalAttack,
	QSkill,
	WSkill,
	ESkill,
	RSkill,
	FlyStart,
	FlyAttack,
	FlyEnd,
	None
};

UENUM(BlueprintType)
enum class ENiagaraAttachType : uint8
{
	Mine,
	Target,
	None
};

UENUM(BlueprintType)
enum class ENiagaraSpawnPositionType : uint8
{
	TargetPosition,
	TargetDirection,
	None
};

USTRUCT(BlueprintType)
struct FMonsterNiagaraData
{
	GENERATED_BODY()

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	TObjectPtr<UNiagaraSystem> NiagaraSystem;



	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	FVector PositionOffset = FVector(0,0,0);

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	FRotator RotationOffset = FRotator(0,0,0);

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	bool bFollow = false;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, meta = (EditCondition = "bFollow", EditConditionHides))
	ENiagaraAttachType AttachType = ENiagaraAttachType::Mine;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, meta = (EditCondition = "bFollow", EditConditionHides))
	FName AttachSocket = NAME_None;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, meta = (EditCondition = "!bFollow", EditConditionHides))
	ENiagaraSpawnPositionType SpawnType = ENiagaraSpawnPositionType::TargetPosition;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, meta = (EditCondition = "!bFollow", EditConditionHides))
	FVector Scale = FVector(1.f);

};

USTRUCT(BlueprintType)
struct FMonsterSoundData
{
	GENERATED_BODY()

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	TObjectPtr<USoundBase> TestSound;
};

USTRUCT(BlueprintType)
struct FMonsterDecalData
{
	GENERATED_BODY()

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	TObjectPtr<UMaterialInterface> DecalMeterial;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	FVector DecalScale = FVector(0, 0, 0);

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	FVector PositionOffset = FVector(0, 0, 0);

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	FRotator RotationOffset = FRotator(0, 0, 0);
};


// [김현수 추가분] 개별 아이템 픽업(확률 조정)을 위한 구조체
USTRUCT(BlueprintType)
struct FDropItemInfo
{
	GENERATED_BODY()

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	TObjectPtr<UBaseItemData> Item;

	// 기본 가중치 1.0f. 숫자가 클수록 동일 등급 내에서 더 자주 나옴. 0.0f면 등장 안 함.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, meta=(ClampMin=0.0f))
	float Weight = 1.0f;
};

// 몬스터 데이터
UCLASS()
class PROJECTER_API UMonsterDataAsset : public UPrimaryDataAsset
{
	GENERATED_BODY()
	
public:
	virtual FPrimaryAssetId GetPrimaryAssetId() const override;

	
public:

	UPROPERTY(EditDefaultsOnly, Category = "MonsterData|Stat")
	FName TableRowName; // 해당 몬스터 Row 이름

	UPROPERTY(EditDefaultsOnly, Category = "MonsterData|Stat", meta = (Categories = "Unit.AttackType"))
	FGameplayTag AttackType;

	UPROPERTY(EditDefaultsOnly, Category = "MonsterData|Stat")
	TObjectPtr<UDataTable> MonsterDataTable;

	UPROPERTY(EditDefaultsOnly, Category = "MonsterData|Stat")
	TObjectPtr<UCurveTable> MonsterCurveTable;

	UPROPERTY(EditDefaultsOnly, Category = "MonsterData|Stat")
	TObjectPtr<UStateTree> MonsterStateTree;;


	UPROPERTY(EditDefaultsOnly, Category = "MonsterData|GAS")
	TArray<TSubclassOf<UGameplayAbility>> DefaultAbilities;

	// 스킬 시스템이 완성되면 사용
	UPROPERTY(EditDefaultsOnly, Category = "MonsterData|GAS")
	TArray<TObjectPtr<USkillDataAsset>> SkillDataAssets;


	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "MonsterData|Montage")
	TMap<EMonsterActionType, TObjectPtr<UAnimMontage>> Montages;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "MonsterData|Effect")
	TMap<EMonsterActionType, FMonsterNiagaraData> Niagaras;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "MonsterData|Effect")
	TMap<EMonsterActionType, FMonsterSoundData> Sounds;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "MonsterData|Effect")
	TMap<EMonsterActionType, FMonsterDecalData> DecalMeterials;


	UPROPERTY(EditDefaultsOnly, Category = "MonsterData|Visual")
	TObjectPtr<USkeletalMesh> Mesh;

	UPROPERTY(EditDefaultsOnly, Category = "MonsterData|Visual")
	FVector MeshScale;

	UPROPERTY(EditDefaultsOnly, Category = "MonsterData|Visual")
	float CollisionRadius;

	UPROPERTY(EditDefaultsOnly, Category = "MonsterData|Visual")
	float CapsuleHalfHeight;

	UPROPERTY(EditDefaultsOnly, Category = "MonsterData|Visual")
	FVector HitBoxExtent;

	UPROPERTY(EditDefaultsOnly, Category = "MonsterData|Visual")
	TSubclassOf<UAnimInstance> Anim;



	UPROPERTY(EditDefaultsOnly, Category = "MonsterData|Reward")
	int Exp;

	// [김현수 추가분] 개별 몬스터 드랍 테이블 가챠 연동용 변수
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "MonsterData|Reward")
	TMap<EItemRarity, float> RarityDropRates;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "MonsterData|Reward")
	TArray<FDropItemInfo> DropItemPool;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "MonsterData|Reward")
	int32 MinDropCount = 1;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "MonsterData|Reward")
	int32 MaxDropCount = 3;

};
