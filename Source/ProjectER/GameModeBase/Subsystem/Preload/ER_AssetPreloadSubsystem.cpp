#include "GameModeBase/Subsystem/Preload/ER_AssetPreloadSubsystem.h"
#include "Engine/AssetManager.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Blueprint/UserWidget.h"
#include "Engine/Engine.h"
#include "Engine/GameViewportClient.h"

// 프리로드 체인을 위해 추가된 헤더들
#include "CharacterSystem/Data/CharacterData.h"
#include "Monster/Data/MonsterDataAsset.h"
#include "SkillSystem/SkillDataAsset.h"
#include "SkillSystem/SkillConfig/BaseSkillConfig.h"
#include "SkillSystem/GameplayEffect/BaseGameplayEffect.h"
#include "SkillSystem/GameplayEffectComponent/BaseGEC.h"
#include "SkillSystem/GameplayCueNotify/Particle/SkillNiagaraSpawnConfig.h"
#include "NiagaraFunctionLibrary.h"
#include "Animation/AnimMontage.h"
#include "SkillSystem/AnimNotify/AnimNotify_SkillGameplayCue.h"
#include "SkillSystem/AnimNotify/AnimNotifyState_SkillGameplayCue.h"



void UER_AssetPreloadSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
}

void UER_AssetPreloadSubsystem::Deinitialize()
{
	if (AssetLoadHandle.IsValid())
	{
		AssetLoadHandle->CancelHandle();
		AssetLoadHandle.Reset();
	}
	PreloadedAssets.Empty();

	Super::Deinitialize();
}

void UER_AssetPreloadSubsystem::StartPreloadMonsterAssets()
{
	// 하위 호환성을 위해 캐릭터 경로 배열을 비워서 호출
	StartPreloadAssets(TArray<FSoftObjectPath>());
}

void UER_AssetPreloadSubsystem::StartPreloadAssets(const TArray<FSoftObjectPath>& CharacterPaths)
{
	if (AssetLoadHandle.IsValid() && AssetLoadHandle->IsLoadingInProgress())
	{
		AssetLoadHandle->CancelHandle();
		AssetLoadHandle.Reset();
	}
	PreloadedAssets.Empty();

	// 1단계: 몬스터 데이터 에셋 스캔 및 플레이어 캐릭터 에셋 목록 취합
	FAssetRegistryModule& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry");
	TArray<FAssetData> AssetDataList;

	FARFilter Filter;
	Filter.PackagePaths.Add(FName("/Game/BCW/Monster/MonsterData"));
	Filter.bRecursivePaths = true;
	Filter.ClassPaths.Add(FTopLevelAssetPath(TEXT("/Script/ProjectER"), TEXT("MonsterDataAsset")));
	
	AssetRegistryModule.Get().GetAssets(Filter, AssetDataList);

	TArray<FSoftObjectPath> LevelAssetsToLoad;
	for (const FAssetData& AssetData : AssetDataList)
	{
		LevelAssetsToLoad.Add(AssetData.ToSoftObjectPath());
	}

	for (const FSoftObjectPath& CharPath : CharacterPaths)
	{
		if (CharPath.IsValid())
		{
			LevelAssetsToLoad.AddUnique(CharPath);
		}
	}

	if (LevelAssetsToLoad.Num() == 0)
	{
		UE_LOG(LogTemp, Log, TEXT("[Preload] No Monster or Character assets to load. Broadcasting complete."));
		OnPreloadComplete.Broadcast();
		return;
	}

	UE_LOG(LogTemp, Log, TEXT("[Preload] Step 1: Starting async load for %d Level assets (Characters + Monsters)..."), LevelAssetsToLoad.Num());

	FStreamableManager& StreamableManager = UAssetManager::GetStreamableManager();
	AssetLoadHandle = StreamableManager.RequestAsyncLoad(
		LevelAssetsToLoad,
		FStreamableDelegate::CreateUObject(this, &UER_AssetPreloadSubsystem::OnLevelAssetsLoadedAsync)
	);
}

