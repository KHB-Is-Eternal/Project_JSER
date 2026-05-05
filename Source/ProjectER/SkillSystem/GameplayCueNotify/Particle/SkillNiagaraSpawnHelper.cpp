#include "SkillSystem/GameplayCueNotify/Particle/SkillNiagaraSpawnHelper.h"
#include "Components/SkeletalMeshComponent.h"
#include "NiagaraFunctionLibrary.h"
#include "NiagaraSystem.h"
#include "NiagaraComponent.h"
#include "NiagaraDataInterface.h"
#include "SkillSystem/GameplayCueNotify/Particle/SkillNiagaraSpawnSettings.h"
#include "SkillSystem/GameplayCueNotify/Particle/SkillNiagaraSpawnConfig.h"
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
			return Settings.RotationOffset;
		}
	}
}

UNiagaraComponent* SkillNiagaraSpawnHelper::SpawnNiagaraBySettings(UWorld* World, const FSkillNiagaraSpawnSettings& Settings, const FTransform& SourceTransform, const AActor* SourceActor, const FVector* OptionalLookAtTarget, USceneComponent* AttachTarget)
{
	UNiagaraComponent* ResultNC = nullptr;

	if (!IsValid(World) || Settings.NiagaraSystem.IsNull())
	{
		return nullptr;
	}

	UNiagaraSystem* LoadedNiagaraSystem = Settings.NiagaraSystem.LoadSynchronous();
	if (!IsValid(LoadedNiagaraSystem))
	{
		return nullptr;
	}

	if (Settings.bAttachToSource && (IsValid(SourceActor) || IsValid(AttachTarget)))
	{
		USceneComponent* FinalAttachComponent = IsValid(AttachTarget) ? AttachTarget : ResolveNiagaraAttachComponent(SourceActor, Settings);
		if (!IsValid(FinalAttachComponent))
		{
			return nullptr;
		}

		const FTransform ParentTransform = FinalAttachComponent->GetSocketTransform(Settings.SocketOrBoneName);
		const FRotator RelativeAttachRotation = CalculateNiagaraRelativeRotation(Settings, ParentTransform, OptionalLookAtTarget);
		
		ResultNC = UNiagaraFunctionLibrary::SpawnSystemAttached(LoadedNiagaraSystem, FinalAttachComponent, Settings.SocketOrBoneName, Settings.LocationOffset, RelativeAttachRotation, EAttachLocation::KeepRelativeOffset, true, true);
	}
	else {
		FTransform BaseTransform = SourceTransform;
		FQuat OffsetRotation = SourceTransform.GetRotation();

		if (Settings.SocketOrBoneName != NAME_None)
		{
			if (IsValid(AttachTarget))
			{
				BaseTransform = AttachTarget->GetSocketTransform(Settings.SocketOrBoneName);
				OffsetRotation = BaseTransform.GetRotation();
			}
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
		const FRotator SpawnRotation = CalculateNiagaraWorldRotation(Settings, BaseTransform, OptionalLookAtTarget);
		
		ResultNC = UNiagaraFunctionLibrary::SpawnSystemAtLocation(World, LoadedNiagaraSystem, SpawnLocation, SpawnRotation, FVector(1.f), true, true);
	}
	
	if (!IsValid(ResultNC)) {
		return nullptr;
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
	for (const auto& Pair : Settings.BoolParameters)
	{
		ResultNC->SetVariableBool(Pair.Key, Pair.Value);
	}
	for (const auto& Pair : Settings.IntParameters)
	{
		ResultNC->SetVariableInt(Pair.Key, Pair.Value);
	}
	for (const auto& Pair : Settings.ObjectParameters)
	{
		ResultNC->SetVariableObject(Pair.Key, Pair.Value);
	}
	for (const auto& Pair : Settings.DataInterfaceParameters)
	{
		if (UNiagaraDataInterface* DI = Pair.Value.Get())
		{
			FNiagaraVariable Var(DI->GetClass(), Pair.Key);
			ResultNC->GetOverrideParameters().SetDataInterface(DI, Var);
		}
	}

	ResultNC->Activate();
	return ResultNC;
}

UNiagaraComponent* SkillNiagaraSpawnHelper::SpawnNiagara(UWorld* World, const USkillNiagaraSpawnConfig* Config, const FTransform& SourceTransform, const AActor* SourceActor, const FVector* OptionalLookAtTarget, USceneComponent* AttachTarget)
{
	if (!IsValid(Config))
	{
		return nullptr;
	}

	return SpawnNiagaraBySettings(World, Config->ToSettings(), SourceTransform, SourceActor, OptionalLookAtTarget, AttachTarget);
}

void SkillNiagaraSpawnHelper::AttachNiagaraByConfig(UNiagaraComponent* Component, USceneComponent* NewParent, const USkillNiagaraSpawnConfig* Config)
{
	if (!IsValid(Component) || !IsValid(NewParent) || !IsValid(Config))
	{
		return;
	}

	if (Config->bAttachToSource)
	{
		// [Fix] SnapToTarget 대신 KeepRelative를 사용하여 처음 설정된 오프셋 유지
		FAttachmentTransformRules AttachRules(EAttachmentRule::KeepRelative, true);
		Component->AttachToComponent(NewParent, AttachRules, Config->SocketOrBoneName);
	}
}