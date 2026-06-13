// Fill out your copyright notice in the Description page of Project Settings.


#include "SkillSystem/AnimNotify/AnimNotifyCueTrackerComponent.h"
#include "NiagaraComponent.h"
#include "NiagaraSystem.h"
#include "Components/AudioComponent.h"
#include "Sound/SoundBase.h"
#include "Animation/AnimInstance.h"
#include "Animation/AnimMontage.h"
#include "SkillSystem/GameplayCueNotify/Particle/SkillNiagaraSpawnConfig.h"
#include "SkillSystem/GameplayCueNotify/Sound/SkillSoundSpawnConfig.h"

#if WITH_EDITOR

// 몽타주 일시정지 판별 헬퍼
static bool IsMontagePaused(USkeletalMeshComponent* MeshComp, UAnimMontage* Montage)
{
	if (IsValid(MeshComp) && IsValid(Montage))
	{
		UAnimInstance* AnimInstance = MeshComp->GetAnimInstance();
		if (IsValid(AnimInstance))
		{
			FAnimMontageInstance* MontageInstance = AnimInstance->GetActiveInstanceForMontage(Montage);
			if (MontageInstance)
			{
				return !MontageInstance->bPlaying || 
				       (MontageInstance->GetPlayRate() == 0.0f) || 
				       !MeshComp->IsPlaying();
			}
		}
	}
	return true;
}

// 부착된 모든 컴포넌트 (부착된 큐 액터 내부 포함) 검색 헬퍼
template <typename T>
static void FindAllAttachedComponents(USkeletalMeshComponent* MeshComp, TArray<T*>& OutComps)
{
	if (!IsValid(MeshComp)) return;

	// 1. Owner Actor 및 자식 컴포넌트, 그리고 Owner에 부착된 자식 액터들의 컴포넌트까지 순회
	AActor* Owner = MeshComp->GetOwner();
	if (IsValid(Owner))
	{
		// 액터 자체의 컴포넌트
		TArray<T*> ActorComps;
		Owner->GetComponents<T>(ActorComps);
		for (T* Comp : ActorComps)
		{
			OutComps.AddUnique(Comp);
		}

		// 부착된 액터(예: AGameplayCueNotify_Looping 등)의 컴포넌트
		TArray<AActor*> AttachedActors;
		Owner->GetAttachedActors(AttachedActors);
		for (AActor* Attached : AttachedActors)
		{
			if (IsValid(Attached))
			{
				TArray<T*> AttachedComps;
				Attached->GetComponents<T>(AttachedComps);
				for (T* Comp : AttachedComps)
				{
					OutComps.AddUnique(Comp);
				}
			}
		}
	}

	// 2. MeshComp 자체에 붙은 컴포넌트 확인 (Editor 프리뷰 환경 등 Owner가 없을 때 대비)
	TArray<USceneComponent*> Children;
	MeshComp->GetChildrenComponents(true, Children);
	for (USceneComponent* Child : Children)
	{
		if (T* TypedComp = Cast<T>(Child))
		{
			OutComps.AddUnique(TypedComp);
		}
	}
}

#endif // WITH_EDITOR

UAnimNotifyCueTrackerComponent::UAnimNotifyCueTrackerComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.bStartWithTickEnabled = true;

#if !WITH_EDITOR
	// 에디터가 아닌 실제 인게임 런타임에서는 틱을 아예 끕니다.
	PrimaryComponentTick.bCanEverTick = false;
	PrimaryComponentTick.bStartWithTickEnabled = false;
#endif
}

void UAnimNotifyCueTrackerComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

