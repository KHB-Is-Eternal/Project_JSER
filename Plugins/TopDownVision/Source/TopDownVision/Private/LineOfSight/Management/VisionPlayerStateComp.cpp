#include "LineOfSight/Management/VisionPlayerStateComp.h"

#include "Net/UnrealNetwork.h"
#include "GameFramework/GameStateBase.h"
#include "GameFramework/PlayerState.h"
#include "LineOfSight/Management/VisionGameStateComp.h"
#include "LineOfSight/VisionComps/Vision_VisualComp.h"
#include "LineOfSight/VisionComps/Vision_EvaluatorComp.h"
#include "LineOfSight/Management/Subsystem/LOSVisionSubsystem.h"

DEFINE_LOG_CATEGORY(VisionPlayerStateComp);

// -------------------------------------------------------------------------- //
//  FastArray callbacks — fire on the owning client only (COND_OwnerOnly)
// -------------------------------------------------------------------------- //

void FPlayerVisibleActorArray::PostReplicatedAdd(
    const TArrayView<int32>& AddedIndices, int32 FinalSize)
{
    if (!OwnerComp) return;
    for (int32 Idx : AddedIndices)
        if (Items.IsValidIndex(Idx))
            OwnerComp->OnTeamEntryAdded(Items[Idx].Target);
}

void FPlayerVisibleActorArray::PreReplicatedRemove(
    const TArrayView<int32>& RemovedIndices, int32 FinalSize)
{
    if (!OwnerComp) return;

    // NOTE: PreReplicatedRemove 시점에는 항목이 아직 배열에 남아 있다.
    // Team을 ExcludeObserverTeam으로 넘겨 제거 중인 항목을 재평가에서 제외한다.
    // (기존 VisionGameStateComp FastArray와 동일한 타이밍 규칙 — 최악의 경우
    //  한 프레임 늦게 숨겨지는 것은 허용)
    for (int32 Idx : RemovedIndices)
        if (Items.IsValidIndex(Idx))
            OwnerComp->OnTeamEntryRemoved(Items[Idx].Target, Items[Idx].ObserverTeam);
}

void FPlayerVisibleActorArray::PostReplicatedChange(
    const TArrayView<int32>& ChangedIndices, int32 FinalSize)
{
    // Entries are only added or removed — no change events expected.
}

// -------------------------------------------------------------------------- //

UVisionPlayerStateComp::UVisionPlayerStateComp()
{
    PrimaryComponentTick.bCanEverTick = false;
    SetIsReplicatedByDefault(true);
}

void UVisionPlayerStateComp::GetLifetimeReplicatedProps(
    TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);
    DOREPLIFETIME(UVisionPlayerStateComp, TeamChannel);
    DOREPLIFETIME(UVisionPlayerStateComp, bAllReveal);

    // 자기 팀 관련 항목만 소유 커넥션으로 전송 (005 부록 A)
    DOREPLIFETIME_CONDITION(UVisionPlayerStateComp, TeamVisibleActors, COND_OwnerOnly);
}

void UVisionPlayerStateComp::BeginPlay()
{
    Super::BeginPlay();

    TeamVisibleActors.OwnerComp = this;

    // [Fix] 폰이 나중에 스폰되거나 Possess될 때 비전 채널을 자동으로 동기화하도록 델리게이트 등록
    if (APlayerState* PS = Cast<APlayerState>(GetOwner()))
    {
        PS->OnPawnSet.AddUniqueDynamic(this, &UVisionPlayerStateComp::OnPawnSet);
    }

    if (UWorld* World = GetWorld())
        World->GetTimerManager().SetTimerForNextTick(
            this, &UVisionPlayerStateComp::RefreshVisibility);
}

// -------------------------------------------------------------------------- //
//  Team
// -------------------------------------------------------------------------- //

void UVisionPlayerStateComp::SetTeamChannel(EVisionChannel InTeam)
{
    TeamChannel = InTeam;

    UE_LOG(VisionPlayerStateComp, Log,
        TEXT("[%s] SetTeamChannel >> %d"),
        *GetOwner()->GetName(), (uint8)TeamChannel);

    // [서버] 팀 확정/변경 시 이 플레이어의 OwnerOnly 배열을 스토어 기준으로 재구성 (005 부록 A)
    if (GetOwner()->HasAuthority())
    {
        if (AGameStateBase* GS = GetWorld()->GetGameState())
        {
            if (UVisionGameStateComp* GSComp = GS->FindComponentByClass<UVisionGameStateComp>())
                GSComp->RebuildPlayerVisibleEntries(this);
        }
    }

    SyncPawnVisionChannel();
    InitializeSameTeamEvaluators();
    RefreshVisibility();
}

