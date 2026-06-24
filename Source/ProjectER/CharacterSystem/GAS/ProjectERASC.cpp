#include "CharacterSystem/GAS/ProjectERASC.h"
#include "GameplayCueInterface.h"
#include "Engine/World.h"
#include "TimerManager.h"

void UProjectERASC::RemoveGameplayCueBySource(const FGameplayTag& GameplayCueTag, const UObject* SourceObject)
{
	if (IsOwnerActorAuthoritative())
	{
		bool bRemoved = false;
		
		// 역순 순회하여 특정 SourceObject를 가진 엔트리만 삭제
		for (int32 idx = ActiveGameplayCues.GameplayCues.Num() - 1; idx >= 0; --idx)
		{
			FActiveGameplayCue& Cue = ActiveGameplayCues.GameplayCues[idx];
			if (Cue.GameplayCueTag == GameplayCueTag && Cue.Parameters.SourceObject == SourceObject)
			{
				UpdateTagMap(GameplayCueTag, -1, EGameplayTagReplicationState::None);
				InvokeGameplayCueEvent(GameplayCueTag, EGameplayCueEvent::Removed, Cue.Parameters);
				ActiveGameplayCues.GameplayCues.RemoveAt(idx);
				bRemoved = true;
				break; // 해당 SourceObject에 대한 큐는 하나뿐일 것이므로 중단
			}
		}

		if (bRemoved)
		{
			ActiveGameplayCues.MarkArrayDirty();
			ForceReplication();
		}
	}
	else if (ScopedPredictionKey.IsLocalClientKey())
	{
		// 클라이언트 예측적 제거 (PredictiveRemove)
		for (int32 idx = 0; idx < ActiveGameplayCues.GameplayCues.Num(); ++idx)
		{
			FActiveGameplayCue& Cue = ActiveGameplayCues.GameplayCues[idx];
			if (Cue.GameplayCueTag == GameplayCueTag && Cue.Parameters.SourceObject == SourceObject && !Cue.bPredictivelyRemoved)
			{
				Cue.bPredictivelyRemoved = true;
				UpdateTagMap(GameplayCueTag, -1, EGameplayTagReplicationState::None);
				InvokeGameplayCueEvent(GameplayCueTag, EGameplayCueEvent::Removed, Cue.Parameters);
				break;
			}
		}
	}
}

void UProjectERASC::InitAbilityActorInfo(AActor* InOwnerActor, AActor* InAvatarActor)
{
	Super::InitAbilityActorInfo(InOwnerActor, InAvatarActor);

	// 클라이언트에서만 주기적으로 유령 GE 청소기 가동 (예: 1.5초마다)
	if (IsNetMode(NM_Client) && GetAvatarActor() != nullptr)
	{
		if (UWorld* World = GetWorld())
		{
			if (!World->GetTimerManager().IsTimerActive(GhostGECleanupTimerHandle))
			{
				World->GetTimerManager().SetTimer(GhostGECleanupTimerHandle, this, &UProjectERASC::CleanupGhostGameplayEffects, 2.0f, true);
			}
		}
	}
}

void UProjectERASC::OnComponentDestroyed(bool bDestroyingHierarchy)
{
	if (GhostGECleanupTimerHandle.IsValid())
	{
		if (UWorld* World = GetWorld())
		{
			World->GetTimerManager().ClearTimer(GhostGECleanupTimerHandle);
		}
	}

	Super::OnComponentDestroyed(bDestroyingHierarchy);
}

void UProjectERASC::CleanupGhostGameplayEffects()
{
	if (UWorld* World = GetWorld())
	{
		float CurrentTime = World->GetTimeSeconds();
		TArray<FActiveGameplayEffectHandle> HandlesToRemove;

		FGameplayEffectQuery Query;
		TArray<FActiveGameplayEffectHandle> ActiveHandles = GetActiveEffects(Query);

		for (const FActiveGameplayEffectHandle& Handle : ActiveHandles)
		{
			const FActiveGameplayEffect* ActiveGE = GetActiveGameplayEffect(Handle);
			if (ActiveGE && ActiveGE->GetDuration() > 0.0f)
			{
				// 통신 지연 오차를 고려해 -0.5초(500ms) 여유를 둡니다.
				if (ActiveGE->GetTimeRemaining(CurrentTime) < -0.5f)
				{
					HandlesToRemove.Add(Handle);
				}
			}
		}

		for (const FActiveGameplayEffectHandle& Handle : HandlesToRemove)
		{
			// 클라이언트 사이드에서 강제 제거 (스택 모두 제거)
			RemoveActiveGameplayEffect(Handle, -1);
		}

		// 2. 유령 태그(Orphaned Tags) 강제 정리
		// GE는 정상적으로 지워졌으나, Prediction Key 롤백 실패 등의 버그로 태그 카운트만 붕 떠있는 경우를 해결합니다.
		for (const FGameplayAbilitySpec& Spec : GetActivatableAbilities())
		{
			if (const UGameplayAbility* Ability = Spec.Ability)
			{
				// USkillBase로 캐스팅하지 않고 언리얼 기본 GetCooldownTags를 호출하여 범용성을 확보합니다.
				if (const FGameplayTagContainer* CooldownTags = Ability->GetCooldownTags())
				{
					for (const FGameplayTag& Tag : *CooldownTags)
					{
						if (GetTagCount(Tag) > 0)
						{
							// 이 태그를 실제로 부여하고 있는 활성화된 GE가 있는지 검색합니다.
							FGameplayEffectQuery TagQuery = FGameplayEffectQuery::MakeQuery_MatchAnyOwningTags(FGameplayTagContainer(Tag));
							TArray<FActiveGameplayEffectHandle> ActiveGEs = GetActiveEffects(TagQuery);
							
							// 태그 카운트는 > 0 인데, 이 태그를 주는 GE가 단 하나도 없다면 완벽한 유령 태그입니다.
							if (ActiveGEs.Num() == 0)
							{
								// 태그 카운트를 강제로 0으로 리셋합니다.
								SetTagMapCount(Tag, 0);
							}
						}
					}
				}
			}
		}
	}
}