#if WITH_EDITOR
	// Dedicated Server 또는 실제 게임 플레이 월드(PIE, Standalone)인 경우 에디터용 트래킹 틱 생략
	if (GetNetMode() == NM_DedicatedServer)
	{
		return;
	}

	if (UWorld* World = GetWorld())
	{
		if (World->IsGameWorld())
		{
			return;
		}
	}

	AActor* Owner = GetOwner();
	if (!IsValid(Owner))
	{
		return;
	}

	// 1. PendingCues 대기 목록 검사 및 실제 추적 등록
	for (int32 i = PendingCues.Num() - 1; i >= 0; --i)
	{
		FPendingTrackedCue& Pending = PendingCues[i];
		if (!Pending.MeshComponent.IsValid() || !Pending.TargetMontage.IsValid() || !Pending.TargetAsset.IsValid())
		{
			PendingCues.RemoveAt(i);
			continue;
		}

		USkeletalMeshComponent* MeshComp = Pending.MeshComponent.Get();
		UAnimMontage* Montage = Pending.TargetMontage.Get();
		bool bFound = false;

		if (UNiagaraSystem* TargetSystem = Cast<UNiagaraSystem>(Pending.TargetAsset.Get()))
		{
			TArray<UNiagaraComponent*> NiagaraComps;
			FindAllAttachedComponents(MeshComp, NiagaraComps);
			
			for (int32 c = NiagaraComps.Num() - 1; c >= 0; --c)
			{
				UNiagaraComponent* NC = NiagaraComps[c];
				if (IsValid(NC) && NC->GetAsset() == TargetSystem && NC->IsActive())
				{
					bool bAlreadyTracked = false;
					for (const FTrackedCueComponent& Tracked : TrackedComponents)
					{
						if (Tracked.NiagaraComponent == NC)
						{
							bAlreadyTracked = true;
							break;
						}
					}

					if (!bAlreadyTracked)
					{
						FTrackedCueComponent NewTracked;
						NewTracked.TargetMontage = Pending.TargetMontage;
						NewTracked.MeshComponent = Pending.MeshComponent;
						NewTracked.NiagaraComponent = NC;
						NewTracked.bWasPaused = IsMontagePaused(Pending.MeshComponent.Get(), Pending.TargetMontage.Get());
						TrackedComponents.Add(NewTracked);
						bFound = true;
						break; // 한 번 찾았으면 더 이상 중복 등록 방지 (가장 최신 것만)
					}
				}
			}
		}
		else if (USoundBase* TargetSound = Cast<USoundBase>(Pending.TargetAsset.Get()))
		{
			TArray<UAudioComponent*> AudioComps;
			FindAllAttachedComponents(MeshComp, AudioComps);

			for (int32 c = AudioComps.Num() - 1; c >= 0; --c)
			{
				UAudioComponent* AC = AudioComps[c];
				if (IsValid(AC) && AC->Sound == TargetSound && AC->IsPlaying())
				{
					bool bAlreadyTracked = false;
					for (const FTrackedCueComponent& Tracked : TrackedComponents)
					{
						if (Tracked.AudioComponent == AC)
						{
							bAlreadyTracked = true;
							break;
						}
					}

					if (!bAlreadyTracked)
					{
						FTrackedCueComponent NewTracked;
						NewTracked.TargetMontage = Pending.TargetMontage;
						NewTracked.MeshComponent = Pending.MeshComponent;
						NewTracked.AudioComponent = AC;
						NewTracked.bWasPaused = IsMontagePaused(Pending.MeshComponent.Get(), Pending.TargetMontage.Get());
						TrackedComponents.Add(NewTracked);
						bFound = true;
						break; // 한 번 찾았으면 중복 등록 방지 (가장 최신 것만)
					}
				}
			}
		}

		if (bFound)
		{
			PendingCues.RemoveAt(i);
		}
	}

	// 2. TrackedComponents 목록 상태 업데이트 및 일시정지 동기화
	for (int32 i = TrackedComponents.Num() - 1; i >= 0; --i)
	{
		FTrackedCueComponent& Tracked = TrackedComponents[i];

		bool bCompValid = false;
		if (Tracked.NiagaraComponent.IsValid())
		{
			bCompValid = IsValid(Tracked.NiagaraComponent.Get()) && Tracked.NiagaraComponent->IsActive();
		}
		else if (Tracked.AudioComponent.IsValid())
		{
			bCompValid = IsValid(Tracked.AudioComponent.Get()) && Tracked.AudioComponent->IsPlaying();
		}

		if (!bCompValid || !Tracked.MeshComponent.IsValid() || !Tracked.TargetMontage.IsValid())
		{
			TrackedComponents.RemoveAt(i);
			continue;
		}

		USkeletalMeshComponent* MeshComp = Tracked.MeshComponent.Get();
		UAnimMontage* Montage = Tracked.TargetMontage.Get();

		bool bIsPaused = IsMontagePaused(MeshComp, Montage);

		// 일시정지 상태 동기화
		if (Tracked.bWasPaused != bIsPaused)
		{
			Tracked.bWasPaused = bIsPaused;
			if (Tracked.NiagaraComponent.IsValid())
			{
				Tracked.NiagaraComponent->SetPaused(bIsPaused);
			}
			if (Tracked.AudioComponent.IsValid())
			{
				Tracked.AudioComponent->SetPaused(bIsPaused);
			}
		}
	}

	// 3. 더 이상 추적 및 대기 중인 대상이 없을 경우 컴포넌트 자동 정리
	if (TrackedComponents.Num() == 0 && PendingCues.Num() == 0)
	{
		DestroyComponent();
	}