void UVisionPlayerStateComp::OnRep_TeamChannel()
{
    UE_LOG(VisionPlayerStateComp, Log,
        TEXT("[%s] OnRep_TeamChannel >> %d"),
        *GetOwner()->GetName(), (uint8)TeamChannel);

    SyncPawnVisionChannel();
    InitializeSameTeamEvaluators();
    RefreshVisibility();
}

// -------------------------------------------------------------------------- //
//  All Reveal
// -------------------------------------------------------------------------- //

void UVisionPlayerStateComp::SetAllReveal(bool bEnabled)
{
    if (!GetOwner()->HasAuthority())
    {
        UE_LOG(VisionPlayerStateComp, Warning,
            TEXT("[%s] SetAllReveal >> Server only"), *GetOwner()->GetName());
        return;
    }

    bAllReveal = bEnabled;

    UE_LOG(VisionPlayerStateComp, Log,
        TEXT("[%s] SetAllReveal >> %s"),
        *GetOwner()->GetName(), bAllReveal ? TEXT("ON") : TEXT("OFF"));

    RefreshVisibility();
}

void UVisionPlayerStateComp::OnRep_AllReveal()
{
    UE_LOG(VisionPlayerStateComp, Log,
        TEXT("[%s] OnRep_AllReveal >> %s"),
        *GetOwner()->GetName(), bAllReveal ? TEXT("ON") : TEXT("OFF"));
    RefreshVisibility();
}

// -------------------------------------------------------------------------- //
//  CanSeeTeam
// -------------------------------------------------------------------------- //

bool UVisionPlayerStateComp::CanSeeTeam(EVisionChannel InTeam) const
{
    if (InTeam == EVisionChannel::AlwaysVisible)
        return true;

    return bAllReveal || (TeamChannel == InTeam);
}

// -------------------------------------------------------------------------- //
//  Per-player visible entries (server writes, owner-only replication)
// -------------------------------------------------------------------------- //

void UVisionPlayerStateComp::AddTeamVisibleEntry(AActor* Target, EVisionChannel ObserverTeam)
{
    if (!Target || !GetOwner()->HasAuthority())
        return;

    // 중복 방지 (동일 Target+Team)
    for (const FVisibleActorEntry& Entry : TeamVisibleActors.Items)
    {
        if (Entry.Target == Target && Entry.ObserverTeam == ObserverTeam)
            return;
    }

    FVisibleActorEntry& Entry = TeamVisibleActors.Items.AddDefaulted_GetRef();
    Entry.Target       = Target;
    Entry.ObserverTeam = ObserverTeam;
    TeamVisibleActors.MarkItemDirty(Entry);

    // 리슨서버 호스트/스탠드얼론: 복제 콜백이 안 오므로 즉시 로컬 반영
    if (IsOwnedByLocalController())
        ReevaluateTargetVisibility(Target);
}

void UVisionPlayerStateComp::RemoveTeamVisibleEntry(AActor* Target, EVisionChannel ObserverTeam)
{
    if (!Target || !GetOwner()->HasAuthority())
        return;

    for (int32 i = TeamVisibleActors.Items.Num() - 1; i >= 0; --i)
    {
        const FVisibleActorEntry& Entry = TeamVisibleActors.Items[i];
        if (Entry.Target != Target || Entry.ObserverTeam != ObserverTeam)
            continue;

        TeamVisibleActors.Items.RemoveAt(i);
        TeamVisibleActors.MarkArrayDirty();

        // 항목이 이미 제거된 뒤라 Exclude 없이 재평가해도 정확함
        if (IsOwnedByLocalController())
            ReevaluateTargetVisibility(Target);
        return;
    }
}

void UVisionPlayerStateComp::ResetTeamVisibleEntries(const TArray<FVisibleActorEntry>& NewEntries)
{
    if (!GetOwner()->HasAuthority())
        return;

    TeamVisibleActors.Items.Reset();
    for (const FVisibleActorEntry& Source : NewEntries)
    {
        // ReplicationID가 새 배열에서 재할당되도록 필드만 복사
        FVisibleActorEntry& Entry = TeamVisibleActors.Items.AddDefaulted_GetRef();
        Entry.Target       = Source.Target;
        Entry.ObserverTeam = Source.ObserverTeam;
    }
    TeamVisibleActors.MarkArrayDirty();

    if (IsOwnedByLocalController())
        RefreshVisibility();
}

