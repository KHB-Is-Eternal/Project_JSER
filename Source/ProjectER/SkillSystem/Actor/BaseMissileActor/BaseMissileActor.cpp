// Fill out your copyright notice in the Description page of Project Settings.


#include "SkillSystem/Actor/BaseMissileActor/BaseMissileActor.h"
#include "GameFramework/ProjectileMovementComponent.h"
#include "Components/SceneComponent.h"
#include "NiagaraComponent.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "Net/UnrealNetwork.h"
#include "SkillSystem/GameplayCueNotify/GCN_SummonedRegistrySubsystem.h"
#include "SkillSystem/GameplayCueNotify/AGCN_SummonedActor.h"
#include "SkillSystem/GameplayEffectComponent/LaunchHomingMissile.h"
#include "CharacterSystem/Interface/TargetableInterface.h"
#include "SkillSystem/GameplayEffect/BaseGameplayEffect.h"
#include "SkillSystem/GameAbility/SkillBase.h"

ABaseMissileActor::ABaseMissileActor()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = true;
	bReplicates = true;
	SetReplicateMovement(true);

	// 충돌체 없는 경량 루트
	SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
	RootComponent = SceneRoot;

	// 유도 비행용 무브먼트
	ProjectileMovement = CreateDefaultSubobject<UProjectileMovementComponent>(TEXT("ProjectileMovement"));
	
	// 초기 속도를 로컬 좌표계가 아닌 월드 좌표계(절대 방향) 기준으로 해석하도록 설정 (곡선 비행 방지 핵심)
	ProjectileMovement->bInitialVelocityInLocalSpace = false;
	
	ProjectileMovement->bRotationFollowsVelocity = true;
	ProjectileMovement->ProjectileGravityScale = 0.0f;

	// [Network Optimization] 파괴 및 위치 동기화 속도를 획기적으로 높입니다.
	//SetNetUpdateFrequency(100.0f);
	//NetPriority = 3.0f;
}

void ABaseMissileActor::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(ABaseMissileActor, ClientActivationTime);
	DOREPLIFETIME(ABaseMissileActor, InitialTargetRotation);
	DOREPLIFETIME(ABaseMissileActor, InstigatorActor);
}

void ABaseMissileActor::OnRep_InstigatorActor()
{
	// 데이터가 늦게 도착했을 경우 핸드셰이크 재시도
	if (InstigatorActor && ClientActivationTime > 0.f)
	{
		if (UWorld* World = GetWorld())
		{
			if (UGCN_SummonedRegistrySubsystem* Registry = World->GetSubsystem<UGCN_SummonedRegistrySubsystem>())
			{
				if (AActor* VfxActor = Registry->FindAndUnregisterVfxActorFuzzy(InstigatorActor, ClientActivationTime, 0.5f))
				{
					OnVfxHandshakeCompleted_Implementation(VfxActor);
				}
			}
		}
	}
}

void ABaseMissileActor::PostNetInit()
{
	Super::PostNetInit();

	// 클라이언트에서만 작동
	if (GetNetMode() == NM_DedicatedServer) return;

	if (UWorld* World = GetWorld())
	{
		if (UGCN_SummonedRegistrySubsystem* Registry = World->GetSubsystem<UGCN_SummonedRegistrySubsystem>())
		{


			// (시전자 + 시전 시간) 조합으로 퍼지 비주얼 검색 (서버-클라이언트 간의 작은 시간 오차 보정, 0.5초 허용)
			if (AActor* VfxActor = Registry->FindAndUnregisterVfxActorFuzzy(InstigatorActor, ClientActivationTime, 0.5f))
			{
				OnVfxHandshakeCompleted_Implementation(VfxActor);
			}
			else
			{
				UE_LOG(LogTemp, Warning, TEXT("ABaseMissileActor: Handshake Failed in PostNetInit. (Registering as Pending)"));
				Registry->RegisterPendingActorFuzzy(InstigatorActor, ClientActivationTime, this);
			}
		}
	}
}

