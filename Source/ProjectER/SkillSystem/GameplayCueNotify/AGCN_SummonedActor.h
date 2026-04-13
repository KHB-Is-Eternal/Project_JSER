#pragma once

#include "CoreMinimal.h"
#include "GameplayCueNotify_Actor.h"
#include "AGCN_SummonedActor.generated.h"

/**
 * 소환물 비주얼을 담당하며 예측 키를 통해 판정 액터와 동기화되는 GCN 액터
 */
UCLASS()
class PROJECTER_API AGCN_SummonedActor : public AGameplayCueNotify_Actor
{
	GENERATED_BODY()

public:
	AGCN_SummonedActor();

protected:
	virtual bool OnExecute_Implementation(AActor* MyTarget, const FGameplayCueParameters& Parameters) override;
	virtual bool WhileActive_Implementation(AActor* MyTarget, const FGameplayCueParameters& Parameters) override;
	virtual bool OnRemove_Implementation(AActor* MyTarget, const FGameplayCueParameters& Parameters) override;

	/** 공통 등록 및 초기화 로직 */
	void HandleSummonedVfx(const FGameplayCueParameters& Parameters);

	/** GEC 데이터로부터 속성 초기화 */
	void InitializeFromGEC(const UObject* SourceObject);

public:
	/** 캐싱된 GEC 데이터를 반환합니다. */
	const UObject* GetSourceObject() const { return CachedSourceObject.Get(); }

private:
	/** 비주얼/물리 설정값이 담긴 GEC 객체 */
	UPROPERTY()
	TWeakObjectPtr<UObject> CachedSourceObject;
};
