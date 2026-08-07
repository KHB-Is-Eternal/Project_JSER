#include "LineOfSight/Management/Subsystem/LOSVisionSubsystem.h"

#include "TopDownVision/Public/LineOfSight/VisionComps/Vision_VisualComp.h"
#include "LineOfSight/Management/VisionGameStateComp.h"
#include "LineOfSight/Management/VisionPlayerStateComp.h"
#include "GameFramework/GameStateBase.h"
#include "GameFramework/PlayerState.h"

DEFINE_LOG_CATEGORY(LOSVisionSubsystem);

// -------------------------------------------------------------------------- //
//  Unified visibility query
// -------------------------------------------------------------------------- //

bool ULOSVisionSubsystem::IsActorVisibleToLocalPlayer(const AActor* Target) const
{
    if (!Target)
        return false;

    const UVision_VisualComp* VisualComp =
        Target->FindComponentByClass<UVision_VisualComp>();
    if (!VisualComp)
    {
        // 정책: 게이팅 대상은 Vision_VisualComp 부착. 미부착 = 게이팅 대상 아님(항상 보임).
        if (!MissingVisualCompWarned.Contains(Target))
        {
            MissingVisualCompWarned.Add(Target);
            UE_LOG(LOSVisionSubsystem, Warning,
                TEXT("IsActorVisibleToLocalPlayer >> No Vision_VisualComp on '%s' — treated as always visible. Attach the component if this actor should be vision-gated."),
                *Target->GetName());
        }
        return true;
    }

    UVisionPlayerStateComp* VisionPS = GetLocalVisionPS(GetWorld());
    if (!VisionPS)
        return true; // 로컬 팀 정보 미수신 단계 — 게이팅 보류 (GetProvidersVisibleToLocalPlayer와 동일한 폴백)

    return VisionPS->ComputeTargetVisibility(Target, VisualComp);
}

// -------------------------------------------------------------------------- //
//  Local player lookup
// -------------------------------------------------------------------------- //

UVisionPlayerStateComp* ULOSVisionSubsystem::GetLocalVisionPS(UWorld* World)
{
    if (!World)
        return nullptr;

    APlayerController* PC = GEngine->GetFirstLocalPlayerController(World);
    if (!PC)
        return nullptr;

    if (PC->PlayerState)
    {
        if (UVisionPlayerStateComp* VPS =
            PC->PlayerState->FindComponentByClass<UVisionPlayerStateComp>())
            return VPS;
    }

    // Fallback — PC->PlayerState may be null during early BeginPlay
    if (AGameStateBase* GS = World->GetGameState())
    {
        for (APlayerState* PS : GS->PlayerArray)
        {
            if (PS && PS->GetOwningController() == PC)
            {
                if (UVisionPlayerStateComp* VPS =
                    PS->FindComponentByClass<UVisionPlayerStateComp>())
                    return VPS;
            }
        }
    }

    return nullptr;
}

// -------------------------------------------------------------------------- //
//  Provider registration
// -------------------------------------------------------------------------- //

bool ULOSVisionSubsystem::RegisterProvider(
    UVision_VisualComp* Provider, EVisionChannel InVisionChannel)
{
    if (!Provider)
    {
        UE_LOG(LOSVisionSubsystem, Error,
            TEXT("RegisterProvider >> Null provider"));
        return false;
    }
    if (!Provider->IsVisionProvider())
    {
        return false;
    }
    if (InVisionChannel == EVisionChannel::None)
    {
        UE_LOG(LOSVisionSubsystem, Error,
            TEXT("RegisterProvider >> VisionChannel is None"));
        return false;
    }

    FRegisteredProviders& ChannelEntry = VisionMap.FindOrAdd(InVisionChannel);
    if (ChannelEntry.RegisteredList.Contains(Provider))
    {
        UE_LOG(LOSVisionSubsystem, Warning,
            TEXT("RegisterProvider >> Already registered: %s on channel %d"),
            *Provider->GetOwner()->GetName(), (uint8)InVisionChannel);
        return false;
    }

    ChannelEntry.RegisteredList.Add(Provider);

    UE_LOG(LOSVisionSubsystem, Verbose,
        TEXT("RegisterProvider >> %s registered on channel %d"),
        *Provider->GetOwner()->GetName(), (uint8)InVisionChannel);

    HandleProviderRegistered(Provider, InVisionChannel);
    return true;
}