void ABaseMissileActor::OnVfxHandshakeCompleted_Implementation(AActor* VfxActor)
{
	if (!VfxActor) return;

	// [Fix] 언리얼 생명주기 싱크 맞추기 위해 OnDestroyed 델리게이트에 VFX 액터를 바인딩
	if (AGCN_SummonedActor* SummonedGCN = Cast<AGCN_SummonedActor>(VfxActor))
	{
		// 액터 통째 부착이 아니라, 알맹이인 나이아가라 컴포넌트만 뽑아서 미사일에 직접 부착합니다!
		if (UNiagaraComponent* NiagaraComp = SummonedGCN->GetVfxComponent())
		{
			FAttachmentTransformRules AttachRules(EAttachmentRule::SnapToTarget, true);
			NiagaraComp->AttachToComponent(this->GetRootComponent(), AttachRules);
		}

		if (!this->OnDestroyed.IsAlreadyBound(SummonedGCN, &AGCN_SummonedActor::OnTargetActorDestroyed))
		{
			this->OnDestroyed.AddDynamic(SummonedGCN, &AGCN_SummonedActor::OnTargetActorDestroyed);
		}

		if (const ULaunchHomingMissile* MissileGEC = Cast<ULaunchHomingMissile>(SummonedGCN->GetSourceObject()))
		{
			if (IsValid(ProjectileMovement))
			{
				ProjectileMovement->InitialSpeed = MissileGEC->InitialSpeed;
				ProjectileMovement->MaxSpeed = MissileGEC->MaxSpeed;
				ProjectileMovement->HomingAccelerationMagnitude = MissileGEC->HomingAccelerationMagnitude;
				ProjectileMovement->bIsHomingProjectile = (MissileGEC->HomingAccelerationMagnitude > 0.f);

			}
		}
	}
}

void ABaseMissileActor::InitializeMissile(
	const TArray<FGameplayEffectSpecHandle>& InEffectSpecHandles,
	AActor* InInstigatorActor,
	AActor* InHomingTarget,
	const FGameplayCueParameters& InHitVfxCueParameters,
	const FGameplayCueParameters& InHitSoundCueParameters,
	float InInitialSpeed,
	float InMaxSpeed,
	float InHomingAcceleration,
	float InReachThreshold,
	bool bInDestroyOnHit,
	const FVector& InInitialDirection)
{
	EffectSpecHandles = InEffectSpecHandles;
	InstigatorActor = InInstigatorActor;
	HomingTargetActor = InHomingTarget;
	HitVfxCueParameters = InHitVfxCueParameters;
	HitSoundCueParameters = InHitSoundCueParameters;
	ReachThreshold = InReachThreshold;
	bDestroyOnHit = bInDestroyOnHit;

	if (IsValid(ProjectileMovement))
	{
		ProjectileMovement->InitialSpeed = InInitialSpeed;
		ProjectileMovement->MaxSpeed = InMaxSpeed;

		if (IsValid(HomingTargetActor))
		{
			ProjectileMovement->bIsHomingProjectile = true;
			ProjectileMovement->HomingTargetComponent = HomingTargetActor->GetRootComponent();
			ProjectileMovement->HomingAccelerationMagnitude = InHomingAcceleration;
		}

		// 초기 발사 방향으로 속도 설정 (FinishSpawning 전이므로 GetActorForwardVector() 대신 직접 전달받은 방향 사용)
		InitialTargetRotation = InInitialDirection.Rotation();
		ProjectileMovement->Velocity = InInitialDirection.GetSafeNormal() * InInitialSpeed;
	}

	// [Standalone/ListenServer Fix] 서버에서 데이터 초기화 즉시 핸드셰이크 시도 (PostNetInit이 안 도는 환경 대응)
	if (InstigatorActor && ClientActivationTime > 0.0f)
	{
		if (UWorld* World = GetWorld())
		{
			if (UGCN_SummonedRegistrySubsystem* Registry = World->GetSubsystem<UGCN_SummonedRegistrySubsystem>())
			{
				if (AActor* VfxActor = Registry->FindAndUnregisterVfxActorFuzzy(InstigatorActor, ClientActivationTime, 0.5f))
				{
					OnVfxHandshakeCompleted_Implementation(VfxActor);
				}
			}
		}
	}
}

