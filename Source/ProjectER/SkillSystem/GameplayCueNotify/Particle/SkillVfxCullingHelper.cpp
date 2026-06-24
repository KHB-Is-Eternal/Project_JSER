#include "SkillSystem/GameplayCueNotify/Particle/SkillVfxCullingHelper.h"
#include "LineOfSight/VisionComps/Vision_VisualComp.h"
#include "CharacterSystem/Interface/TargetableInterface.h"
#include "GameFramework/ProjectileMovementComponent.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/Pawn.h"
#include "Engine/World.h"
#include "SkillSystem/Actor/BaseMissileActor/BaseMissileActor.h"

constexpr float MaxParticleSpawnDistanceSq = 1000.0f * 1000.0f;

EVfxCullState USkillVfxCullingHelper::CheckVfxCulling(const AActor* TargetActor, const FGameplayCueParameters& Parameters, bool bIsPersistent)
{
	// 1. 기본 무조건 스폰 예외 상황 체크 (아군이거나 투사체인 경우 등)
	const UWorld* World = IsValid(TargetActor) ? TargetActor->GetWorld() : nullptr;
	if (!IsValid(World)) return EVfxCullState::SpawnAndIgnoreVision;

	const APlayerController* LocalPC = World->GetFirstPlayerController();
	if (!IsValid(LocalPC)) return EVfxCullState::SpawnAndIgnoreVision;

	const APawn* LocalPawn = LocalPC->GetPawn();
	if (!IsValid(LocalPawn)) return EVfxCullState::SpawnAndIgnoreVision;

	const AActor* InstigatorActor = Cast<AActor>(Parameters.Instigator.Get());
	const AActor* EffectCauser = Cast<AActor>(Parameters.EffectCauser.Get());

	// --- 예외 1: Instigator가 아군(같은 팀)이면 무조건 표시 ---
	if (IsValid(InstigatorActor))
	{
		const ITargetableInterface* InstigatorTeam = Cast<ITargetableInterface>(InstigatorActor);
		const ITargetableInterface* LocalTeam = Cast<ITargetableInterface>(LocalPawn);
		if (InstigatorTeam && LocalTeam)
		{
			if (InstigatorTeam->GetTeamType() != ETeamType::None && InstigatorTeam->GetTeamType() == LocalTeam->GetTeamType())
			{
				return EVfxCullState::SpawnAndIgnoreVision;
			}
		}
	}

	// --- 예외 2: 발사체형 스킬 판정 (한 번 보이면 끝까지 보임 상태로 전이) ---
	bool bIsProjectile = false;
	if (IsValid(EffectCauser))
	{
		if (EffectCauser->IsA(ABaseMissileActor::StaticClass()) || EffectCauser->FindComponentByClass<UProjectileMovementComponent>() != nullptr)
		{
			bIsProjectile = true;
		}
	}
	if (!bIsProjectile && IsValid(TargetActor))
	{
		if (TargetActor->IsA(ABaseMissileActor::StaticClass()) || TargetActor->FindComponentByClass<UProjectileMovementComponent>() != nullptr)
		{
			bIsProjectile = true;
		}
	}

	// 2. 시야(Vision) 판정
	// TargetActor나 Instigator 둘 중 하나라도 시야 컴포넌트가 있다면 그것을 우선으로 판별합니다.
	const AActor* VisionCheckActor = IsValid(TargetActor) ? TargetActor : InstigatorActor;
	if (IsValid(VisionCheckActor))
	{
		if (UVision_VisualComp* VisionComp = VisionCheckActor->FindComponentByClass<UVision_VisualComp>())
		{
			const float CurrentAlpha = VisionComp->GetVisibilityAlpha();
			// 시야 알파값이 0보다 크면 현재 내 시야에 보인다는 의미
			if (CurrentAlpha > 0.0f)
			{
				return bIsProjectile ? EVfxCullState::SpawnAndTrackVisionUntilSeen : EVfxCullState::SpawnAndTrackVision;
			}
			else
			{
				// 시야 밖이라면 지속형/단발성에 따라 처리가 갈림
				if (bIsProjectile)
				{
					return bIsPersistent ? EVfxCullState::SpawnAndTrackVisionUntilSeen : EVfxCullState::SkipSpawn;
				}
				return bIsPersistent ? EVfxCullState::SpawnHidden : EVfxCullState::SkipSpawn;
			}
		}
	}

	// 3. 시야 컴포넌트를 찾지 못했다면 기존 거리 기반 판정(Fallback) 적용
	FVector EffectLocation;
	if (!Parameters.Location.IsNearlyZero())
	{
		EffectLocation = Parameters.Location;
	}
	else if (IsValid(EffectCauser))
	{
		EffectLocation = EffectCauser->GetActorLocation();
	}
	else if (IsValid(TargetActor))
	{
		EffectLocation = TargetActor->GetActorLocation();
	}
	else
	{
		return EVfxCullState::SpawnAndTrackVision;
	}

	if (FVector::DistSquared(LocalPawn->GetActorLocation(), EffectLocation) <= MaxParticleSpawnDistanceSq)
	{
		return EVfxCullState::SpawnAndTrackVision;
	}

	return bIsPersistent ? EVfxCullState::SpawnHidden : EVfxCullState::SkipSpawn;
}