bool UVisionPlayerStateComp::IsOwnedByLocalController() const
{
    const APlayerState* PS = Cast<APlayerState>(GetOwner());
    const AController* OwnerController = PS ? PS->GetOwningController() : nullptr;
    return OwnerController && OwnerController->IsLocalController();
}

// -------------------------------------------------------------------------- //
//  FastArray client callbacks
// -------------------------------------------------------------------------- //

void UVisionPlayerStateComp::OnTeamEntryAdded(AActor* Target)
{
    if (!Target) return;

    // 팀 채널이 아직 복제 전이어도 안전 — OnRep_TeamChannel의 RefreshVisibility가
    // 전체 항목을 다시 평가해 보정한다 (기존 PendingReveals 큐 대체).
    ReevaluateTargetVisibility(Target);
}

void UVisionPlayerStateComp::OnTeamEntryRemoved(AActor* Target, EVisionChannel Team)
{
    if (!Target) return;

    // 제거 중인 항목은 아직 배열에 남아 있으므로 Exclude로 건너뛴다
    ReevaluateTargetVisibility(Target, Team);
}

// -------------------------------------------------------------------------- //
//  SyncPawnVisionChannel
//
//  Players have VisionChannel = None when Vision_VisualComp::Initialize()
//  runs because the team replicates after BeginPlay. This pushes the correct
//  channel to the VisualComp and re-registers it with the subsystem so
//  GetProvidersForTeam and CanSeeTeam work correctly for player pawns.
// -------------------------------------------------------------------------- //

void UVisionPlayerStateComp::SyncPawnVisionChannel()
{
    if (TeamChannel == EVisionChannel::None)
        return;

    APlayerState* PS = Cast<APlayerState>(GetOwner());
    if (!PS) return;

    AController* Controller = PS->GetOwningController();
    APawn* Pawn = Controller ? Controller->GetPawn() : nullptr;
    if (!Pawn) return;

    UVision_VisualComp* VisualComp =
        Pawn->FindComponentByClass<UVision_VisualComp>();
    if (!VisualComp) return;

    if (VisualComp->GetVisionChannel() == TeamChannel)
        return;

    ULOSVisionSubsystem* Subsystem =
        GetWorld()->GetSubsystem<ULOSVisionSubsystem>();

    // Unregister from old channel first
    if (VisualComp->GetVisionChannel() != EVisionChannel::None && Subsystem)
        Subsystem->UnregisterProvider(VisualComp, VisualComp->GetVisionChannel());

    VisualComp->SetVisionChannel(TeamChannel);

    if (Subsystem)
        Subsystem->RegisterProvider(VisualComp, TeamChannel);

    UE_LOG(VisionPlayerStateComp, Log,
        TEXT("[%s] SyncPawnVisionChannel >> %s synced to channel %d"),
        *GetOwner()->GetName(), *Pawn->GetName(), (uint8)TeamChannel);
}

// -------------------------------------------------------------------------- //
//  InitializeSameTeamEvaluators
// -------------------------------------------------------------------------- //

void UVisionPlayerStateComp::InitializeSameTeamEvaluators()
{
    if (TeamChannel == EVisionChannel::None)
        return;

    UWorld* World = GetWorld();
    if (!World) return;

    APlayerController* PC = GEngine->GetFirstLocalPlayerController(World);
    if (!PC) return;

    ULOSVisionSubsystem* Subsystem = World->GetSubsystem<ULOSVisionSubsystem>();
    if (!Subsystem) return;

    // Iterate ALL providers across every channel.
    // CanSeeTeam is the single filter — same logic the RT manager uses.
    for (UVision_VisualComp* Provider : Subsystem->GetAllProviders())
    {
        if (!Provider || !Provider->GetOwner())
            continue;

        // 로컬 플레이어의 팀 정보가 세팅/변경되었으므로 모든 시야 제공자의 동적 반경을 갱신합니다.
        Provider->RefreshOcclusionAndEvaluatorRadius();

        if (!CanSeeTeam(Provider->GetVisionChannel()))
            continue;

        // Skip locally controlled pawn — already initialized
        APawn* Pawn = Cast<APawn>(Provider->GetOwner());
        if (Pawn && Pawn->IsLocallyControlled())
            continue;

        UVision_EvaluatorComp* Evaluator =
            Provider->GetOwner()->FindComponentByClass<UVision_EvaluatorComp>();
        if (!Evaluator)
            continue;

        Evaluator->InitializeIfSameTeam();
    }

    UE_LOG(VisionPlayerStateComp, Log,
        TEXT("[%s] InitializeSameTeamEvaluators >> Done for team %d"),
        *GetOwner()->GetName(), (uint8)TeamChannel);
}

