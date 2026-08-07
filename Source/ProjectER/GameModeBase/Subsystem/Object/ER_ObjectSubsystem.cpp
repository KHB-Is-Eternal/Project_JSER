#include "GameModeBase/Subsystem/Object/ER_ObjectSubsystem.h"
#include "GameModeBase/GameMode/ER_InGameMode.h"
#include "GameModeBase/State/ER_GameState.h"
#include "GameModeBase/PointActor/ER_PointActor.h"

#include "Monster/BaseMonster.h"

#include "Kismet/GameplayStatics.h"
#include "LevelManagement/LevelGraphManager/LevelAreaGameStateComp/LevelAreaGameModeComponent.h"
#include "LevelManagement/LevelGraphManager/LevelAreaSubsystem/LevelAreaGraphSubsystem.h"

void UER_ObjectSubsystem::InitializeObjectPoints(TMap<FName, FObjectClassConfig>& ObjectClass)
{
    UWorld* World = GetWorld();
    if (!World)
        return;

    if (World->GetNetMode() == NM_Client)
        return;

    if (bIsInitialized)
        return;

    UE_LOG(LogTemp, Log, TEXT("[OSS] InitializeObjectPoints Start Points Count : %d"), Points.Num());

    const FName ObjectTag(TEXT("Object"));
    const FName BossTag(TEXT("Monster"));
    const FName SupplyTag(TEXT("Supply"));
    const FName SafeTag(TEXT("Safe"));

    SupplyPointsByRegion.Reset();
    BossPoints.Reset();
    SafePoints.Reset();
    SpawnedSafeZoneActor = nullptr;

    for (auto& Point : Points)
    {
        AActor* PointActor = Point.Get();

        if (!IsValid(PointActor))
            continue;

        const FObjectClassConfig* Picked = nullptr;
        FName DAName;
        FName TagName;

        // 보스 이니셜라이즈
        if (PointActor->ActorHasTag(BossTag))
        {
            for (const FName& Tag : PointActor->Tags)
            {
                if (Tag == BossTag)
                    continue;

                if (const FObjectClassConfig* Found = ObjectClass.Find(Tag))
                {
                    TagName = BossTag;
                    Picked = Found;
                    DAName = Tag;
                    break;
                }
            }

            if (!Picked || !Picked->Class)
            {
                UE_LOG(LogTemp, Warning, TEXT("[OSS] No Class mapping for %s"), *PointActor->GetName());
                continue;
            }

            FObjectInfo Info;
            Info.SpawnPoint = PointActor;
            Info.ObjectClass = Picked->Class;
            Info.DAName = DAName;
            Info.bIsSpawned = false;
            
            if (AER_PointActor* PA = Cast<AER_PointActor>(PointActor))
            {
                Info.RegionType = PA->RegionType;
            }
            else
            {
                Info.RegionType = ERegionType::None;
            }

            BossPoints.Add(Info);
        }
        
        // 보급 이니셜라이즈
        else if (PointActor->ActorHasTag(SupplyTag))
        {
            for (const FName& Tag : PointActor->Tags)
            {
                if (Tag == SupplyTag)
                    continue;

                if (const FObjectClassConfig* Found = ObjectClass.Find(Tag))
                {
                    TagName = SupplyTag;
                    Picked = Found;
                    //DAName = Tag;
                    break;
                }
            }

            if (!Picked || !Picked->Class)
            {
                UE_LOG(LogTemp, Warning, TEXT("[OSS] No Class mapping for %s"), *PointActor->GetName());
                continue;
            }
            AER_PointActor* PA = Cast<AER_PointActor>(PointActor);
            if (!PA)
                continue;

            FObjectInfo Info;
            Info.SpawnPoint = PointActor;
            Info.ObjectClass = Picked->Class;
            Info.bIsSpawned = false;
            Info.RegionType = PA->RegionType;

            SupplyPointsByRegion.FindOrAdd(Info.RegionType).Add(Info);
        }
        
        // 안전 구역 이니셜라이즈
        else if (PointActor->ActorHasTag(SafeTag))
        {
            for (const FName& Tag : PointActor->Tags)
            {
                if (Tag == SafeTag)
                    continue;

                if (const FObjectClassConfig* Found = ObjectClass.Find(Tag))
                {
                    TagName = SafeTag;
                    Picked = Found;
                    break;
                }
            }

            if (!Picked || !Picked->Class)
            {
                UE_LOG(LogTemp, Warning, TEXT("[OSS] No SafeZone Class mapping for %s"), *PointActor->GetName());
                continue;
            }
            AER_PointActor* PA = Cast<AER_PointActor>(PointActor);
            if (!PA)
                continue;

            FObjectInfo Info;
            Info.SpawnPoint = PointActor;
            Info.ObjectClass = Picked->Class;
            Info.bIsSpawned = false;
            Info.RegionType = PA->RegionType;

            SafePoints.Add(Info);
        }
        
    }
    bIsInitialized = true;
    UE_LOG(LogTemp, Log, TEXT("[OSS] InitializeObjectPoints End. SupplyPoint Count : %d , BossPoint Count : %d, SafePoint Count : %d"), SupplyPointsByRegion.Num(), BossPoints.Num(), SafePoints.Num());
}

