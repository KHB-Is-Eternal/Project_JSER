#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Net/Serialization/FastArraySerializer.h"
#include "LineOfSight/VisionData.h"
#include "LineOfSight/Management/VisionGameStateComp.h" // FVisibleActorEntry
#include "VisionPlayerStateComp.generated.h"

class UVision_VisualComp;

TOPDOWNVISION_API DECLARE_LOG_CATEGORY_EXTERN(VisionPlayerStateComp, Log, All);

// -------------------------------------------------------------------------- //
//  Per-player visible actor array (COND_OwnerOnly)
//
//  [005 부록 A] 각 플레이어는 자기 팀이 볼 수 있는 항목만 수신한다.
//  서버의 VisionGameStateComp가 CanSeeTeam으로 걸러서 채워 준다.
// -------------------------------------------------------------------------- //

USTRUCT()
struct FPlayerVisibleActorArray : public FFastArraySerializer
{
    GENERATED_BODY()

    UPROPERTY()
    TArray<FVisibleActorEntry> Items;

    UPROPERTY()
    UVisionPlayerStateComp* OwnerComp = nullptr;

    void PostReplicatedAdd   (const TArrayView<int32>& AddedIndices,   int32 FinalSize);
    void PreReplicatedRemove (const TArrayView<int32>& RemovedIndices, int32 FinalSize);
    void PostReplicatedChange(const TArrayView<int32>& ChangedIndices, int32 FinalSize);

    bool NetDeltaSerialize(FNetDeltaSerializeInfo& DeltaParms)
    {
        return FFastArraySerializer::FastArrayDeltaSerialize<FVisibleActorEntry, FPlayerVisibleActorArray>(
            Items, DeltaParms, *this);
    }
};

template<>
struct TStructOpsTypeTraits<FPlayerVisibleActorArray> : public TStructOpsTypeTraitsBase2<FPlayerVisibleActorArray>
{
    enum { WithNetDeltaSerializer = true };
};

UCLASS(ClassGroup=(Vision), meta=(BlueprintSpawnableComponent))
class TOPDOWNVISION_API UVisionPlayerStateComp : public UActorComponent
{
    GENERATED_BODY()

public:
    UVisionPlayerStateComp();

protected:
    virtual void BeginPlay() override;
    virtual void GetLifetimeReplicatedProps(
        TArray<FLifetimeProperty>& OutLifetimeProps) const override;

public:
    UFUNCTION(BlueprintCallable, Category="Vision")
    void SetTeamChannel(EVisionChannel InTeam);

    UFUNCTION(BlueprintCallable, Category="Vision")
    EVisionChannel GetTeamChannel() const { return TeamChannel; }

    UFUNCTION(BlueprintCallable, Category="Vision")
    void SetAllReveal(bool bEnabled);

    UFUNCTION(BlueprintCallable, Category="Vision")
    bool IsAllReveal() const { return bAllReveal; }

    bool CanSeeTeam(EVisionChannel InTeam) const;

    void ReevaluateTargetVisibility(
        AActor* Target,
        EVisionChannel ExcludeObserverTeam = EVisionChannel::None);

    /** Pure query: would Target be visible to this local player right now?
     *  Same judgment order as ReevaluateTargetVisibility (AllReveal ->
     *  CanSeeTeam -> local vote map -> replicated GSComp state), but with
     *  no side effects — never calls SetVisible. */
    bool ComputeTargetVisibility(
        const AActor* Target,
        const UVision_VisualComp* VisualComp,
        EVisionChannel ExcludeObserverTeam = EVisionChannel::None) const;

    UFUNCTION(BlueprintCallable, Category="Vision")
    void RefreshVisibility();

    // --- Per-player visible entries (server writes, owner-only replication) --- //

    /** [서버 전용] VisionGameStateComp의 팬아웃/재구성 경로에서만 호출 */
    void AddTeamVisibleEntry(AActor* Target, EVisionChannel ObserverTeam);
    void RemoveTeamVisibleEntry(AActor* Target, EVisionChannel ObserverTeam);
    void ResetTeamVisibleEntries(const TArray<FVisibleActorEntry>& NewEntries);

    const TArray<FVisibleActorEntry>& GetTeamVisibleActors() const { return TeamVisibleActors.Items; }

    // --- FastArray callbacks (owning client) --- //
    void OnTeamEntryAdded(AActor* Target);
    void OnTeamEntryRemoved(AActor* Target, EVisionChannel Team);

private:
    UPROPERTY(ReplicatedUsing=OnRep_TeamChannel)
    EVisionChannel TeamChannel = EVisionChannel::None;

    UPROPERTY(ReplicatedUsing=OnRep_AllReveal)
    bool bAllReveal = false;

    // 이 플레이어가 볼 수 있는 항목만 담기는 배열 — 소유 커넥션에만 리플리케이트 (005 부록 A)
    UPROPERTY(Replicated)
    FPlayerVisibleActorArray TeamVisibleActors;

    UFUNCTION() void OnRep_TeamChannel();
    UFUNCTION() void OnRep_AllReveal();

    UFUNCTION()
    void OnPawnSet(APlayerState* PlayerState, APawn* NewPawn, APawn* OldPawn);

    /** 이 PS의 소유 컨트롤러가 로컬 컨트롤러인가 (리슨 호스트/스탠드얼론의 즉시 반영 판정).
     *  GetLocalVisionPS와의 포인터 비교는 심리스 트래블의 PS 교체 창에서 옛 PS를
     *  돌려받아 어긋날 수 있으므로, 소유 관계를 직접 묻는다. */
    bool IsOwnedByLocalController() const;

    void InitializeSameTeamEvaluators();

    /** Pushes TeamChannel onto the owning pawn's Vision_VisualComp and
     *  re-registers it with the subsystem. Players have VisionChannel=None
     *  at Initialize() time because the team replicates later — this call
     *  fixes that as soon as the team is known. */
    void SyncPawnVisionChannel();
};