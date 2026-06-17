// Fill out your copyright notice in the Description page of Project Settings.

#include "SkillSystem/GameplayCueNotify/Particle/GCN_DamageText.h"
#include "CharacterSystem/GameplayTags/GameplayTags.h"
#include "NiagaraFunctionLibrary.h"
#include "NiagaraComponent.h"
#include "NiagaraSystem.h"

UGCN_DamageText::UGCN_DamageText()
{
	// 1. C++ 생성자에서 반응할 게임플레이 큐 태그 자동 바인딩
	GameplayCueTag = ProjectER::GameplayCue::Combat::DamageText;

	// 2. 기본 소프트 에셋 경로 지정하여 캐싱
	DamageTextNiagaraSystem = TSoftObjectPtr<UNiagaraSystem>(FSoftObjectPath(TEXT("/Game/VFX/ProjectVFX/FloatingText/NS_FloatingText.NS_FloatingText")));
}

bool UGCN_DamageText::OnExecute_Implementation(AActor* MyTarget, const FGameplayCueParameters& Parameters) const
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

	// 3. 소프트 레퍼런스 검사 및 동기 로드
	UNiagaraSystem* SelectedSystem = DamageTextNiagaraSystem.Get();
	if (!SelectedSystem)
	{
		SelectedSystem = DamageTextNiagaraSystem.LoadSynchronous();
	}

	if (!IsValid(SelectedSystem))
	{
		return false;
	}

	// 4. 전송받은 인코딩 데이터 디코딩
	const float DamageNumber = Parameters.RawMagnitude;
	const float TextSize = Parameters.NormalizedMagnitude;
	const FLinearColor TextColor = FLinearColor(Parameters.Normal.X, Parameters.Normal.Y, Parameters.Normal.Z, 1.f);

	if (DamageNumber <= 0.f)
	{
		return false;
	}

	// 5. 스폰 위치 결정 (머리 위 기준 랜덤 오프셋 적용)
	const FVector SpawnLocation = MyTarget->GetActorLocation() + FVector(
		FMath::FRandRange(-30.f, 30.f),
		FMath::FRandRange(-30.f, 30.f),
		120.f
	);

	// 6. 나이아가라 시스템 스폰 및 파라미터 세팅
	UNiagaraComponent* const NiagaraComp = UNiagaraFunctionLibrary::SpawnSystemAtLocation(
		World,
		SelectedSystem,
		SpawnLocation
	);

	if (IsValid(NiagaraComp))
	{
		NiagaraComp->SetVariableFloat(FName(TEXT("Number")), DamageNumber);
		NiagaraComp->SetVariableLinearColor(FName(TEXT("Color")), TextColor);
		NiagaraComp->SetVariableFloat(FName(TEXT("Size")), TextSize);
		return true;
	}

	return false;
}
