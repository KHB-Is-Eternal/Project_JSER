// Fill out your copyright notice in the Description page of Project Settings.

#include "SkillSystem/GameplayCueNotify/Particle/GCN_FloatingText.h"
#include "NiagaraFunctionLibrary.h"
#include "NiagaraComponent.h"
#include "NiagaraSystem.h"
#include "SkillSystem/GameplayCueNotify/Particle/SkillVfxCullingHelper.h"
#include "SkillSystem/GameplayCueNotify/Particle/VisionParticleManagerSubsystem.h"
#include "LineOfSight/VisionComps/Vision_VisualComp.h"

UGCN_FloatingText::UGCN_FloatingText()
{
	// 기본 소프트 에셋 경로 지정하여 캐싱
	FloatingTextNiagaraSystem = TSoftObjectPtr<UNiagaraSystem>(FSoftObjectPath(TEXT("/Game/VFX/ProjectVFX/FloatingText/NS_FloatingText.NS_FloatingText")));
}

bool UGCN_FloatingText::OnExecute_Implementation(AActor* MyTarget, const FGameplayCueParameters& Parameters) const
{
	if (!IsValid(MyTarget))
	{
		return false;
	}

	UWorld* const World = MyTarget->GetWorld();
	if (!IsValid(World))
	{
		return false;
	}

	const EVfxCullState CullState = USkillVfxCullingHelper::CheckVfxCulling(MyTarget, Parameters, false);
	if (CullState == EVfxCullState::SkipSpawn)
	{
		return false;
	}

	// 소프트 레퍼런스 검사 및 동기 로드
	UNiagaraSystem* SelectedSystem = FloatingTextNiagaraSystem.Get();
	if (!SelectedSystem)
	{
		SelectedSystem = FloatingTextNiagaraSystem.LoadSynchronous();
	}

	if (!IsValid(SelectedSystem))
	{
		return false;
	}

	// 전송받은 인코딩 데이터 디코딩
	const float ValueNumber = Parameters.RawMagnitude;
	const float TextSize = Parameters.NormalizedMagnitude;
	const FLinearColor TextColor = FLinearColor(Parameters.Normal.X, Parameters.Normal.Y, Parameters.Normal.Z, 1.f);

	if (ValueNumber <= 0.f)
	{
		return false;
	}

	// 스폰 위치 결정 (머리 위 기준 랜덤 오프셋 적용)
	const FVector SpawnLocation = MyTarget->GetActorLocation() + FVector(
		FMath::FRandRange(-30.f, 30.f),
		FMath::FRandRange(-30.f, 30.f),
		120.f
	);

	// 나이아가라 시스템 스폰 및 파라미터 세팅
	UNiagaraComponent* const NiagaraComp = UNiagaraFunctionLibrary::SpawnSystemAtLocation(
		World,
		SelectedSystem,
		SpawnLocation
	);

	if (IsValid(NiagaraComp))
	{
		NiagaraComp->SetVariableFloat(FName(TEXT("Number")), ValueNumber);
		NiagaraComp->SetVariableLinearColor(FName(TEXT("Color")), TextColor);
		NiagaraComp->SetVariableFloat(FName(TEXT("Size")), TextSize);

		UVision_VisualComp* VisionVisualComp = MyTarget->FindComponentByClass<UVision_VisualComp>();
		if (CullState == EVfxCullState::SpawnHidden || 
			(CullState == EVfxCullState::SpawnAndTrackVisionUntilSeen && 
			 IsValid(VisionVisualComp) && VisionVisualComp->GetVisibilityAlpha() <= 0.0f))
		{
			NiagaraComp->SetVisibility(false);
			NiagaraComp->SetHiddenInGame(true);
		}

		if (CullState == EVfxCullState::SpawnAndTrackVision || 
			CullState == EVfxCullState::SpawnHidden || 
			CullState == EVfxCullState::SpawnAndTrackVisionUntilSeen)
		{
			if (UVisionParticleManagerSubsystem* VisionSubsystem = World->GetSubsystem<UVisionParticleManagerSubsystem>())
			{
				const bool bTrackUntilSeen = (CullState == EVfxCullState::SpawnAndTrackVisionUntilSeen);
				VisionSubsystem->RegisterParticle(NiagaraComp, MyTarget, bTrackUntilSeen);
			}
		}

		return true;
	}

	return false;
}