void ULOSVisionSubsystem::UnregisterProvider(
    UVision_VisualComp* Provider, EVisionChannel InVisionChannel)
{
    if (!Provider)
    {
        UE_LOG(LOSVisionSubsystem, Error,
            TEXT("UnregisterProvider >> Null provider"));
        return;
    }

    if (FRegisteredProviders* ChannelEntry = VisionMap.Find(InVisionChannel))
    {
        if (ChannelEntry->RegisteredList.Remove(Provider) > 0)
        {
            UE_LOG(LOSVisionSubsystem, Verbose,
                TEXT("UnregisterProvider >> %s unregistered from channel %d"),
                *Provider->GetOwner()->GetName(), (uint8)InVisionChannel);
            return;
        }
    }

    if (Provider->IsVisionProvider())
    {
        UE_LOG(LOSVisionSubsystem, Warning,
            TEXT("UnregisterProvider >> Could not find %s on channel %d"),
            *Provider->GetOwner()->GetName(), (uint8)InVisionChannel);
    }
}

TArray<UVision_VisualComp*> ULOSVisionSubsystem::GetProvidersForTeam(
    EVisionChannel TeamChannel) const
{
    TArray<UVision_VisualComp*> Out;

    if (const FRegisteredProviders* Entry = VisionMap.Find(TeamChannel))
        Out.Append(Entry->RegisteredList);
    else
        UE_LOG(LOSVisionSubsystem, Verbose,
            TEXT("GetProvidersForTeam >> No providers yet for channel %d"),
            (uint8)TeamChannel);

    return Out;
}

// Fixed typo: was GeAllProviders
TArray<UVision_VisualComp*> ULOSVisionSubsystem::GetAllProviders() const
{
    TArray<UVision_VisualComp*> Out;
    for (const TPair<EVisionChannel, FRegisteredProviders>& Pair : VisionMap)
        Out.Append(Pair.Value.RegisteredList);
    return Out;
}

TArray<UVision_VisualComp*> ULOSVisionSubsystem::GetProvidersVisibleToLocalPlayer() const
{
    TArray<UVision_VisualComp*> AllProviders = GetAllProviders();

    UVisionPlayerStateComp* LocalVisionPS = GetLocalVisionPS(GetWorld());
    if (!LocalVisionPS)
    {
        // VisionPS not ready yet — return everything so the RT is not left
        // blank. RefreshVisibility will correct state on the next tick.
        UE_LOG(LOSVisionSubsystem, Verbose,
            TEXT("GetProvidersVisibleToLocalPlayer >> VisionPS not ready, returning all providers"));
        return AllProviders;
    }

    TArray<UVision_VisualComp*> Out;
    Out.Reserve(AllProviders.Num());

    for (UVision_VisualComp* Provider : AllProviders)
    {
        if (!Provider || !Provider->GetOwner())
            continue;

        if (LocalVisionPS->CanSeeTeam(Provider->GetVisionChannel()))
            Out.Add(Provider);
    }

    return Out;
}

// -------------------------------------------------------------------------- //
//  Provider reveal logic
// -------------------------------------------------------------------------- //

void ULOSVisionSubsystem::HandleProviderRegistered(
    UVision_VisualComp* NewProvider, EVisionChannel Channel)
{
    UVisionGameStateComp* GSComp = GetVisionGameStateComp();
    if (!GSComp)
        return;

    const bool bHasAuthority = GSComp->GetOwner()->HasAuthority();
    UVisionPlayerStateComp* LocalVisionPS = GetLocalVisionPS(GetWorld());

    for (UVision_VisualComp* Existing : GetProvidersForTeam(Channel))
    {
        if (!Existing || !Existing->GetOwner())
            continue;

        if (bHasAuthority)
        {
            // 서버: 권위 스토어 기록 → 팀별 OwnerOnly 팬아웃 (005 부록 A)
            GSComp->SetActorVisibleToTeam(Existing->GetOwner(), Channel);
        }
        else if (LocalVisionPS)
        {
            // 클라: 스토어 쓰기가 권위 가드로 차단되므로 로컬 재평가를 직접 호출.
            // 자기/아군 폰은 CanSeeTeam 단락으로 즉시 표시(자기 시야 원의 알파를 살림),
            // 적 프로바이더는 즉시 숨김 — Phase 3 이전의 클라 로컬 표시 경로 대체.
            LocalVisionPS->ReevaluateTargetVisibility(Existing->GetOwner());
        }

        UE_LOG(LOSVisionSubsystem, Verbose,
            TEXT("HandleProviderRegistered >> Revealed %s to channel [%s]"),
            *Existing->GetOwner()->GetName(), *UEnum::GetValueAsString(Channel));
    }

    if (UVisionPlayerStateComp* VisionPS = GetLocalVisionPS(GetWorld()))
    {
        UE_LOG(LOSVisionSubsystem, Verbose,
            TEXT("HandleProviderRegistered >> VisionPS ready, calling RefreshVisibility"));
        VisionPS->RefreshVisibility();
    }
    else
    {
        UE_LOG(LOSVisionSubsystem, Verbose,
            TEXT("HandleProviderRegistered >> VisionPS not ready, BeginPlay next-tick will catch up"));
    }
}

