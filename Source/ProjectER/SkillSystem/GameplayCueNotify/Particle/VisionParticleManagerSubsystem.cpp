#include "SkillSystem/GameplayCueNotify/Particle/VisionParticleManagerSubsystem.h"
#include "NiagaraComponent.h"
#include "LineOfSight/VisionComps/Vision_VisualComp.h"
#include "SkillSystem/GameplayCueNotify/AGCN_SummonedActor.h"
#include "SkillSystem/GameplayCueNotify/Components/GroundIndicatorComponent.h"

void UVisionParticleManagerSubsystem::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	// [Optimization] 10 FPS 제한 (0.1초마다 실행)
	TickAccumulator += DeltaTime;
	if (TickAccumulator < 0.1f)
	{
		return;
	}
	// 초과분을 남기지 않고 0으로 초기화하여 정확히 프레임을 건너뛰게 함
	TickAccumulator = 0.0f;

	for (int32 i = ManagedParticles.Num() - 1; i >= 0; --i)
	{
		UNiagaraComponent* NC = ManagedParticles[i].ParticleComp.Get();
		AActor* Target = ManagedParticles[i].VisionTarget.Get();

		// 파티클이나 타겟이 파괴되었으면 리스트에서 안전하게 제거
		if (!IsValid(NC) || !IsValid(Target))
		{
			ManagedParticles.RemoveAtSwap(i);
			continue;
		}

		if (UVision_VisualComp* VisionComp = Target->FindComponentByClass<UVision_VisualComp>())
		{
			const float CurrentAlpha = VisionComp->GetVisibilityAlpha();
			const bool bShouldBeVisible = (CurrentAlpha > 0.0f);
			const bool bCurrentlyVisible = NC->GetVisibleFlag();

			if (bShouldBeVisible)
			{
				if (!bCurrentlyVisible)
				{
					NC->SetVisibility(true);
					NC->SetHiddenInGame(false);

					if (AGCN_SummonedActor* SummonedActor = Cast<AGCN_SummonedActor>(Target))
					{
						if (SummonedActor->CollisionIndicatorComp)
						{
							SummonedActor->CollisionIndicatorComp->SetVisibility(true, true);
							SummonedActor->CollisionIndicatorComp->SetHiddenInGame(false, true);
						}
					}
				}

				if (ManagedParticles[i].bTrackUntilSeen)
				{
					ManagedParticles.RemoveAtSwap(i);
					continue;
				}
			}
			else
			{
				if (bCurrentlyVisible)
				{
					NC->SetVisibility(false);
					NC->SetHiddenInGame(true);

					if (AGCN_SummonedActor* SummonedActor = Cast<AGCN_SummonedActor>(Target))
					{
						if (SummonedActor->CollisionIndicatorComp)
						{
							SummonedActor->CollisionIndicatorComp->SetVisibility(false, true);
							SummonedActor->CollisionIndicatorComp->SetHiddenInGame(true, true);
						}
					}
				}
			}
		}
	}
}

TStatId UVisionParticleManagerSubsystem::GetStatId() const
{
	RETURN_QUICK_DECLARE_CYCLE_STAT(UVisionParticleManagerSubsystem, STATGROUP_Tickables);
}

void UVisionParticleManagerSubsystem::RegisterParticle(UNiagaraComponent* Particle, AActor* TargetActor, bool bTrackUntilSeen)
{
	if (!IsValid(Particle) || !IsValid(TargetActor))
	{
		return;
	}

	ManagedParticles.Emplace(Particle, TargetActor, bTrackUntilSeen);
}