void UER_AssetPreloadSubsystem::OnLevelAssetsLoadedAsync()
{
	if (!AssetLoadHandle.IsValid())
	{
		OnPreloadComplete.Broadcast();
		return;
	}

	// 1단계에서 로드된 레벨 에셋들을 GC 방지를 위해 보관
	TArray<UObject*> LoadedLevelAssets;
	AssetLoadHandle->GetLoadedAssets(LoadedLevelAssets);
	PreloadedAssets.Append(LoadedLevelAssets);
	AssetLoadHandle.Reset();

	// 2단계: 플레이어 캐릭터의 SkillDataAsset 소프트 경로 및 캐릭터 몽타주 소프트 경로 수집
	TArray<FSoftObjectPath> SkillPathsToLoad;

	for (UObject* Asset : LoadedLevelAssets)
	{
		if (const UCharacterData* CharData = Cast<UCharacterData>(Asset))
		{
			for (const TSoftObjectPtr<USkillDataAsset>& SkillPtr : CharData->SkillDataAsset)
			{
				if (!SkillPtr.IsNull())
				{
					SkillPathsToLoad.AddUnique(SkillPtr.ToSoftObjectPath());
				}
			}

			// 캐릭터 몽타주 비동기 로딩 대상에 추가
			for (const auto& Pair : CharData->CharacterMontages)
			{
				if (!Pair.Value.IsNull())
				{
					SkillPathsToLoad.AddUnique(Pair.Value.ToSoftObjectPath());
				}
			}
		}
		else if (const UMonsterDataAsset* MonsterData = Cast<UMonsterDataAsset>(Asset))
		{
			// 몬스터 스킬은 이미 하드 레퍼런스 상태이므로 가비지 컬렉션 방지 배열에 직접 등록만 해둠
			for (UObject* MonsterSkill : MonsterData->SkillDataAssets)
			{
				if (MonsterSkill)
				{
					PreloadedAssets.AddUnique(MonsterSkill);
				}
			}

			// 몬스터 몽타주도 하드 레퍼런스 상태이므로 GC 방지 배열에 등록
			for (const auto& Pair : MonsterData->Montages)
			{
				if (Pair.Value)
				{
					PreloadedAssets.AddUnique(Pair.Value);
				}
			}
		}
	}

	if (SkillPathsToLoad.Num() == 0)
	{
		// 추가로 로드할 스킬 에셋이 없다면 바로 3단계(나이아가라 파티클 추출)로 점프
		UE_LOG(LogTemp, Log, TEXT("[Preload] Step 2: No dynamic SkillDataAssets to load. Proceeding to particle extraction..."));
		OnSkillAssetsLoadedAsync();
		return;
	}

	UE_LOG(LogTemp, Log, TEXT("[Preload] Step 2: Starting async load for %d dynamic SkillDataAssets..."), SkillPathsToLoad.Num());

	FStreamableManager& StreamableManager = UAssetManager::GetStreamableManager();
	AssetLoadHandle = StreamableManager.RequestAsyncLoad(
		SkillPathsToLoad,
		FStreamableDelegate::CreateUObject(this, &UER_AssetPreloadSubsystem::OnSkillAssetsLoadedAsync)
	);
}

