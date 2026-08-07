#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Net/Serialization/FastArraySerializer.h"
#include "LineOfSight/VisionData.h"
#include "VisionGameStateComp.generated.h"

class UVision_VisualComp;
class UVisionPlayerStateComp;

TOPDOWNVISION_API DECLARE_LOG_CATEGORY_EXTERN(VisionGameStateComp, Log, All);

// -------------------------------------------------------------------------- //
//  Visible actor entry
// -------------------------------------------------------------------------- //

USTRUCT()
struct FVisibleActorEntry : public FFastArraySerializerItem
{
    GENERATED_BODY()

    UPROPERTY()
    AActor* Target = nullptr;

    UPROPERTY()
    EVisionChannel ObserverTeam = EVisionChannel::None;
};

// -------------------------------------------------------------------------- //
//  Component
//
//  [005 부록 A] 서버 권위 스토어. VisibleActors는 더 이상 전 클라이언트에
//  리플리케이트하지 않는다 — 대신 항목이 추가/제거될 때 CanSeeTeam을 만족하는
//  각 플레이어의 VisionPlayerStateComp(COND_OwnerOnly FastArray)로 팬아웃한다.
//  각 클라이언트는 자기 팀과 관련된 항목만 수신한다.
// -------------------------------------------------------------------------- //

UCLASS(ClassGroup=(Vision), meta=(BlueprintSpawnableComponent))
class TOPDOWNVISION_API UVisionGameStateComp : public UActorComponent
{
    GENERATED_BODY()

public:
    UVisionGameStateComp();

public:
    // --- Server API --- //

    /** Unified name used everywhere — was SetActorVisibleByTeam in some call sites. */
    UFUNCTION(BlueprintCallable, Category="Vision")
    void SetActorVisibleToTeam(AActor* Target, EVisionChannel Team);

    UFUNCTION(BlueprintCallable, Category="Vision")
    void ClearActorVisibleToTeam(AActor* Target, EVisionChannel Team);

    UFUNCTION(BlueprintCallable, Category="Vision")
    bool IsActorVisibleToTeam(AActor* Target, EVisionChannel Team) const;

    UFUNCTION(BlueprintCallable, Category="Vision")
    EVisionChannel GetLocalPlayerTeamChannel() const;

    /** [서버 전용] 플레이어 팀 확정/변경 시, 스토어에서 해당 플레이어가 볼 수 있는
     *  항목만 걸러 그 플레이어의 OwnerOnly 배열을 재구성한다. */
    void RebuildPlayerVisibleEntries(UVisionPlayerStateComp* VisionPS) const;

    const TArray<FVisibleActorEntry>& GetVisibleActors() const { return VisibleActors; }

private:
    /** 항목 추가/제거를 CanSeeTeam을 만족하는 모든 플레이어의 OwnerOnly 배열로 전파 */
    void PushEntryToEligiblePlayers(AActor* Target, EVisionChannel Team, bool bAdd) const;

    // 서버 권위 스토어 — 리플리케이트하지 않음 (005 부록 A: 팀 시야 전체 유출 차단)
    UPROPERTY()
    TArray<FVisibleActorEntry> VisibleActors;
};