#include "SkillSystem/GameplayCueNotify/AGCN_SummonedActor.h"
#include "SkillSystem/GameplayCueNotify/GCN_SummonedRegistrySubsystem.h"
#include "SkillSystem/GameplayEffectComponent/SummonRangeBaseGEC.h"
#include "SkillSystem/GameplayEffectComponent/LaunchProjectile.h"
#include "SkillSystem/GameplayEffectComponent/LaunchHomingMissile.h"
#include "SkillSystem/GameplayCueNotify/Particle/SkillNiagaraSpawnConfig.h"
#include "SkillSystem/GameplayCueNotify/Sound/SkillSoundSpawnConfig.h"
#include "SkillSystem/GAS/ProjectERGameplayEffectContext.h"
#include "SkillSystem/Interfaces/SkillVisualDataProvider.h"
#include "NiagaraComponent.h"
#include "NiagaraFunctionLibrary.h"
#include "Components/AudioComponent.h"
#include "SkillSystem/GameplayCueNotify/Particle/SkillNiagaraSpawnHelper.h"
#include "SkillSystem/GameplayCueNotify/Sound/SkillSoundSpawnHelper.h"
#include "GameFramework/ProjectileMovementComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/ShapeComponent.h"
#include "Components/SphereComponent.h"
#include "Components/BoxComponent.h"
#include "Components/CapsuleComponent.h"
#include "Engine/StaticMesh.h"
#include "CharacterSystem/Interface/TargetableInterface.h"
#include "Engine/Engine.h"
#include "GameFramework/PlayerController.h"

AGCN_SummonedActor::AGCN_SummonedActor()
{
	// [Network Optimization] GCN 액터는 기본적으로 클라이언트에서 실행되므로 리플리케이션은 끄는 것이 일반적입니다.
	bReplicates = false;

	// [Fix] RootComponent 생성 (루트가 없으면 월드 위치 설정이 불가능합니다.)
	USceneComponent* SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
	RootComponent = SceneRoot;

	// 예측 이동을 위한 컴포넌트 생성 (기본은 비활성)
	MovementComponent = CreateDefaultSubobject<UProjectileMovementComponent>(TEXT("MovementComponent"));
	MovementComponent->bAutoActivate = false;
	MovementComponent->bRotationFollowsVelocity = true;
	MovementComponent->ProjectileGravityScale = 0.0f;
}

bool AGCN_SummonedActor::OnExecute_Implementation(AActor* MyTarget, const FGameplayCueParameters& Parameters)
{
	HandleSummonedVfx(Parameters);
	return Super::OnExecute_Implementation(MyTarget, Parameters);
}

bool AGCN_SummonedActor::WhileActive_Implementation(AActor* MyTarget, const FGameplayCueParameters& Parameters)
{
	HandleSummonedVfx(Parameters);
	return Super::WhileActive_Implementation(MyTarget, Parameters);
}

bool AGCN_SummonedActor::OnRemove_Implementation(AActor* MyTarget, const FGameplayCueParameters& Parameters)
{
	// 제거 시 레지스트리에서도 명시적으로 정리 (판정 액터가 매칭에 실패했거나 일찍 소멸된 경우 대비)
	if (UWorld* World = GetWorld())
	{
		if (UGCN_SummonedRegistrySubsystem* Registry = World->GetSubsystem<UGCN_SummonedRegistrySubsystem>())
		{
			AActor* ActualInstigator = GetActualInstigator(Parameters);
			const FProjectERGameplayEffectContext* Context = static_cast<const FProjectERGameplayEffectContext*>(Parameters.EffectContext.Get());

			if (ActualInstigator && Context && Context->ClientActivationTime > 0.0f)
			{
				Registry->GetAndUnregisterVfxActor(ActualInstigator, Context->ClientActivationTime);
			}
		}
	}

	return Super::OnRemove_Implementation(MyTarget, Parameters);
}