void UER_AssetPreloadSubsystem::OnSkillAssetsLoadedAsync()
{
	if (AssetLoadHandle.IsValid())
	{
		// 2단계에서 로드된 플레이어 스킬 에셋들을 GC 방지를 위해 보관
		TArray<UObject*> LoadedSkillAssets;
		AssetLoadHandle->GetLoadedAssets(LoadedSkillAssets);
		PreloadedAssets.Append(LoadedSkillAssets);
		AssetLoadHandle.Reset();
	}

	// 3단계: 로드된 모든 스킬(플레이어 + 몬스터 스킬) 및 몽타주들로부터 나이아가라 파티클 경로 수집
	TArray<FSoftObjectPath> NiagaraPathsToLoad;

	for (UObject* Asset : PreloadedAssets)
	{
		// 1. 캐릭터 데이터가 소유한 고유 캐릭터 VFX 수집
		if (const UCharacterData* CharData = Cast<UCharacterData>(Asset))
		{
			if (!CharData->BasicHitVFX.IsNull()) NiagaraPathsToLoad.AddUnique(CharData->BasicHitVFX.ToSoftObjectPath());
			if (!CharData->ReviveVFX.IsNull()) NiagaraPathsToLoad.AddUnique(CharData->ReviveVFX.ToSoftObjectPath());
			if (!CharData->LevelUpVFX.IsNull()) NiagaraPathsToLoad.AddUnique(CharData->LevelUpVFX.ToSoftObjectPath());
			if (!CharData->DeathVFX.IsNull()) NiagaraPathsToLoad.AddUnique(CharData->DeathVFX.ToSoftObjectPath());
			continue;
		}

		// 2. 비동기 로딩된 캐릭터 몽타주 및 몬스터 몽타주 분석
		if (const UAnimMontage* Montage = Cast<UAnimMontage>(Asset))
		{
			CollectNiagaraPathsFromMontage(Montage, NiagaraPathsToLoad);
			continue;
		}

		const USkillDataAsset* SkillData = Cast<USkillDataAsset>(Asset);
		if (!SkillData)
		{
			continue;
		}

		// 스킬 설정 내의 몽타주 수집
		if (SkillData->SkillConfig)
		{
			if (const UAnimMontage* SkillMontage = SkillData->SkillConfig->GetAnimMontage())
			{
				CollectNiagaraPathsFromMontage(SkillMontage, NiagaraPathsToLoad);
			}
		}

		if (!SkillData->SkillConfig)
		{
			continue;
		}

		const UBaseSkillConfig* Config = SkillData->SkillConfig;
		TArray<TSubclassOf<UBaseGameplayEffect>> AssociatedGEs;

		// Active Skill의 ExecutionPhases 내의 GE 수집
		for (const FSkillExecutionPhase& Phase : Config->GetExecutionPhases())
		{
			for (const TSubclassOf<UBaseGameplayEffect>& GEClass : Phase.Effects)
			{
				if (GEClass) AssociatedGEs.AddUnique(GEClass);
			}
		}

		// MouseTargetSkill 등 TargetPhases 내의 GE 수집
		if (const UMouseTargetSkillConfig* TargetConfig = Cast<UMouseTargetSkillConfig>(Config))
		{
			for (const FTargetExecutionPhase& Phase : TargetConfig->GetTargetPhases())
			{
				for (const TSubclassOf<UBaseGameplayEffect>& GEClass : Phase.TargetEffects)
				{
					if (GEClass) AssociatedGEs.AddUnique(GEClass);
				}
			}
		}

		// Passive Skill의 Effects 수집
		if (const UPassiveSkillConfig* PassiveConfig = Cast<UPassiveSkillConfig>(Config))
		{
			for (const TSubclassOf<UBaseGameplayEffect>& GEClass : PassiveConfig->Effects)
			{
				if (GEClass) AssociatedGEs.AddUnique(GEClass);
			}
		}

		// 수집된 GE들의 Component(BaseGEC)들로부터 나이아가라 경로 수집
		for (const TSubclassOf<UBaseGameplayEffect>& GEClass : AssociatedGEs)
		{
			if (!GEClass) continue;

			const UBaseGameplayEffect* GE = GEClass->GetDefaultObject<UBaseGameplayEffect>();
			if (!GE) continue;

			for (const UGameplayEffectComponent* Component : GE->GetGEComponents())
			{
				if (const UBaseGEC* BaseGEC = Cast<UBaseGEC>(Component))
				{
					BaseGEC->CollectNiagaraPaths(NiagaraPathsToLoad);
				}
			}
		}
	}

	if (NiagaraPathsToLoad.Num() == 0)
	{
		UE_LOG(LogTemp, Log, TEXT("[Preload] Step 3: No Niagara systems gathered for preloading. Broadcasting complete immediately."));
		OnPreloadComplete.Broadcast();
		return;
	}

	UE_LOG(LogTemp, Log, TEXT("[Preload] Step 3: Starting async load for %d Niagara systems..."), NiagaraPathsToLoad.Num());

	FStreamableManager& StreamableManager = UAssetManager::GetStreamableManager();
	AssetLoadHandle = StreamableManager.RequestAsyncLoad(
		NiagaraPathsToLoad,
		FStreamableDelegate::CreateUObject(this, &UER_AssetPreloadSubsystem::OnNiagaraAssetsLoadedAsync)
	);
}