// -------------------------------------------------------------------------- //
//  Visibility reporting
//
//  Uses a TSet of weak observer pointers per team instead of a vote counter.
//  This makes add/remove idempotent — the same observer reporting twice does
//  not inflate the count, and a stale observer (destroyed pawn) is cleaned up
//  automatically without needing a matching Remove call.
// -------------------------------------------------------------------------- //

static void CleanInvalidObservers(TSet<TWeakObjectPtr<AActor>>& Set)
{
    for (auto It = Set.CreateIterator(); It; ++It)
    {
        if (!It->IsValid())
            It.RemoveCurrent();
    }
}

void ULOSVisionSubsystem::ReportTargetVisibility(
    AActor* Observer,
    EVisionChannel ObserverTeam,
    AActor* Target,
    bool bVisible)
{
    if (!Observer || !Target || ObserverTeam == EVisionChannel::None)
        return;

    const uint8 TeamID = (uint8)ObserverTeam;

    FTargetVisibilityVotes& Votes = VisibilityVotes.FindOrAdd(Target);
    TSet<TWeakObjectPtr<AActor>>& Observers = Votes.ObserversByTeam.FindOrAdd(TeamID);

    CleanInvalidObservers(Observers);
    const bool bWasVisible = Observers.Num() > 0;

    if (bVisible) Observers.Add(Observer);
    else          Observers.Remove(Observer);

    CleanInvalidObservers(Observers);
    const bool bIsNowVisible = Observers.Num() > 0;

    if (Observers.Num() == 0)
    {
        Votes.ObserversByTeam.Remove(TeamID);
        if (Votes.ObserversByTeam.Num() == 0)
            VisibilityVotes.Remove(Target);
    }

    if (bWasVisible == bIsNowVisible)
        return;

    // ------------------------------------------------------------------ //
    //  CLIENT: apply locally right now — waiting for the server RPC       //
    //  round trip would add latency. Trust the local vote map (just       //
    //  updated above) and call ReevaluateTargetVisibility directly.       //
    //  The server RPC updates the GSComp store, which fans the entry out  //
    //  to eligible players' owner-only arrays (005 부록 A).               //
    // ------------------------------------------------------------------ //
    if (GetWorld()->GetNetMode() == NM_Client)
    {
        if (UVisionPlayerStateComp* VisionPS = GetLocalVisionPS(GetWorld()))
            VisionPS->ReevaluateTargetVisibility(Target);
        return;
    }

    // SERVER / STANDALONE: write through GSComp as the replication source.
    UVisionGameStateComp* GSComp = GetVisionGameStateComp();
    if (!GSComp) return;

    if (bIsNowVisible)
        GSComp->SetActorVisibleToTeam(Target, ObserverTeam);
    else
        GSComp->ClearActorVisibleToTeam(Target, ObserverTeam);
}

// -------------------------------------------------------------------------- //

UVisionGameStateComp* ULOSVisionSubsystem::GetVisionGameStateComp() const
{
    AGameStateBase* GS = GetWorld()->GetGameState();
    if (!GS)
    {
        UE_LOG(LOSVisionSubsystem, Warning,
            TEXT("GetVisionGameStateComp >> No GameState"));
        return nullptr;
    }

    UVisionGameStateComp* Comp = GS->FindComponentByClass<UVisionGameStateComp>();
    if (!Comp)
    {
        UE_LOG(LOSVisionSubsystem, Warning,
            TEXT("GetVisionGameStateComp >> Not found on %s"),
            *GS->GetClass()->GetName());
    }

    return Comp;
}