void ABaseMissileActor::BeginPlay()
{
	Super::BeginPlay();

	// FinishSpawning 내부의 ExecuteConstruction/InitializeComponent에 의해 회전값과 Velocity가 왜곡되는 것을 방지하기 위해,
	// Transform이 확정된 시점(BeginPlay)에서 우리가 저장한 절대 방향으로 다시 한번 강제 설정합니다.
	if (IsValid(ProjectileMovement) && ProjectileMovement->InitialSpeed > 0.f)
	{
		SetActorRotation(InitialTargetRotation);
		ProjectileMovement->Velocity = GetActorForwardVector() * ProjectileMovement->InitialSpeed;
	}
}

void ABaseMissileActor::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	// 서버에서만 적중 판정
	if (bHasReached || !HasAuthority())
	{
		return;
	}

	if (!IsValid(HomingTargetActor))
	{
		// 타겟이 사라졌으면 파괴
		if (bDestroyOnHit)
		{
			Destroy();
		}
		return;
	}

	const float Distance = FVector::Dist(GetActorLocation(), HomingTargetActor->GetActorLocation());
	if (Distance <= ReachThreshold)
	{
		OnReachedTarget();
	}
}

void ABaseMissileActor::OnReachedTarget()
{
	if (bHasReached)
	{
		return;
	}
	bHasReached = true;

	// 1. 타겟에 효과 적용
	ApplyEffectsToTarget(HomingTargetActor);

	// 2. 적중 효과 실행
	ExecuteHitCues();

	// 3. 파괴 처리
	if (bDestroyOnHit)
	{
		Destroy();
	}
}

void ABaseMissileActor::ApplyEffectsToTarget(AActor* TargetActor)
{
	if (!IsValid(TargetActor))
	{
		return;
	}

	UAbilitySystemComponent* TargetASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(TargetActor);
	if (!IsValid(TargetASC))
	{
		return;
	}

	for (const FGameplayEffectSpecHandle& Handle : EffectSpecHandles)
	{
		if (Handle.IsValid())
		{
			// GE 레벨의 타겟팅 속성을 검사하여 부여 전에 필터링
			if (Handle.Data->Def)
			{
				const UBaseGameplayEffect* BaseGE = Cast<UBaseGameplayEffect>(Handle.Data->Def.Get());
				if (BaseGE)
				{
					if (!USkillBase::IsValidRelationship(InstigatorActor, TargetActor, BaseGE->TargetRelationship))
					{
						continue;
					}
				}
			}

			TargetASC->ApplyGameplayEffectSpecToSelf(*Handle.Data.Get());
		}
	}
}

void ABaseMissileActor::ExecuteHitCues()
{
	if (!IsValid(InstigatorActor))
	{
		return;
	}

	UAbilitySystemComponent* InstigatorASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(InstigatorActor);
	if (!IsValid(InstigatorASC))
	{
		return;
	}

	// VFX
	if (HitVfxCueParameters.OriginalTag.IsValid())
	{
		FGameplayCueParameters Params = HitVfxCueParameters;
		Params.Location = GetActorLocation();
		Params.EffectCauser = this;
		Params.TargetAttachComponent = IsValid(HomingTargetActor) ? HomingTargetActor->GetRootComponent() : nullptr;
		InstigatorASC->ExecuteGameplayCue(HitVfxCueParameters.OriginalTag, Params);
	}

	// Sound
	if (HitSoundCueParameters.OriginalTag.IsValid())
	{
		FGameplayCueParameters Params = HitSoundCueParameters;
		Params.Location = GetActorLocation();
		Params.EffectCauser = this;
		Params.TargetAttachComponent = IsValid(HomingTargetActor) ? HomingTargetActor->GetRootComponent() : nullptr;
		InstigatorASC->ExecuteGameplayCue(HitSoundCueParameters.OriginalTag, Params);
	}
}