void UER_AssetPreloadSubsystem::OnNiagaraAssetsLoadedAsync()
{
	if (AssetLoadHandle.IsValid())
	{
		TArray<UObject*> LoadedNiagaraAssets;
		AssetLoadHandle->GetLoadedAssets(LoadedNiagaraAssets);
		PreloadedAssets.Append(LoadedNiagaraAssets);
		
		UE_LOG(LogTemp, Log, TEXT("[Preload] Niagara Assets loading complete! Loaded %d systems. Preload process fully complete!"), LoadedNiagaraAssets.Num());

		if (UWorld* World = GetWorld())
		{
			int32 PreSpawnCount = 0;

			for (UObject* Asset : LoadedNiagaraAssets)
			{
				if (UNiagaraSystem* NiagaraSystem = Cast<UNiagaraSystem>(Asset))
				{
					UNiagaraComponent* TempComp = UNiagaraFunctionLibrary::SpawnSystemAtLocation(
						World,
						NiagaraSystem,
						FVector(0.f, 0.f, -100000.f),
						FRotator::ZeroRotator,
						FVector(1.f),
						true,
						true,
						ENCPoolMethod::AutoRelease
					);
					if (TempComp)
					{
						PreSpawnCount++;
					}
				}
			}
			UE_LOG(LogTemp, Log, TEXT("[Preload] Optimized %d Niagara systems via AutoRelease Pre-spawn (Total spawned: %d) to prevent rendering and instantiation hitching."), LoadedNiagaraAssets.Num(), PreSpawnCount);
		}

		AssetLoadHandle.Reset();
	}

	OnPreloadComplete.Broadcast();
}

void UER_AssetPreloadSubsystem::ShowLoadingScreen(TSubclassOf<UUserWidget> LoadingUIClass)
{
	if (!LoadingUIClass || LoadingUIInstance) return;

	LoadingUIInstance = CreateWidget<UUserWidget>(GetGameInstance(), LoadingUIClass);
	if (LoadingUIInstance)
	{
		if (GEngine && GEngine->GameViewport)
		{
			GEngine->GameViewport->AddViewportWidgetContent(LoadingUIInstance->TakeWidget(), 100);
		}
	}
}

void UER_AssetPreloadSubsystem::HideLoadingScreen()
{
	if (!LoadingUIInstance)
	{
		UE_LOG(LogTemp, Log, TEXT("[Preload] HideLoadingScreen: No active LoadingUIInstance to remove."));
		return;
	}

	if (GEngine && GEngine->GameViewport)
	{
		GEngine->GameViewport->RemoveViewportWidgetContent(LoadingUIInstance->TakeWidget());
	}

	LoadingUIInstance = nullptr;
}

void UER_AssetPreloadSubsystem::CollectNiagaraPathsFromMontage(const UAnimMontage* InMontage, TArray<FSoftObjectPath>& OutPaths) const
{
	if (InMontage == nullptr)
	{
		return;
	}

	for (const FAnimNotifyEvent& Event : InMontage->Notifies)
	{
		if (Event.Notify != nullptr)
		{
			if (const UAnimNotify_SkillGameplayCue* SkillNotify = Cast<UAnimNotify_SkillGameplayCue>(Event.Notify))
			{
				if (SkillNotify->GetSpawnConfig() && !SkillNotify->GetSpawnConfig()->NiagaraSystem.IsNull())
				{
					OutPaths.AddUnique(SkillNotify->GetSpawnConfig()->NiagaraSystem.ToSoftObjectPath());
				}
			}
			else if (const UAnimNotifyState_SkillGameplayCue* SkillNotifyState = Cast<UAnimNotifyState_SkillGameplayCue>(static_cast<UObject*>(Event.Notify)))
			{
				if (SkillNotifyState->GetSpawnConfig() && !SkillNotifyState->GetSpawnConfig()->NiagaraSystem.IsNull())
				{
					OutPaths.AddUnique(SkillNotifyState->GetSpawnConfig()->NiagaraSystem.ToSoftObjectPath());
				}
			}
		}
	}
}
