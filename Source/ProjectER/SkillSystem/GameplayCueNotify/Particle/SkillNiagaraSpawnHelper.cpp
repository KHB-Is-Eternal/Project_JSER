#include "SkillSystem/GameplayCueNotify/Particle/SkillNiagaraSpawnHelper.h"
#include "Components/SkeletalMeshComponent.h"
#include "NiagaraFunctionLibrary.h"
#include "NiagaraSystem.h"
#include "NiagaraComponent.h"
#include "SkillSystem/GameplayCueNotify/Particle/SkillNiagaraSpawnSettings.h"
#include "Engine/World.h"

namespace
{
	USceneComponent* ResolveNiagaraAttachComponent(const AActor* SourceActor, const FSkillNiagaraSpawnSettings& Settings)
	{
		if (!IsValid(SourceActor))
		{
			return nullptr;
		}

		if (Settings.SocketOrBoneName != NAME_None)
		{
			if (USkeletalMeshComponent* SkeletalMeshComponent = SourceActor->FindComponentByClass<USkeletalMeshComponent>())
			{
				if (SkeletalMeshComponent->DoesSocketExist(Settings.SocketOrBoneName))
				{
					return SkeletalMeshComponent;
				}
			}
		}

		return SourceActor->GetRootComponent();
	}

	FRotator CalculateNiagaraWorldRotation(const FSkillNiagaraSpawnSettings& Settings, const FTransform& SourceTransform, const FVector* OptionalLookAtTarget)
	{
		switch (Settings.RotationMode)
		{
		case ENiagaraSpawnRotationMode::WorldRotationOffsetOnly:
			return Settings.RotationOffset;

		case ENiagaraSpawnRotationMode::LookAtTargetPlusOffset:
			if (OptionalLookAtTarget != nullptr)
			{
				const FVector SourceLocation = SourceTransform.GetLocation();
				const FVector LookDirection = *OptionalLookAtTarget - SourceLocation;
				if (!LookDirection.IsNearlyZero())
				{
					return FRotationMatrix::MakeFromX(LookDirection).Rotator() + Settings.RotationOffset;
				}
			}
			return SourceTransform.GetRotation().Rotator() + Settings.RotationOffset;

		case ENiagaraSpawnRotationMode::SourceRotationPlusOffset:
		default:
			return SourceTransform.GetRotation().Rotator() + Settings.RotationOffset;
		}
	}

	FRotator CalculateNiagaraRelativeRotation(const FSkillNiagaraSpawnSettings& Settings, const FTransform& ParentTransform, const FVector* OptionalLookAtTarget)
	{
		switch (Settings.RotationMode)
		{
		case ENiagaraSpawnRotationMode::WorldRotationOffsetOnly:
			// 부모의 회전을 취소하고 순수 월드 오프셋만 바라보게 함
			return (ParentTransform.GetRotation().Inverse() * Settings.RotationOffset.Quaternion()).Rotator();

		case ENiagaraSpawnRotationMode::LookAtTargetPlusOffset:
			if (OptionalLookAtTarget != nullptr)
			{
				const FVector ParentLocation = ParentTransform.GetLocation();
				const FVector LookDirection = *OptionalLookAtTarget - ParentLocation;
				if (!LookDirection.IsNearlyZero())
				{
					const FRotator TargetWorldRot = FRotationMatrix::MakeFromX(LookDirection).Rotator() + Settings.RotationOffset;
					return (ParentTransform.GetRotation().Inverse() * TargetWorldRot.Quaternion()).Rotator();
				}
			}
			return Settings.RotationOffset;

		case ENiagaraSpawnRotationMode::SourceRotationPlusOffset:
		default:
			// 부모(Source)를 그대로 따라가면 되므로 순수 로컬 오프셋만 반환
			return Settings.RotationOffset;
		}
	}
}

