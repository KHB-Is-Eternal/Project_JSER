#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "NiagaraComponent.h"
#include "GameFramework/Actor.h"
#include "VisionParticleManagerSubsystem.generated.h"

class UVision_VisualComp;

USTRUCT()
struct FManagedVisionParticle
{
	GENERATED_BODY()

	UPROPERTY()
	TWeakObjectPtr<UNiagaraComponent> ParticleComp;

	UPROPERTY()
	TWeakObjectPtr<AActor> VisionTarget;

	UPROPERTY()
	bool bTrackUntilSeen = false;

	FManagedVisionParticle() {}
	FManagedVisionParticle(UNiagaraComponent* InParticle, AActor* InTarget, bool bInTrackUntilSeen = false)
		: ParticleComp(InParticle), VisionTarget(InTarget), bTrackUntilSeen(bInTrackUntilSeen) {}
};

/**
 * 지속형 파티클들이 시야(Fog of War)에 들어오거나 나갈 때 실시간으로 보이고 숨겨지도록 관리하는 서브시스템.
 * 폴링(틱) 대신 타겟의 Vision_VisualComp 델리게이트(OnTargetRevealed/Hidden)를 구독해 동작한다. (006 합-2)
 */
UCLASS()
class PROJECTER_API UVisionParticleManagerSubsystem : public UWorldSubsystem
{
	GENERATED_BODY()

public:
	/**
	 * 파티클을 등록하여 시야 상태에 따라 자동으로 가시성을 토글하도록 만듭니다.
	 * @param Particle 등록할 파티클 컴포넌트
	 * @param TargetActor 시야 판정의 기준이 되는 액터 (보통 MyTarget 또는 Instigator)
	 */
	UFUNCTION(BlueprintCallable, Category = "Skill VFX")
	void RegisterParticle(UNiagaraComponent* Particle, AActor* TargetActor, bool bTrackUntilSeen = false);

private:
	// 시야 델리게이트 핸들러 — FOcclusionTracerEvent는 발신자를 전달하지 않으므로 관리 목록 전체를 재검사한다
	UFUNCTION()
	void OnVisionStateChanged();

	void RefreshManagedParticles();
	bool IsTargetVisibleToLocalPlayer(const AActor* Target) const;
	static void ApplyParticleVisibility(UNiagaraComponent* Particle, AActor* Target, bool bVisible);

	TArray<FManagedVisionParticle> ManagedParticles;

	/** 델리게이트 중복 바인딩 방지 (같은 타겟에 파티클 여러 개 등록 가능) */
	TSet<TWeakObjectPtr<UVision_VisualComp>> BoundVisionComps;
};