void UER_ObjectSubsystem::PickSupplySpawnIndex()
{
    UWorld* World = GetWorld();
    if (!World || World->GetNetMode() == NM_Client)
    {
        return;
    }

    if (!bIsInitialized)
    {
        return;
    }

    PendingSupplyPicks.Reset();

    AER_GameState* ERGS = World->GetGameState<AER_GameState>();
    int32 CurrentPhase = ERGS ? ERGS->GetCurrentPhase() : 1;

    AER_InGameMode* GM = Cast<AER_InGameMode>(World->GetAuthGameMode());
    ULevelAreaGameModeComponent* AreaGSComp = GM ? GM->GetComponentByClass<ULevelAreaGameModeComponent>() : nullptr;
    ULevelAreaGraphSubsystem* GraphSub = World->GetSubsystem<ULevelAreaGraphSubsystem>();

    TArray<int32> NextHazards;
    if (AreaGSComp)
    {
        NextHazards = AreaGSComp->GetNextPhaseZoneIDs(CurrentPhase);
    }

    for (auto& Pair : SupplyPointsByRegion)
    {
        ERegionType Region = Pair.Key;
        TArray<FObjectInfo>& Infos = Pair.Value;

        TArray<int32> Candidates;
        Candidates.Reserve(Infos.Num());

        for (int32 i = 0; i < Infos.Num(); ++i)
        {
            // 이미 스폰 됐거나, 예약 상태라면 제외
            if (!Infos[i].bIsSpawned && !Infos[i].bIsReserved && Infos[i].ObjectClass && IsValid(Infos[i].SpawnPoint.Get()))
            {
                int32 NodeID = static_cast<int32>(Infos[i].RegionType);

                // 현재 금지구역이거나, 다음 페이즈에 금지구역이 될 구역은 제외
                bool bIsCurrentHazard = GraphSub ? GraphSub->IsNodeHazard(NodeID) : false;
                bool bIsNextHazard = NextHazards.Contains(NodeID);

                if (!bIsCurrentHazard && !bIsNextHazard)
                {
                    Candidates.Add(i);
                }
            }
        }

        if (Candidates.Num() == 0)
            continue;

        const int32 PickIdx = Candidates[FMath::RandRange(0, Candidates.Num() - 1)];

        FSupplySpawnPick Info;
        Info.Region = Region;
        Info.Index = PickIdx;

        // 예약으로 변경
        Infos[PickIdx].bIsReserved = true;
        PendingSupplyPicks.Add(Info);

        if (AER_PointActor* PA = Cast<AER_PointActor>(Infos[PickIdx].SpawnPoint.Get()))
        {
            PA->SetSelectedVisual(true);
        }
    }
}

void UER_ObjectSubsystem::SpawnSupplyObject()
{
    UWorld* World = GetWorld();
    if (!World || World->GetNetMode() == NM_Client)
    {
        return;
    }

    if (!bIsInitialized || PendingSupplyPicks.Num() == 0)
    {
        return;
    }

    for (int32 p = PendingSupplyPicks.Num() - 1; p >= 0; --p)
    {
        const FSupplySpawnPick Pick = PendingSupplyPicks[p];

        TArray<FObjectInfo>* InfosPtr = SupplyPointsByRegion.Find(Pick.Region);
        if (!InfosPtr || !InfosPtr->IsValidIndex(Pick.Index))
        {
            PendingSupplyPicks.RemoveAtSwap(p);
            continue;
        }

        FObjectInfo& Info = (*InfosPtr)[Pick.Index];

        if (Info.bIsSpawned)
        {
            Info.bIsReserved = false;
            PendingSupplyPicks.RemoveAtSwap(p);
            continue;
        }

        SpawnObjectInternal(Info);
        PendingSupplyPicks.RemoveAtSwap(p);
    }
}

