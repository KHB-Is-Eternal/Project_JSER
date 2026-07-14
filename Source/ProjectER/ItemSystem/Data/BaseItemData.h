#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "BaseItemData.generated.h"

UENUM(BlueprintType)
enum class EItemPickupType : uint8
{
    Automatic    UMETA(DisplayName = "Automatic Pickup (Overlap)"),
    Interaction  UMETA(DisplayName = "Manual Pickup (Click)")
};

UENUM(BlueprintType)
enum class EItemCategory : uint8
{
    None         UMETA(DisplayName = "분류 없음 (None)"),
    Material     UMETA(DisplayName = "재료 아이템 (Material)"),
    Consumable   UMETA(DisplayName = "소비 아이템 (Consumable)"),
    Equipment    UMETA(DisplayName = "장비 아이템 (Equipment)")
};

UENUM(BlueprintType)
enum class EItemRarity : uint8
{
    Normal      UMETA(DisplayName = "일반 (Normal)"),
    Rare        UMETA(DisplayName = "희귀 (Rare)"),
    Unique      UMETA(DisplayName = "영웅/유니크 (Unique)")
};

UCLASS(BlueprintType)
class PROJECTER_API UBaseItemData : public UPrimaryDataAsset
{
    GENERATED_BODY()

public:
    UBaseItemData();

    virtual FPrimaryAssetId GetPrimaryAssetId() const override;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item|Info")
    FText ItemName;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item|Visual")
    TSoftObjectPtr<UStaticMesh> ItemMesh;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item|Info")
    TSoftObjectPtr<UTexture2D> ItemIcon;

    UFUNCTION(BlueprintCallable, Category = "Item|UI")
    FLinearColor GetRarityColor() const;

    UFUNCTION(BlueprintCallable, Category = "Item|UI")
    FLinearColor GetRarityTextColor() const;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item|Settings")
    EItemPickupType PickupType;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item|Settings")
    EItemCategory ItemCategory = EItemCategory::None;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item|Settings")
    EItemRarity ItemRarity = EItemRarity::Normal;

    // Item ToolTip
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item|Info")
    FText ItemShortDesc;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item|Info")
    FText ItemLongDesc;

};