// -------------------------------------------------------------------------- //
//  ReevaluateTargetVisibility
// -------------------------------------------------------------------------- //

void UVisionPlayerStateComp::ReevaluateTargetVisibility(
    AActor* Target, EVisionChannel ExcludeObserverTeam)
{
    if (!Target) return;

    UVision_VisualComp* VisualComp =
        Target->FindComponentByClass<UVision_VisualComp>();
    if (!VisualComp) return;

    // 리슨 서버 월드에는 PC가 여러 개라 GetFirstPlayerController가 원격 클라의
    // PC를 돌려줄 수 있음 — "이 머신에 로컬 플레이어가 있는가"를 직접 묻는다
    // (데디케이트 서버면 null → 표시 로직 스킵).
    APlayerController* PC = GEngine->GetFirstLocalPlayerController(GetWorld());
    if (!PC) return;

    VisualComp->SetVisible(
        ComputeTargetVisibility(Target, VisualComp, ExcludeObserverTeam));
}

bool UVisionPlayerStateComp::ComputeTargetVisibility(
    const AActor* Target,
    const UVision_VisualComp* VisualComp,
    EVisionChannel ExcludeObserverTeam) const
{
    if (!Target || !VisualComp) return false;

    if (bAllReveal)
        return true;

    const EVisionChannel TargetTeam = VisualComp->GetVisionChannel();
    if (CanSeeTeam(TargetTeam))
        return true;

    // --- Pass 1: local vote map ---
    // Updated synchronously before any RPC — zero latency for the
    // evaluating client.
    ULOSVisionSubsystem* Subsystem =
        GetWorld()->GetSubsystem<ULOSVisionSubsystem>();

    if (Subsystem)
    {
        // const_cast: TMap 키 조회 전용, Target을 수정하지 않음
        const TMap<uint8, TSet<TWeakObjectPtr<AActor>>>* VoteMap =
            Subsystem->GetVisibilityVotesForTarget(const_cast<AActor*>(Target));

        if (VoteMap)
        {
            for (const TPair<uint8, TSet<TWeakObjectPtr<AActor>>>& TeamPair
                : *VoteMap)
            {
                EVisionChannel EntryTeam = (EVisionChannel)TeamPair.Key;

                if (ExcludeObserverTeam != EVisionChannel::None &&
                    EntryTeam == ExcludeObserverTeam)
                    continue;

                if (TeamPair.Value.Num() == 0)
                    continue;

                if (CanSeeTeam(EntryTeam))
                    return true;
            }
        }
    }

    // --- Pass 2: per-player replicated state ---
    // Catches shared vision from teammates on other machines.
    // Player B has no local vote for Player A's sighting — this pass finds it
    // via the owner-only FastArray the server fans out to us (005 부록 A).
    for (const FVisibleActorEntry& Entry : TeamVisibleActors.Items)
    {
        if (Entry.Target != Target)
            continue;

        if (ExcludeObserverTeam != EVisionChannel::None &&
            Entry.ObserverTeam == ExcludeObserverTeam)
            continue;

        if (CanSeeTeam(Entry.ObserverTeam))
            return true;
    }

    return false;
}

// -------------------------------------------------------------------------- //
//  RefreshVisibility
// -------------------------------------------------------------------------- //

void UVisionPlayerStateComp::RefreshVisibility()
{
    UWorld* World = GetWorld();
    if (!World) return;

    TSet<AActor*> Evaluated;
    for (const FVisibleActorEntry& Entry : TeamVisibleActors.Items)
    {
        if (!Entry.Target || Evaluated.Contains(Entry.Target))
            continue;

        Evaluated.Add(Entry.Target);
        ReevaluateTargetVisibility(Entry.Target);
    }

    UE_LOG(VisionPlayerStateComp, Verbose,
        TEXT("[%s] RefreshVisibility >> %d unique actors | Team:%d | AllReveal:%d"),
        *GetOwner()->GetName(),
        Evaluated.Num(),
        (uint8)TeamChannel,
        (int32)bAllReveal);
}

void UVisionPlayerStateComp::OnPawnSet(APlayerState* PlayerState, APawn* NewPawn, APawn* OldPawn)
{
    SyncPawnVisionChannel();
}