void UER_ObjectSubsystem::PickBossSpawnIndex()
{
    UWorld* World = GetWorld();
    if (!World || World->GetNetMode() == NM_Client)
    {
        return;
    }

    if (!bIsInitialized)
    {
        return;
    }

    if (BossPoints.Num() <= 0)
    {
        return;
    }

    PendingBossPicks.Reset();

    AER_GameState* ERGS = World->GetGameState<AER_GameState>();
    int32 CurrentPhase = ERGS ? ERGS->GetCurrentPhase() : 1;

    AER_InGameMode* GM = Cast<AER_InGameMode>(World->GetAuthGameMode());
    ULevelAreaGameModeComponent* AreaGSComp = GM ? GM->GetComponentByClass<ULevelAreaGameModeComponent>() : nullptr;
    ULevelAreaGraphSubsystem* GraphSub = World->GetSubsystem<ULevelAreaGraphSubsystem>();

    TArray<int32> NextHazards;
    if (AreaGSComp)
    {
        NextHazards = AreaGSComp->GetNextPhaseZoneIDs(CurrentPhase);
    }

    TArray<int32> Candidates;
    for (int32 i = 0; i < BossPoints.Num(); ++i)
    {
        if (!BossPoints[i].bIsReserved && !BossPoints[i].bIsSpawned && BossPoints[i].ObjectClass && IsValid(BossPoints[i].SpawnPoint.Get()))
        {
            int32 NodeID = static_cast<int32>(BossPoints[i].RegionType);

            bool bIsCurrentHazard = GraphSub ? GraphSub->IsNodeHazard(NodeID) : false;
            bool bIsNextHazard = NextHazards.Contains(NodeID);

            if (!bIsCurrentHazard && !bIsNextHazard)
            {
                Candidates.Add(i);
            }
        }
    }

    int32 PickIdx = -1;
    if (Candidates.Num() > 0)
    {
        PickIdx = Candidates[FMath::RandRange(0, Candidates.Num() - 1)];
    }

    if (PickIdx != -1)
    {
        FSupplySpawnPick Info;
        Info.Region = ERegionType::None;
        Info.Index = PickIdx;

        BossPoints[PickIdx].bIsReserved = true;
        PendingBossPicks.Add(Info);

        if (AER_PointActor* PA = Cast<AER_PointActor>(BossPoints[PickIdx].SpawnPoint.Get()))
        {
            PA->SetSelectedVisual(true);
        }
    }


}

void UER_ObjectSubsystem::SpawnBossObject()
{
    UWorld* World = GetWorld();
    if (!World || World->GetNetMode() == NM_Client)
    {
        return;
    }

    if (!bIsInitialized || PendingBossPicks.Num() == 0)
    {
        return;
    }

    for (int32 p = PendingBossPicks.Num() - 1; p >= 0; --p)
    {
        const FSupplySpawnPick Pick = PendingBossPicks[p];

        if (!BossPoints.IsValidIndex(Pick.Index))
        {
            PendingBossPicks.RemoveAtSwap(p);
            continue;
        }

        FObjectInfo& Info = BossPoints[Pick.Index];

        if (Info.bIsSpawned)
        {
            Info.bIsReserved = false;
            PendingBossPicks.RemoveAtSwap(p);
            continue;
        }

        SpawnObjectInternal(Info);
        PendingBossPicks.RemoveAtSwap(p);
    }
}