#endif // WITH_EDITOR
}

#if WITH_EDITOR

UAnimNotifyCueTrackerComponent* UAnimNotifyCueTrackerComponent::GetOrCreateTracker(AActor* Owner)
{
	if (!IsValid(Owner))
	{
		return nullptr;
	}

	// 실제 게임 월드(PIE, Standalone)일 때는 트래킹 컴포넌트를 아예 생성하지 않습니다.
	if (UWorld* World = Owner->GetWorld())
	{
		if (World->IsGameWorld())
		{
			return nullptr;
		}
	}

	UAnimNotifyCueTrackerComponent* Tracker = Owner->FindComponentByClass<UAnimNotifyCueTrackerComponent>();
	if (!Tracker)
	{
		Tracker = NewObject<UAnimNotifyCueTrackerComponent>(Owner, UAnimNotifyCueTrackerComponent::StaticClass());
		if (Tracker)
		{
			Tracker->RegisterComponent();
		}
	}
	return Tracker;
}

void UAnimNotifyCueTrackerComponent::RegisterNiagaraCue(USkeletalMeshComponent* MeshComp, UAnimMontage* Montage, USkillNiagaraSpawnConfig* SpawnConfig)
{
	if (UWorld* World = GetWorld())
	{
		if (World->IsGameWorld())
		{
			return;
		}
	}

	if (!IsValid(MeshComp) || !IsValid(Montage) || !IsValid(SpawnConfig) || SpawnConfig->NiagaraSystem.IsNull())
	{
		return;
	}

	UNiagaraSystem* TargetSystem = SpawnConfig->NiagaraSystem.LoadSynchronous();
	if (!IsValid(TargetSystem))
	{
		return;
	}

	// 에디터 스크러빙 등으로 인해 NotifyEnd를 타지 못하고 남아버린 고아(Orphan) 큐 정리
	for (int32 i = TrackedComponents.Num() - 1; i >= 0; --i)
	{
		if (TrackedComponents[i].MeshComponent == MeshComp && TrackedComponents[i].TargetMontage == Montage)
		{
			if (TrackedComponents[i].NiagaraComponent.IsValid() && TrackedComponents[i].NiagaraComponent->GetAsset() == TargetSystem)
			{
				TrackedComponents[i].NiagaraComponent->SetVisibility(false);
				TrackedComponents[i].NiagaraComponent->Deactivate();
				TrackedComponents.RemoveAt(i);
			}
		}
	}

	// 먼저 대기 목록에 추가
	FPendingTrackedCue Pending;
	Pending.TargetMontage = Montage;
	Pending.MeshComponent = MeshComp;
	Pending.TargetAsset = TargetSystem;
	PendingCues.Add(Pending);

	// 즉시 스폰되었는지 확인하여 찾으면 추적 및 대기 목록 갱신
	TArray<UNiagaraComponent*> NiagaraComps;
	FindAllAttachedComponents(MeshComp, NiagaraComps);

	for (int32 c = NiagaraComps.Num() - 1; c >= 0; --c)
	{
		UNiagaraComponent* NC = NiagaraComps[c];
		if (IsValid(NC) && NC->GetAsset() == TargetSystem && NC->IsActive())
		{
			bool bAlreadyTracked = false;
			for (const FTrackedCueComponent& Tracked : TrackedComponents)
			{
				if (Tracked.NiagaraComponent == NC)
				{
					bAlreadyTracked = true;
					break;
				}
			}

			if (!bAlreadyTracked)
			{
				FTrackedCueComponent NewTracked;
				NewTracked.TargetMontage = Montage;
				NewTracked.MeshComponent = MeshComp;
				NewTracked.NiagaraComponent = NC;
				NewTracked.bWasPaused = IsMontagePaused(MeshComp, Montage);
				TrackedComponents.Add(NewTracked);
				
				// 대기 목록에서 방금 추가한 항목 제거
				for (int32 j = PendingCues.Num() - 1; j >= 0; --j)
				{
					if (PendingCues[j].TargetAsset == TargetSystem && PendingCues[j].MeshComponent == MeshComp)
					{
						PendingCues.RemoveAt(j);
						break;
					}
				}
				break; // 가장 최신 스폰된 한 개만 등록 보장
			}
		}
	}
}

