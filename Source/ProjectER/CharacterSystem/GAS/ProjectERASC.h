#pragma once

#include "CoreMinimal.h"
#include "AbilitySystemComponent.h"
#include "ProjectERASC.generated.h"

UCLASS()
class PROJECTER_API UProjectERASC : public UAbilitySystemComponent
{
	GENERATED_BODY()

public:
	/**
	 * SourceObject가 일치하는 특정 GameplayCue 엔트리만 선택적으로 제거합니다.
	 * 엔진 기본 RemoveGameplayCue는 같은 태그의 모든 엔트리를 삭제하므로,
	 * 동일 태그를 공유하는 다른 이펙트가 영향받지 않도록 이 함수를 사용합니다.
	 */
	void RemoveGameplayCueBySource(const FGameplayTag& GameplayCueTag, const UObject* SourceObject);

	virtual void InitAbilityActorInfo(AActor* InOwnerActor, AActor* InAvatarActor) override;

protected:
	virtual void OnComponentDestroyed(bool bDestroyingHierarchy) override;

	// 유령 쿨타임 태그 정리를 위한 클리너
	void CleanupGhostGameplayEffects();

	FTimerHandle GhostGECleanupTimerHandle;

	/**
	 * [Guard] 쿨다운 GE가 제거된 직후, 해당 GE가 부여했던 스킬 쿨다운 태그가
	 * (다른 부여 GE 없이) 카운트만 남아있으면 즉시 0으로 리셋합니다.
	 * 청소기(2초 주기)와 달리 GE 제거 시점에 확정적으로 동작하는 최후 방어선입니다.
	 */
	void OnAnyGERemoved_CooldownGuard(const FActiveGameplayEffect& RemovedGE);
	bool bCooldownGuardBound = false;
};
