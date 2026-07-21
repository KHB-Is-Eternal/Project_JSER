#include "SkillSystem/GameplayCueNotify/Particle/VisionParticleManagerSubsystem.h"
#include "NiagaraComponent.h"
#include "LineOfSight/Management/Subsystem/LOSVisionSubsystem.h"
#include "LineOfSight/VisionComps/Vision_VisualComp.h"
#include "SkillSystem/GameplayCueNotify/AGCN_SummonedActor.h"
#include "SkillSystem/GameplayCueNotify/Components/GroundIndicatorComponent.h"

void UVisionParticleManagerSubsystem::RegisterParticle(UNiagaraComponent* Particle, AActor* TargetActor, bool bTrackUntilSeen)
{
	if (!IsValid(Particle) || !IsValid(TargetActor))
	{
		return;
	}

	// 등록 시점에 현재 시야 상태를 즉시 반영 (폴링 제거로 다음 틱 보정이 없으므로 여기서 확정)
	const bool bVisible = IsTargetVisibleToLocalPlayer(TargetActor);
	ApplyParticleVisibility(Particle, TargetActor, bVisible);

	// "한 번 보이면 끝까지 보임" 추적은 이미 보이는 순간 완료
	if (bVisible && bTrackUntilSeen)
	{
		return;
	}

	UVision_VisualComp* VisionComp = TargetActor->FindComponentByClass<UVision_VisualComp>();
	if (!IsValid(VisionComp))
	{
		// 이벤트 소스가 없으면 시야 상태가 변할 수 없으므로 추적하지 않음 (미부착 경고는 질의 API가 담당)
		return;
	}

	ManagedParticles.Emplace(Particle, TargetActor, bTrackUntilSeen);

	if (!BoundVisionComps.Contains(VisionComp))
	{
		VisionComp->OnTargetRevealed.AddDynamic(this, &UVisionParticleManagerSubsystem::OnVisionStateChanged);
		VisionComp->OnTargetHidden.AddDynamic(this, &UVisionParticleManagerSubsystem::OnVisionStateChanged);
		BoundVisionComps.Add(VisionComp);
	}
}

void UVisionParticleManagerSubsystem::OnVisionStateChanged()
{
	RefreshManagedParticles();
}

void UVisionParticleManagerSubsystem::RefreshManagedParticles()
{
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

		const bool bShouldBeVisible = IsTargetVisibleToLocalPlayer(Target);
		const bool bCurrentlyVisible = NC->GetVisibleFlag();

		if (bShouldBeVisible != bCurrentlyVisible)
		{
			ApplyParticleVisibility(NC, Target, bShouldBeVisible);
		}

		if (bShouldBeVisible && ManagedParticles[i].bTrackUntilSeen)
		{
			ManagedParticles.RemoveAtSwap(i);
		}
	}
}

bool UVisionParticleManagerSubsystem::IsTargetVisibleToLocalPlayer(const AActor* Target) const
{
	if (const ULOSVisionSubsystem* VisionSubsystem = GetWorld()->GetSubsystem<ULOSVisionSubsystem>())
	{
		return VisionSubsystem->IsActorVisibleToLocalPlayer(Target);
	}
	return true;
}

void UVisionParticleManagerSubsystem::ApplyParticleVisibility(UNiagaraComponent* Particle, AActor* Target, bool bVisible)
{
	Particle->SetVisibility(bVisible);
	Particle->SetHiddenInGame(!bVisible);

	if (AGCN_SummonedActor* SummonedActor = Cast<AGCN_SummonedActor>(Target))
	{
		if (SummonedActor->CollisionIndicatorComp)
		{
			SummonedActor->CollisionIndicatorComp->SetVisibility(bVisible, true);
			SummonedActor->CollisionIndicatorComp->SetHiddenInGame(!bVisible, true);
		}
	}
}
