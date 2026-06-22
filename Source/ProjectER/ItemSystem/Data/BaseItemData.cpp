#include "ItemSystem/Data/BaseItemData.h"

UBaseItemData::UBaseItemData()
{
    // 생성자
}

FPrimaryAssetId UBaseItemData::GetPrimaryAssetId() const
{
    return FPrimaryAssetId("Items", GetFName());
}

FLinearColor UBaseItemData::GetRarityColor() const
{
    switch (ItemRarity)
    {
    case EItemRarity::Rare:
        return FLinearColor(0.4f, 0.1f, 0.8f, 1.0f); // 보라색빛
    case EItemRarity::Unique:
        return FLinearColor(0.9f, 0.3f, 0.6f, 1.0f); // 분홍색빛
    case EItemRarity::Normal:
    default:
        return FLinearColor(0.15f, 0.15f, 0.15f, 1.0f); // 짙은 회색 기본값
    }
}