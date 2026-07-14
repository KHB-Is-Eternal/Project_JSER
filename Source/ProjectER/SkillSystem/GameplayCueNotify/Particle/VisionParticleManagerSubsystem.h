#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "NiagaraComponent.h"
#include "GameFramework/Actor.h"
#include "VisionParticleManagerSubsystem.generated.h"

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
 * 지속형 파티클들이 시야(Fog of War)에 들어오거나 나갈 때 실시간으로 보이고 숨겨지도록 관리하는 서브시스템
 */
UCLASS()
class PROJECTER_API UVisionParticleManagerSubsystem : public UTickableWorldSubsystem
{
	GENERATED_BODY()

public:
	virtual void Tick(float DeltaTime) override;
	virtual TStatId GetStatId() const override;

	/**
	 * 파티클을 등록하여 시야 상태에 따라 자동으로 가시성을 토글하도록 만듭니다.
	 * @param Particle 등록할 파티클 컴포넌트
	 * @param TargetActor 시야 판정의 기준이 되는 액터 (보통 MyTarget 또는 Instigator)
	 */
	UFUNCTION(BlueprintCallable, Category = "Skill VFX")
	void RegisterParticle(UNiagaraComponent* Particle, AActor* TargetActor, bool bTrackUntilSeen = false);

private:
	TArray<FManagedVisionParticle> ManagedParticles;

	/** 10 FPS 제한용 누적 타이머 */
	float TickAccumulator = 0.0f;
};
