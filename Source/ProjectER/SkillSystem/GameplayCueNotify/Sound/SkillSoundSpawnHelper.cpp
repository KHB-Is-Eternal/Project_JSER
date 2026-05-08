#include "SkillSystem/GameplayCueNotify/Sound/SkillSoundSpawnHelper.h"
#include "SkillSystem/GameplayCueNotify/Sound/SkillSoundSpawnSettings.h"
#include "Kismet/GameplayStatics.h"
#include "Components/AudioComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/World.h"
#include "Sound/SoundBase.h"

namespace
{
	USceneComponent* ResolveSoundAttachComponent(const AActor* SourceActor, const FSkillSoundSpawnSettings& Settings)
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

	FRotator CalculateSoundWorldRotation(const FSkillSoundSpawnSettings& Settings, const FTransform& SourceTransform, const FVector* OptionalLookAtTarget)
	{
		switch (Settings.RotationMode)
		{
		case ESoundSpawnRotationMode::WorldRotationOffsetOnly:
			return Settings.RotationOffset;

		case ESoundSpawnRotationMode::LookAtTargetPlusOffset:
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

		case ESoundSpawnRotationMode::SourceRotationPlusOffset:
		default:
			return SourceTransform.GetRotation().Rotator() + Settings.RotationOffset;
		}
	}

	FRotator CalculateSoundRelativeRotation(const FSkillSoundSpawnSettings& Settings, const FTransform& ParentTransform, const FVector* OptionalLookAtTarget)
	{
		switch (Settings.RotationMode)
		{
		case ESoundSpawnRotationMode::WorldRotationOffsetOnly:
			return (ParentTransform.GetRotation().Inverse() * Settings.RotationOffset.Quaternion()).Rotator();

		case ESoundSpawnRotationMode::LookAtTargetPlusOffset:
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

		case ESoundSpawnRotationMode::SourceRotationPlusOffset:
		default:
			return Settings.RotationOffset;
		}
	}
}

#include "SkillSystem/GameplayCueNotify/Sound/SkillSoundSpawnConfig.h"

namespace
{
	// ... (ResolveSoundAttachComponent, CalculateSoundWorldRotation, CalculateSoundRelativeRotation는 그대로 유지)
}

UAudioComponent* SkillSoundSpawnHelper::PlaySoundBySettings(UWorld* World, const FSkillSoundSpawnSettings& Settings, const FTransform& SourceTransform, const AActor* SourceActor, const FVector* OptionalLookAtTarget, USceneComponent* AttachTarget)
{
	if (!IsValid(World) || Settings.Sound.IsNull())
	{
		return nullptr;
	}

	USoundBase* LoadedSound = Settings.Sound.LoadSynchronous();
	if (!IsValid(LoadedSound))
	{
		return nullptr;
	}

	FString ActorName = IsValid(SourceActor) ? SourceActor->GetName() : TEXT("None");
	FString NetModeStr = World->GetNetMode() == NM_Client ? TEXT("Client") : TEXT("Server");
	UE_LOG(LogTemp, Warning, TEXT("@@@ [SFX Play] Asset: [%s], Instigator: [%s], Location: %s, Mode: [%s]"), 
		*LoadedSound->GetName(), *ActorName, *SourceTransform.GetLocation().ToString(), *NetModeStr);

	if (Settings.bAttachToSource && (IsValid(SourceActor) || IsValid(AttachTarget)))
	{
		USceneComponent* FinalAttachComponent = IsValid(AttachTarget) ? AttachTarget : ResolveSoundAttachComponent(SourceActor, Settings);
		if (IsValid(FinalAttachComponent))
		{
			const FTransform ParentTransform = FinalAttachComponent->GetSocketTransform(Settings.SocketOrBoneName);
			const FRotator RelativeAttachRotation = CalculateSoundRelativeRotation(Settings, ParentTransform, OptionalLookAtTarget);
			
			return UGameplayStatics::SpawnSoundAttached(
				LoadedSound, 
				FinalAttachComponent, 
				Settings.SocketOrBoneName, 
				Settings.LocationOffset, 
				RelativeAttachRotation, 
				EAttachLocation::KeepRelativeOffset, 
				false, 
				Settings.VolumeMultiplier, 
				Settings.PitchMultiplier
			);
		}
	}

	// At Location (독립형 사운드로 생성하여 액터 수명과 분리)
	const FVector SourceLocation = SourceTransform.GetLocation();
	const FQuat SourceRotation = SourceTransform.GetRotation();
	const FVector WorldLocationOffset = Settings.bUseSourceRotationForLocationOffset
		? SourceRotation.RotateVector(Settings.LocationOffset)
		: Settings.LocationOffset;
	const FVector SpawnLocation = SourceLocation + WorldLocationOffset;
	const FRotator SpawnRotation = CalculateSoundWorldRotation(Settings, SourceTransform, OptionalLookAtTarget);

	return UGameplayStatics::SpawnSoundAtLocation(
		World, 
		LoadedSound, 
		SpawnLocation, 
		SpawnRotation, 
		Settings.VolumeMultiplier, 
		Settings.PitchMultiplier,
		0.0f,
		nullptr,
		nullptr,
		true // bAutoDestroy
	);
}

UAudioComponent* SkillSoundSpawnHelper::SpawnSound(UWorld* World, const USkillSoundSpawnConfig* Config, const FTransform& SourceTransform, const AActor* SourceActor, const FVector* OptionalLookAtTarget, USceneComponent* AttachTarget)
{
	if (!IsValid(Config))
	{
		return nullptr;
	}

	return PlaySoundBySettings(World, Config->ToSettings(), SourceTransform, SourceActor, OptionalLookAtTarget, AttachTarget);
}

void SkillSoundSpawnHelper::AttachSoundByConfig(UAudioComponent* Component, USceneComponent* NewParent, const USkillSoundSpawnConfig* Config)
{
	if (!IsValid(Component) || !IsValid(NewParent) || !IsValid(Config))
	{
		return;
	}

	if (Config->bAttachToSource)
	{
		// [Fix] 사운드 소켓 정보와 상대 트랜스폼 유지하며 부착
		FAttachmentTransformRules AttachRules(EAttachmentRule::KeepRelative, true);
		Component->AttachToComponent(NewParent, AttachRules, Config->SocketOrBoneName);
	}
}