void AGCN_SummonedActor::HandleSummonedVfx(const FGameplayCueParameters& Parameters)
{
	// [Fix] 같은 GCN 인스턴스에서 OnExecute + WhileActive 이중 호출 시 자기 파괴 방지
	if (bIsAlreadyInitialized)
	{
		return;
	}
	bIsAlreadyInitialized = true;

	// 1. 초기 위치 및 회전 설정 (World Origin 방지)
	FVector SpawnLocation = Parameters.Location;
	FRotator SpawnRotation = Parameters.Normal.IsNearlyZero() ? GetActorRotation() : Parameters.Normal.Rotation();
	
	SetActorLocationAndRotation(SpawnLocation, SpawnRotation);

	// 2. 기본 수명 설정 (서버 트래벌 등 예외 상황에서 고아가 되는 것 방지)
	if (GetLifeSpan() <= 0.0f)
	{
		SetLifeSpan(10.0f); // 10초 후 자동 소멸 (핸드셰이크 성공 시 갱신될 수 있음)
	}

	AActor* ActualInstigator = GetActualInstigator(Parameters);
	const FProjectERGameplayEffectContext* Context = static_cast<const FProjectERGameplayEffectContext*>(Parameters.EffectContext.Get());

	if (ActualInstigator && Context && Context->ClientActivationTime > 0.0f)
	{
		if (UWorld* World = GetWorld())
		{
			if (UGCN_SummonedRegistrySubsystem* Registry = World->GetSubsystem<UGCN_SummonedRegistrySubsystem>())
			{
				// [중요] Standalone/ListenServer 호스트 등에서의 중복 실행 방지
				// 이미 예측으로 생성된 액터가 있다면 이 인스턴스는 아무것도 하지 않고 즉시 소멸해야 합니다.
				if (Registry->IsVfxActorRegistered(ActualInstigator, Context->ClientActivationTime))
				{
					Destroy();
					return;
				}

				// 비주얼 액터 등록
				Registry->RegisterVfxActor(ActualInstigator, Context->ClientActivationTime, this);
			}
		}
	}

	// [Fix] 중복 체크를 통과한 경우에만 실제 비주얼/오디오 초기화 수행 (중복 재생 방지)
	InitializeFromGEC(Parameters.SourceObject.Get());
}

void AGCN_SummonedActor::InitializeFromGEC(const UObject* SourceObject)
{
	if (!SourceObject) return;
	
	CachedSourceObject = const_cast<UObject*>(SourceObject);



	// [Refactor] 특정 GEC 클래스나 부모 GEC에 의존하지 않고 인터페이스(ISkillVisualDataProvider)를 사용합니다.
	if (const ISkillVisualDataProvider* VisualSource = Cast<ISkillVisualDataProvider>(SourceObject))
	{
		// 1. 비주얼(VFX) 초기화 - 인터페이스가 제공하는 컨피그를 사용
		SetupVfxComponent(VisualSource->GetAGCN_NiagaraConfig());

		// 2. 사운드(SFX) 초기화 - 인터페이스가 제공하는 컨피그를 사용
		SetupSfxComponent(VisualSource->GetAGCN_SoundConfig());

		// 3. 이동(Movement) 초기화 - GEC인 경우에만 추가 설정 수행
		if (const UBaseGEC* BaseGEC = Cast<UBaseGEC>(VisualSource))
		{
			if (MovementComponent)
			{
				BaseGEC->SetupMovement(MovementComponent);
				
				// 발사체 성격인 경우 즉시 활성화
				if (MovementComponent->InitialSpeed > 0.0f)
				{
					MovementComponent->Velocity = GetActorForwardVector() * MovementComponent->InitialSpeed;
					MovementComponent->Activate(true);
				}
			}
		}
	}
}

void AGCN_SummonedActor::SetupVfxComponent(const USkillNiagaraSpawnConfig* NiagaraConfig)
{
	if (!NiagaraConfig)
	{
		return;
	}

	// [Refactor] 전역 헬퍼를 사용하여 VFX 생성 및 설정 통합
	VfxComponent = SkillNiagaraSpawnHelper::SpawnNiagara(GetWorld(), NiagaraConfig, GetActorTransform(), this);
}

void AGCN_SummonedActor::SetupSfxComponent(const USkillSoundSpawnConfig* SoundConfig)
{
	if (!SoundConfig)
	{
		return;
	}

	// [Refactor] 전역 헬퍼를 사용하여 사운드 생성 및 설정 통합
	SfxComponent = SkillSoundSpawnHelper::SpawnSound(GetWorld(), SoundConfig, GetActorTransform(), this);
}


AActor* AGCN_SummonedActor::GetActualInstigator(const FGameplayCueParameters& Parameters) const
{
	// 1. Parameters에 직접 전달된 Instigator 확인
	if (AActor* ParamsInstigator = Parameters.Instigator.Get())
	{
		return ParamsInstigator;
	}

	// 2. EffectContext에 저장된 Instigator 확인
	if (Parameters.EffectContext.IsValid())
	{
		return Parameters.EffectContext.GetInstigator();
	}

	return nullptr;
}

void AGCN_SummonedActor::OnTargetActorDestroyed(AActor* DestroyedActor)
{
	// [Fix] 부착된 경우에만 액터와 함께 즉시 제거합니다. 
	// 부착되지 않은(AtLocation) 경우, 나이아가라 자체 수명에 맡겨 잔상이 남도록 합니다.
	if (VfxComponent)
	{
		bool bShouldDestroyInstantly = true;
		if (const ISkillVisualDataProvider* VisualSource = Cast<ISkillVisualDataProvider>(CachedSourceObject.Get()))
		{
			if (const USkillNiagaraSpawnConfig* Config = VisualSource->GetAGCN_NiagaraConfig())
			{
				bShouldDestroyInstantly = Config->bAttachToSource;
			}
		}

		if (bShouldDestroyInstantly)
		{
			VfxComponent->DestroyComponent();
		}
	}

	Destroy();
}

