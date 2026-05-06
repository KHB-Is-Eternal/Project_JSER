// Fill out your copyright notice in the Description page of Project Settings.


#include "SkillSystem/GameplayEffectComponent/LaunchProjectile.h"
#include "Abilities/GameplayAbilityTypes.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "GameplayEffect.h"
#include "GameFramework/Actor.h"
#include "SkillSystem/Actor/BaseRangeOverlapEffectActor/BaseRangeOverlapEffectActor.h"

//#include "SkillSystem/GameplayEffectComponent/SummonRangeAtBone.h"
#include "GameFramework/ProjectileMovementComponent.h"

#include "GameFramework/Character.h"
#include "Components/SkeletalMeshComponent.h"
#include "SkillSystem/GAS/ProjectERGameplayEffectContext.h"
#include "GameFramework/GameStateBase.h"
#include "GameFramework/GameState.h"

ULaunchProjectile::ULaunchProjectile()
{
}

void ULaunchProjectile::InitializeRangeActor(ABaseRangeOverlapEffectActor* RangeActor, AActor* Instigator, const FGameplayEffectContextHandle& Context, const FGameplayCueParameters& HitTargetVfxCueParameters, const FGameplayCueParameters& HitTargetSoundCueParameters) const
{
	Super::InitializeRangeActor(RangeActor, Instigator, Context, HitTargetVfxCueParameters, HitTargetSoundCueParameters);

	if (IsValid(RangeActor))
	{
		RangeActor->bDestroyOnOverlap = this->bDestroyOnHit;
		
		// 필수: 동적 이동 컴포넌트의 위치 변화가 클라이언트에 복제되도록 설정
		RangeActor->SetReplicateMovement(true);

		UProjectileMovementComponent* MovementComp = NewObject<UProjectileMovementComponent>(RangeActor, UProjectileMovementComponent::StaticClass(), TEXT("DynamicProjectileMovement"));
		if (IsValid(MovementComp))
		{
			// 동적으로 추가된 컴포넌트가 에디터/클라이언트에 정상 표시되고 복제되도록 설정
			MovementComp->CreationMethod = EComponentCreationMethod::Instance;
			MovementComp->SetIsReplicated(true);

			// 이동에 필요한 기본 속성 할당
			float Speed = this->InitialSpeed;
			MovementComp->InitialSpeed = Speed;
			MovementComp->MaxSpeed = Speed;
			MovementComp->ProjectileGravityScale = this->GravityScale;
			
			// 컴포넌트를 액터 인스턴스에 등록
			MovementComp->RegisterComponent();
			RangeActor->AddInstanceComponent(MovementComp);

			// 기준 컴포넌트 지정 (반드시 Register 이후에 호출)
			MovementComp->SetUpdatedComponent(RangeActor->GetRootComponent());
			// [Revert] UProjectileMovementComponent는 bInitialVelocityInLocalSpace가 기본 true이므로, 항상 (1,0,0)을 주어야 올바른 정면 방향으로 날아갑니다.
			MovementComp->Velocity = FVector::ForwardVector * Speed;
			MovementComp->Activate(true);
		}
	}
}

FTransform ULaunchProjectile::CalculateSpawnTransform(const FGameplayEffectSpec& GESpec, const AActor* Instigator, const AActor* TargetActor) const
{
	if (!IsValid(Instigator))
	{
		return FTransform::Identity;
	}

	FVector SpawnLocation = Instigator->GetActorLocation();
	FRotator SpawnRotation = Instigator->GetActorRotation();

	// 1. 기본 위치 및 회전 결정 (Bone 고려)
	if (!this->BoneName.IsNone())
	{
		const ACharacter* const Character = Cast<ACharacter>(Instigator);
		const USkeletalMeshComponent* Mesh = Character ? Character->GetMesh() : Instigator->FindComponentByClass<USkeletalMeshComponent>();
		
		if (IsValid(Mesh) && Mesh->DoesSocketExist(this->BoneName))
		{
			SpawnLocation = Mesh->GetSocketLocation(this->BoneName);
			
			// bUseInstigatorRotation이 false일 때만 본의 회전을 사용
			if (!this->bUseInstigatorRotation)
			{
				SpawnRotation = Mesh->GetSocketRotation(this->BoneName);
			}
		}
	}

	// 2. 발사 방향 결정 (Targeting - 타겟 위치가 있을 경우)
	if (this->bUseEffectContextDirection)
	{
		FVector TargetLocation = SpawnLocation;
		const FGameplayEffectContextHandle &Context = GESpec.GetContext();
		if (Context.HasOrigin())
		{
			TargetLocation = Context.GetOrigin();
		}
		else if (const FHitResult *Hit = Context.GetHitResult())
		{
			if (!Hit->Location.IsZero())
			{
				TargetLocation = Hit->Location;
			}
			else if (Hit->GetActor())
			{
				TargetLocation = Hit->GetActor()->GetActorLocation();
			}
		}

		// 위치가 다를 때만 조준 회전값 적용
		if (!TargetLocation.Equals(SpawnLocation))
		{
			SpawnRotation = (TargetLocation - SpawnLocation).Rotation();
		}
	}

	// 3. 공통 오프셋 및 지면 보정 적용
	ApplyCommonSpawnOptions(SpawnLocation, SpawnRotation, Instigator);

	return FTransform(SpawnRotation, SpawnLocation);
}