void SkillNiagaraSpawnHelper::SpawnNiagaraBySettings(UWorld* World, const FSkillNiagaraSpawnSettings& Settings, const FTransform& SourceTransform, const AActor* SourceActor, const FVector* OptionalLookAtTarget, USceneComponent* AttachTarget)
{
	UNiagaraComponent* ResultNC = nullptr;

	if (!IsValid(World) || Settings.NiagaraSystem.IsNull())
	{
		return;
	}

	UNiagaraSystem* LoadedNiagaraSystem = Settings.NiagaraSystem.LoadSynchronous();
	if (!IsValid(LoadedNiagaraSystem))
	{
		return;
	}

	if (Settings.bAttachToSource && (IsValid(SourceActor) || IsValid(AttachTarget)))
	{
		USceneComponent* FinalAttachComponent = IsValid(AttachTarget) ? AttachTarget : ResolveNiagaraAttachComponent(SourceActor, Settings);
		if (!IsValid(FinalAttachComponent))
		{
			return;
		}

		const FTransform ParentTransform = FinalAttachComponent->GetSocketTransform(Settings.SocketOrBoneName);
		const FRotator RelativeAttachRotation = CalculateNiagaraRelativeRotation(Settings, ParentTransform, OptionalLookAtTarget);
		ResultNC = UNiagaraFunctionLibrary::SpawnSystemAttached(LoadedNiagaraSystem, FinalAttachComponent, Settings.SocketOrBoneName, Settings.LocationOffset, RelativeAttachRotation, EAttachLocation::KeepRelativeOffset, true, false);
	}
	else {
		FTransform BaseTransform = SourceTransform;
		FQuat OffsetRotation = SourceTransform.GetRotation(); // 오프셋 계산에 사용할 회전 (시전자 기준)

		// [Fix] 부착하지 않더라도 본 이름이 있으면 해당 위치와 회전을 기준점으로 사용합니다.
		if (Settings.SocketOrBoneName != NAME_None)
		{
			// 1. 명시적으로 전달된 AttachTarget(컴포넌트)이 있다면 최우선적으로 그 본 트랜스폼을 사용
			if (IsValid(AttachTarget))
			{
				BaseTransform = AttachTarget->GetSocketTransform(Settings.SocketOrBoneName);
				OffsetRotation = BaseTransform.GetRotation();
			}
			// 2. 아니면 SourceActor에서 컴포넌트를 찾아 해결
			else if (IsValid(SourceActor))
			{
				if (USceneComponent* BoneComponent = ResolveNiagaraAttachComponent(SourceActor, Settings))
				{
					BaseTransform = BoneComponent->GetSocketTransform(Settings.SocketOrBoneName);
					OffsetRotation = BaseTransform.GetRotation();
				}
			}
		}

		const FVector SourceLocation = BaseTransform.GetLocation();
		const FVector WorldLocationOffset = Settings.bUseSourceRotationForLocationOffset
			? OffsetRotation.RotateVector(Settings.LocationOffset)
			: Settings.LocationOffset;
		
		const FVector SpawnLocation = SourceLocation + WorldLocationOffset;
		
		// 회전값 결정 (본을 찾았다면 본의 회전 기준, 아니면 시전자 기준)
		const FRotator SpawnRotation = CalculateNiagaraWorldRotation(Settings, BaseTransform, OptionalLookAtTarget);
		
		ResultNC = UNiagaraFunctionLibrary::SpawnSystemAtLocation(World, LoadedNiagaraSystem, SpawnLocation, SpawnRotation, FVector(1.f), true, false);
	}
	
	if (!IsValid(ResultNC)) {
		UE_LOG(LogTemp, Warning, TEXT("ResultNC is Null, Niagara is Not Spawn"));
		return;
	}

	// 파라미터 적용
	for (const auto& Pair : Settings.FloatParameters)
	{
		ResultNC->SetVariableFloat(Pair.Key, Pair.Value);
	}
	for (const auto& Pair : Settings.VectorParameters)
	{
		ResultNC->SetVariableVec3(Pair.Key, Pair.Value);
	}
	for (const auto& Pair : Settings.ColorParameters)
	{
		ResultNC->SetVariableLinearColor(Pair.Key, Pair.Value);
	}

	ResultNC->Activate();
}