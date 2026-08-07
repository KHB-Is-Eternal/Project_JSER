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

FLinearColor UBaseItemData::GetRarityTextColor() const
{
    switch (ItemRarity)
    {
    case EItemRarity::Rare:
        return FLinearColor(0.45f, 0.2f, 0.65f, 1.0f); // 채도 낮춘 진한 보라
    case EItemRarity::Unique:
        return FLinearColor(0.7f, 0.25f, 0.45f, 1.0f); // 채도 낮춘 자주색
    case EItemRarity::Normal:
    default:
        return FLinearColor(0.0f, 0.0f, 0.0f, 1.0f); // 검은색 (폰트 기본값)
    }
}