void AGCN_SummonedActor::SetupCollisionOutline(UShapeComponent* InCollisionComponent, AActor* InInstigatorActor)
{
	if (!IsValid(InCollisionComponent) || !IsValid(InInstigatorActor))
	{
		return;
	}

	// 1. 로컬 플레이어 기반 아군/적군 판단 (아군 251, 적군 250)
	int32 StencilValue = 250; // 기본 적군
	if (APlayerController* LocalPC = GEngine ? GEngine->GetFirstLocalPlayerController(GetWorld()) : nullptr)
	{
		if (AActor* LocalPawn = LocalPC->GetPawn())
		{
			ITargetableInterface* LocalTargetable = Cast<ITargetableInterface>(LocalPawn);
			ITargetableInterface* InstigatorTargetable = Cast<ITargetableInterface>(InInstigatorActor);

			if (LocalTargetable && InstigatorTargetable)
			{
				if (LocalTargetable->GetTeamType() == InstigatorTargetable->GetTeamType())
				{
					StencilValue = 251; // 아군
				}
			}
		}
	}

	// 2. 콜리전 형태에 따른 엔진 기본 메쉬 로드 및 스케일 조정
	UStaticMesh* BaseMesh = nullptr;
	FVector MeshScale = FVector(1.0f);

	if (USphereComponent* SphereComp = Cast<USphereComponent>(InCollisionComponent))
	{
		BaseMesh = Cast<UStaticMesh>(StaticLoadObject(UStaticMesh::StaticClass(), nullptr, TEXT("/Engine/BasicShapes/Sphere.Sphere")));
		if (BaseMesh)
		{
			// 엔진 Sphere 반경은 50 (지름 100)
			float ScaleFactor = SphereComp->GetUnscaledSphereRadius() / 50.0f;
			MeshScale = FVector(ScaleFactor);
		}
	}
	else if (UBoxComponent* BoxComp = Cast<UBoxComponent>(InCollisionComponent))
	{
		BaseMesh = Cast<UStaticMesh>(StaticLoadObject(UStaticMesh::StaticClass(), nullptr, TEXT("/Engine/BasicShapes/Cube.Cube")));
		if (BaseMesh)
		{
			// 엔진 Cube Extent는 50 (크기 100x100x100)
			MeshScale = BoxComp->GetUnscaledBoxExtent() / 50.0f;
		}
	}
	else if (UCapsuleComponent* CapsuleComp = Cast<UCapsuleComponent>(InCollisionComponent))
	{
		BaseMesh = Cast<UStaticMesh>(StaticLoadObject(UStaticMesh::StaticClass(), nullptr, TEXT("/Engine/BasicShapes/Cylinder.Cylinder")));
		if (BaseMesh)
		{
			// 엔진 Cylinder는 반경 50, 높이 100 (half height 50)
			float RadiusScale = CapsuleComp->GetUnscaledCapsuleRadius() / 50.0f;
			float HeightScale = CapsuleComp->GetUnscaledCapsuleHalfHeight() / 50.0f;
			MeshScale = FVector(RadiusScale, RadiusScale, HeightScale);
		}
	}

	if (!BaseMesh)
	{
		return;
	}

	// 3. 아웃라인용 메쉬 컴포넌트 동적 생성 혹은 재사용
	bool bIsNewComponent = false;
	if (!CollisionOutlineMesh)
	{
		CollisionOutlineMesh = NewObject<UStaticMeshComponent>(this, TEXT("CollisionOutlineMesh"));
		if (!CollisionOutlineMesh) return;
		bIsNewComponent = true;
	}

	CollisionOutlineMesh->SetStaticMesh(BaseMesh);
	CollisionOutlineMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	// 4. 렌더링 설정 (메인 패스 및 일반 뎁스 패스 제외, 커스텀 뎁스만 렌더링)
	CollisionOutlineMesh->SetRenderInMainPass(false);
	CollisionOutlineMesh->SetRenderInDepthPass(false);
	CollisionOutlineMesh->SetRenderCustomDepth(true);
	CollisionOutlineMesh->SetCustomDepthStencilValue(StencilValue);
	CollisionOutlineMesh->SetCastShadow(false);
	CollisionOutlineMesh->SetAffectDistanceFieldLighting(false);

	// 5. 부착 및 트랜스폼 동기화
	if (bIsNewComponent)
	{
		CollisionOutlineMesh->RegisterComponent();
		CollisionOutlineMesh->AttachToComponent(InCollisionComponent, FAttachmentTransformRules::SnapToTargetNotIncludingScale);
	}
	CollisionOutlineMesh->SetWorldScale3D(MeshScale);
	CollisionOutlineMesh->SetWorldLocationAndRotationNoPhysics(InCollisionComponent->GetComponentLocation(), InCollisionComponent->GetComponentRotation());
}

