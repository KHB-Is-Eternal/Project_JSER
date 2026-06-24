#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "GameplayCue_Types.h"
#include "SkillVfxCullingHelper.generated.h"

UENUM(BlueprintType)
enum class EVfxCullState : uint8
{
	SpawnAndIgnoreVision,         // 무조건 스폰하고 시야 추적 안 함 (아군 파티클 등 - 항상 보임)
	SpawnAndTrackVision,          // 정상 스폰 후 시야 추적 함 (장판형 스킬 - 안개로 가면 꺼짐)
	SpawnAndTrackVisionUntilSeen, // 스폰 후 시야에 보일 때까지만 추적 함 (발사체형 스킬 - 한 번 보인 후에는 안개로 가도 유지)
	SpawnHidden,                  // 숨겨진 상태로 스폰 후 시야 추적 함 (현재 안개 속에 스폰되는 적군 지속형 파티클)
	SkipSpawn                     // 스폰 아예 생략 (현재 안개 속에 스폰되는 적군 단발성 파티클)
};

/**
 * 전역 VFX 최적화(컬링) 판별 헬퍼
 * 시야(Fog of War) 및 거리, 팀 판별 로직을 일원화하여 파티클의 스폰 여부와 초기 상태를 결정합니다.
 */
UCLASS()
class PROJECTER_API USkillVfxCullingHelper : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	/**
	 * 파티클 스폰 전 호출하여 스폰 상태를 판별합니다.
	 * @param TargetActor 이벤트를 받은 대상 액터
	 * @param Parameters GCN에서 전달받은 파라미터 
	 * @param bIsPersistent 지속형 파티클인지 여부 (지속형이면 시야 밖이라도 스폰하여 숨김 상태로 둠)
	 */
	UFUNCTION(BlueprintCallable, Category = "Skill VFX")
	static EVfxCullState CheckVfxCulling(const AActor* TargetActor, const FGameplayCueParameters& Parameters, bool bIsPersistent);
};