void UAnimNotifyCueTrackerComponent::RegisterSoundCue(USkeletalMeshComponent* MeshComp, UAnimMontage* Montage, USkillSoundSpawnConfig* SpawnConfig)
{
	if (UWorld* World = GetWorld())
	{
		if (World->IsGameWorld())
		{
			return;
		}
	}

	if (!IsValid(MeshComp) || !IsValid(Montage) || !IsValid(SpawnConfig) || SpawnConfig->Sound.IsNull())
	{
		return;
	}

	USoundBase* TargetSound = SpawnConfig->Sound.LoadSynchronous();
	if (!IsValid(TargetSound))
	{
		return;
	}

	// 에디터 스크러빙 고아 큐 정리
	for (int32 i = TrackedComponents.Num() - 1; i >= 0; --i)
	{
		if (TrackedComponents[i].MeshComponent == MeshComp && TrackedComponents[i].TargetMontage == Montage)
		{
			if (TrackedComponents[i].AudioComponent.IsValid() && TrackedComponents[i].AudioComponent->Sound == TargetSound)
			{
				TrackedComponents[i].AudioComponent->Stop();
				TrackedComponents.RemoveAt(i);
			}
		}
	}

	// 먼저 대기 목록에 추가
	FPendingTrackedCue Pending;
	Pending.TargetMontage = Montage;
	Pending.MeshComponent = MeshComp;
	Pending.TargetAsset = TargetSound;
	PendingCues.Add(Pending);

	// 즉시 스폰되었는지 확인하여 찾으면 추적 및 대기 목록 갱신
	TArray<UAudioComponent*> AudioComps;
	FindAllAttachedComponents(MeshComp, AudioComps);

	for (int32 c = AudioComps.Num() - 1; c >= 0; --c)
	{
		UAudioComponent* AC = AudioComps[c];
		if (IsValid(AC) && AC->Sound == TargetSound && AC->IsPlaying())
		{
			bool bAlreadyTracked = false;
			for (const FTrackedCueComponent& Tracked : TrackedComponents)
			{
				if (Tracked.AudioComponent == AC)
				{
					bAlreadyTracked = true;
					break;
				}
			}

			if (!bAlreadyTracked)
			{
				FTrackedCueComponent NewTracked;
				NewTracked.TargetMontage = Montage;
				NewTracked.MeshComponent = MeshComp;
				NewTracked.AudioComponent = AC;
				NewTracked.bWasPaused = IsMontagePaused(MeshComp, Montage);
				TrackedComponents.Add(NewTracked);

				// 대기 목록에서 방금 추가한 항목 제거
				for (int32 j = PendingCues.Num() - 1; j >= 0; --j)
				{
					if (PendingCues[j].TargetAsset == TargetSound && PendingCues[j].MeshComponent == MeshComp)
					{
						PendingCues.RemoveAt(j);
						break;
					}
				}
				break; // 가장 최신 한 개만 등록 보장
			}
		}
	}
}

void UAnimNotifyCueTrackerComponent::UnregisterCue(USkeletalMeshComponent* MeshComp, UObject* TargetAsset)
{
	if (UWorld* World = GetWorld())
	{
		if (World->IsGameWorld())
		{
			return;
		}
	}

	if (!IsValid(MeshComp) || !IsValid(TargetAsset))
	{
		return;
	}

	// 대기 목록에서 제거
	for (int32 i = PendingCues.Num() - 1; i >= 0; --i)
	{
		if (PendingCues[i].MeshComponent == MeshComp && PendingCues[i].TargetAsset == TargetAsset)
		{
			PendingCues.RemoveAt(i);
		}
	}

	// 추적 목록에서 제거 및 일시정지 해제 보장
	for (int32 i = TrackedComponents.Num() - 1; i >= 0; --i)
	{
		if (TrackedComponents[i].MeshComponent == MeshComp)
		{
			bool bMatch = false;
			if (TrackedComponents[i].NiagaraComponent.IsValid() && TrackedComponents[i].NiagaraComponent->GetAsset() == TargetAsset)
			{
				TrackedComponents[i].NiagaraComponent->SetPaused(false);
				bMatch = true;
			}
			else if (TrackedComponents[i].AudioComponent.IsValid() && TrackedComponents[i].AudioComponent->Sound == TargetAsset)
			{
				TrackedComponents[i].AudioComponent->SetPaused(false);
				bMatch = true;
			}

			if (bMatch)
			{
				TrackedComponents.RemoveAt(i);
			}
		}
	}
}

#endif // WITH_EDITOR
