#include "CharacterSystem/GAS/ProjectERASC.h"
#include "GameplayCueInterface.h"

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
