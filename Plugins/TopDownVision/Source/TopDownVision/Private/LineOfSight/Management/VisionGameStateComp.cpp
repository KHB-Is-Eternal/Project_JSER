#include "LineOfSight/Management/VisionGameStateComp.h"

#include "GameFramework/PlayerState.h"
#include "GameFramework/GameStateBase.h"
#include "LineOfSight/Management/VisionPlayerStateComp.h"

DEFINE_LOG_CATEGORY(VisionGameStateComp);

// -------------------------------------------------------------------------- //
//  Component lifecycle
// -------------------------------------------------------------------------- //

UVisionGameStateComp::UVisionGameStateComp()
{
    PrimaryComponentTick.bCanEverTick = false;
    SetIsReplicatedByDefault(true);
}

// -------------------------------------------------------------------------- //
//  Server API
//
//  [005 부록 A] 이 스토어는 서버 전용이다. 클라이언트로의 전파는
//  PushEntryToEligiblePlayers가 각 플레이어의 VisionPlayerStateComp
//  (COND_OwnerOnly FastArray)에 항목을 써 넣는 방식으로만 일어난다.
// -------------------------------------------------------------------------- //

void UVisionGameStateComp::SetActorVisibleToTeam(AActor* Target, EVisionChannel Team)
{
    if (!Target)
    {
        UE_LOG(VisionGameStateComp, Warning,
            TEXT("SetActorVisibleToTeam >> Null target"));
        return;
    }

    // 서버 권위 — 클라이언트 호출(예: HandleProviderRegistered)은 무시.
    // 아군 가시성은 CanSeeTeam 단락으로 이미 보장되므로 클라 측 항목이 필요 없다.
    if (!GetOwner()->HasAuthority())
        return;

    if (IsActorVisibleToTeam(Target, Team))
        return;

    FVisibleActorEntry& Entry = VisibleActors.AddDefaulted_GetRef();
    Entry.Target       = Target;
    Entry.ObserverTeam = Team;

    PushEntryToEligiblePlayers(Target, Team, /*bAdd=*/true);

    UE_LOG(VisionGameStateComp, Verbose,
        TEXT("SetActorVisibleToTeam >> %s visible to team [%s]"),
        *Target->GetName(), *UEnum::GetValueAsString(Team));
}

void UVisionGameStateComp::ClearActorVisibleToTeam(AActor* Target, EVisionChannel Team)
{
    if (!Target)
    {
        UE_LOG(VisionGameStateComp, Warning,
            TEXT("ClearActorVisibleToTeam >> Null target"));
        return;
    }

    if (!GetOwner()->HasAuthority())
        return;

    for (int32 i = VisibleActors.Num() - 1; i >= 0; --i)
    {
        const FVisibleActorEntry& Entry = VisibleActors[i];
        if (Entry.Target != Target || Entry.ObserverTeam != Team)
            continue;

        VisibleActors.RemoveAt(i);

        PushEntryToEligiblePlayers(Target, Team, /*bAdd=*/false);

        UE_LOG(VisionGameStateComp, Verbose,
            TEXT("ClearActorVisibleToTeam >> %s hidden from team [%s]"),
            *Target->GetName(), *UEnum::GetValueAsString(Team));
        return;
    }

    UE_LOG(VisionGameStateComp, Verbose,
        TEXT("ClearActorVisibleToTeam >> %s was not visible to team [%s], nothing to clear"),
        *Target->GetName(), *UEnum::GetValueAsString(Team));
}

bool UVisionGameStateComp::IsActorVisibleToTeam(AActor* Target, EVisionChannel Team) const
{
    for (const FVisibleActorEntry& Entry : VisibleActors)
    {
        if (Entry.Target != Target)
            continue;

        if (Entry.ObserverTeam == EVisionChannel::AlwaysVisible)
            return true;

        if (Entry.ObserverTeam == Team)
            return true;
    }
    return false;
}

EVisionChannel UVisionGameStateComp::GetLocalPlayerTeamChannel() const
{
    APlayerController* LocalPC = GEngine->GetFirstLocalPlayerController(GetWorld());
    if (!LocalPC)
        return EVisionChannel::None;

    AGameStateBase* GS = GetWorld()->GetGameState();
    if (!GS)
        return EVisionChannel::None;

    for (APlayerState* PS : GS->PlayerArray)
    {
        if (!PS || PS->GetOwningController() != LocalPC)
            continue;

        if (UVisionPlayerStateComp* VisionPS =
            PS->FindComponentByClass<UVisionPlayerStateComp>())
        {
            return VisionPS->GetTeamChannel();
        }
    }

    return EVisionChannel::None;
}

// -------------------------------------------------------------------------- //
//  Fan-out to eligible players
// -------------------------------------------------------------------------- //

void UVisionGameStateComp::PushEntryToEligiblePlayers(
    AActor* Target, EVisionChannel Team, bool bAdd) const
{
    AGameStateBase* GS = GetWorld()->GetGameState();
    if (!GS) return;

    for (APlayerState* PS : GS->PlayerArray)
    {
        if (!PS) continue;

        UVisionPlayerStateComp* VisionPS =
            PS->FindComponentByClass<UVisionPlayerStateComp>();
        if (!VisionPS) continue;

        if (!VisionPS->CanSeeTeam(Team))
            continue;

        if (bAdd)
            VisionPS->AddTeamVisibleEntry(Target, Team);
        else
            VisionPS->RemoveTeamVisibleEntry(Target, Team);
    }
}

void UVisionGameStateComp::RebuildPlayerVisibleEntries(UVisionPlayerStateComp* VisionPS) const
{
    if (!VisionPS || !GetOwner()->HasAuthority())
        return;

    TArray<FVisibleActorEntry> Filtered;
    for (const FVisibleActorEntry& Entry : VisibleActors)
    {
        if (VisionPS->CanSeeTeam(Entry.ObserverTeam))
            Filtered.Add(Entry);
    }

    VisionPS->ResetTeamVisibleEntries(Filtered);

    UE_LOG(VisionGameStateComp, Verbose,
        TEXT("RebuildPlayerVisibleEntries >> %s rebuilt with %d entries"),
        *VisionPS->GetOwner()->GetName(), Filtered.Num());
}