void UER_ObjectSubsystem::SpawnObjectInternal(FObjectInfo& Info)
{
    UWorld* World = GetWorld();
    if (!World) 
    {
        return;
    }

    if (!IsValid(Info.SpawnPoint.Get()) || !Info.ObjectClass)
    {
        Info.bIsReserved = false;
        return;
    }

    const FTransform SpawnTM = Info.SpawnPoint->GetActorTransform();

    AActor* Spawned = World->SpawnActorDeferred<AActor>(
        Info.ObjectClass,
        SpawnTM,
        nullptr,
        nullptr,
        ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn
    );

    if (!Spawned)
    {
        Info.bIsReserved = false;
        return;
    }

    if (ABaseMonster* BossMonster = Cast<ABaseMonster>(Spawned))
    {
        const int32 Key = Info.SpawnPoint->GetUniqueID();
        BossMonster->SetSpawnPoint(Key);
    }

    // 1. 스폰부터 완벽하게 끝내기 (BeginPlay -> PossessedBy 등 호출 보장)
    UGameplayStatics::FinishSpawningActor(Spawned, SpawnTM);

    // 2. 그 이후에 스탯/스킬 꽂아넣기
    if (ABaseMonster* BossMonster = Cast<ABaseMonster>(Spawned))
    {
        FPrimaryAssetId MonsterAssetId(TEXT("Monster"), Info.DAName);
        AER_GameState* ERGS = World->GetAuthGameMode()->GetGameState<AER_GameState>();
        int32 Phase = (ERGS && ERGS->GetCurrentPhase() > 0) ? ERGS->GetCurrentPhase() : 1;
        BossMonster->InitMonsterData(MonsterAssetId, Phase);

        // 보스 몬스터 등장 시 디버그 로그 및 화면 메시지 출력
        if (Info.DAName.ToString().Contains(TEXT("Boss"), ESearchCase::IgnoreCase))
        {
            FString RegionName = UEnum::GetValueAsString(Info.RegionType);
            UE_LOG(LogTemp, Warning, TEXT("[ER_ObjectSubsystem] Boss Monster Spawned! ID: %s, Level: %d, Region: %s"), *MonsterAssetId.ToString(), Phase, *RegionName);
            if (GEngine)
            {
                GEngine->AddOnScreenDebugMessage(
                    -1, 
                    10.0f, 
                    FColor::Red, 
                    FString::Printf(TEXT("★ Boss Spawned: %s (Phase %d) at %s ★"), *Info.DAName.ToString(), Phase, *RegionName)
                );
            }
        }
    }

    Info.bIsSpawned = true;
    Info.bIsReserved = false;

    if (AER_PointActor* PA = Cast<AER_PointActor>(Info.SpawnPoint.Get()))
    {
        PA->SetSelectedVisual(false);
    }
}

void UER_ObjectSubsystem::RegisterPoint(AActor* Point)
{
	Points.AddUnique(Point);
}

void UER_ObjectSubsystem::UnregisterPoint(AActor* Point)
{
	Points.Remove(Point);
}

void UER_ObjectSubsystem::SpawnSafeZone(int32 NextHazardRegionID)
{
    UWorld* World = GetWorld();
    if (!World || World->GetNetMode() == NM_Client)
    {
        return;
    }

    if (!bIsInitialized)
    {
        return;
    }

    // 기존에 활성화된 구역이 있다면 안전하게 제거
    DespawnSafeZone();

    // 입력받은 지역 번호와 매칭되는 스폰 위치 탐색
    FObjectInfo* TargetInfo = nullptr;
    for (FObjectInfo& Info : SafePoints)
    {
        if (static_cast<int32>(Info.RegionType) == NextHazardRegionID && IsValid(Info.SpawnPoint.Get()))
        {
            TargetInfo = &Info;
            break;
        }
    }

    if (TargetInfo && TargetInfo->ObjectClass)
    {
        const FTransform SpawnTM = TargetInfo->SpawnPoint->GetActorTransform();
        
        SpawnedSafeZoneActor = World->SpawnActorDeferred<AActor>(
            TargetInfo->ObjectClass,
            SpawnTM,
            nullptr,
            nullptr,
            ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn
        );

        if (SpawnedSafeZoneActor)
        {
            UGameplayStatics::FinishSpawningActor(SpawnedSafeZoneActor, SpawnTM);
            TargetInfo->bIsSpawned = true;
            
            // 시각적 피드백 선택 해제 (기존 포인트 스폰 로직들과 동일하게 처리)
            if (AER_PointActor* PA = Cast<AER_PointActor>(TargetInfo->SpawnPoint.Get()))
            {
                PA->SetSelectedVisual(false);
            }
            
            UE_LOG(LogTemp, Log, TEXT("[OSS] Successfully Spawned Safe Zone at Region: %d"), NextHazardRegionID);
        }
    }
    else
    {
        UE_LOG(LogTemp, Warning, TEXT("[OSS] Failed to find spawn point or object class for Safe Zone at Region: %d"), NextHazardRegionID);
    }
}

void UER_ObjectSubsystem::DespawnSafeZone()
{
    if (IsValid(SpawnedSafeZoneActor))
    {
        SpawnedSafeZoneActor->Destroy();
        SpawnedSafeZoneActor = nullptr;
        UE_LOG(LogTemp, Log, TEXT("[OSS] Safe Zone Despawned."));
    }

    // 모든 안전 구역 포인트의 스폰 상태 초기화
    for (FObjectInfo& Info : SafePoints)
    {
        Info.bIsSpawned = false;
    }
}
