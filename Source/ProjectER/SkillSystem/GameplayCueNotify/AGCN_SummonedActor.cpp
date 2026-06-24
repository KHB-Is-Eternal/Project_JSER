#include "SkillSystem/GameplayCueNotify/AGCN_SummonedActor.h"
#include "SkillSystem/GameplayCueNotify/Components/GroundIndicatorComponent.h"
#include "SkillSystem/GameplayCueNotify/GCN_SummonedRegistrySubsystem.h"
#include "SkillSystem/GameplayEffectComponent/SummonRangeBaseGEC.h"
#include "SkillSystem/GameplayEffectComponent/SummonRangeAtBone.h"
#include "SkillSystem/GameplayEffectComponent/LaunchProjectile.h"
#include "SkillSystem/GameplayEffectComponent/LaunchHomingMissile.h"
#include "SkillSystem/GameplayCueNotify/Particle/SkillNiagaraSpawnConfig.h"
#include "SkillSystem/GameplayCueNotify/Sound/SkillSoundSpawnConfig.h"
#include "SkillSystem/GAS/ProjectERGameplayEffectContext.h"
#include "SkillSystem/Interfaces/SkillVisualDataProvider.h"
#include "NiagaraComponent.h"
#include "NiagaraFunctionLibrary.h"
#include "SkillSystem/GameplayCueNotify/Particle/SkillNiagaraSpawnHelper.h"
#include "SkillSystem/GameplayCueNotify/Particle/SkillVfxCullingHelper.h"
#include "SkillSystem/GameplayCueNotify/Particle/VisionParticleManagerSubsystem.h"
#include "SkillSystem/GameplayCueNotify/Sound/SkillSoundSpawnHelper.h"
#include "GameFramework/ProjectileMovementComponent.h"
#include "Components/ShapeComponent.h"
#include "Engine/World.h"
#include "LineOfSight/VisionComps/Vision_VisualComp.h"
#include "Components/SphereComponent.h"
#include "Components/BoxComponent.h"
#include "Components/CapsuleComponent.h"
#include "CharacterSystem/Interface/TargetableInterface.h"
#include "Engine/Engine.h"
#include "GameFramework/PlayerController.h"
#include "Components/StaticMeshComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "Engine/Texture2D.h"
#include "Materials/MaterialInstanceDynamic.h"

AGCN_SummonedActor::AGCN_SummonedActor()
{
	// [Network Optimization] GCN 액터는 기본적으로 클라이언트에서 실행되므로 리플리케이션은 끄는 것이 일반적입니다.
	bReplicates = false;

	// [Fix] RootComponent 생성 (루트가 없으면 월드 위치 설정이 불가능합니다.)
	// 시야(OcclusionTarget) 판정을 위해 UPrimitiveComponent를 상속받는 SphereComponent를 루트로 사용합니다.
	SceneRoot = CreateDefaultSubobject<USphereComponent>(TEXT("SceneRoot"));
	SetRootComponent(SceneRoot);
	SceneRoot->InitSphereRadius(10.0f);
	
	// [Fix] 시야 시스템(FOW)의 탐지 구체(DetectionSphere)가 이 액터를 인식할 수 있도록 콜리전을 켭니다.
	SceneRoot->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	SceneRoot->SetGenerateOverlapEvents(true); // 언리얼 엔진에서 겹침 이벤트가 발생하려면 쌍방 모두 true여야 합니다!
	
	// 프로젝트 커스텀 채널인 VisionSensor(ECC_GameTraceChannel3)를 기본 오브젝트 타입으로 설정합니다.
	SceneRoot->SetCollisionObjectType(ECC_GameTraceChannel3);
	
	// 불필요한 물리/트레이스 간섭을 막기 위해 기본적으로 모든 채널을 무시합니다.
	SceneRoot->SetCollisionResponseToAllChannels(ECR_Ignore);
	
	// FOW 탐지 구체와의 겹침(Overlap) 이벤트만 허용합니다. 
	// 탐지 구체가 WorldDynamic일 수도 있고 VisionSensor일 수도 있으므로 둘 다 허용합니다.
	SceneRoot->SetCollisionResponseToChannel(ECC_WorldDynamic, ECR_Overlap);
	SceneRoot->SetCollisionResponseToChannel(ECC_GameTraceChannel3, ECR_Overlap);
	
	// [Fix] TopDownVision 플러그인이 겹침 이벤트를 수신했을 때 무시하지 않도록 기본 태그를 추가합니다.
	SceneRoot->ComponentTags.Add(TEXT("VisionTarget"));
	
	SceneRoot->PrimaryComponentTick.bCanEverTick = false; // 불필요한 연산 방지를 위해 틱을 끕니다
	RootComponent = SceneRoot;

	// 예측 이동을 위한 컴포넌트 생성 (기본은 비활성)
	MovementComponent = CreateDefaultSubobject<UProjectileMovementComponent>(TEXT("MovementComponent"));
	MovementComponent->bAutoActivate = false;
	MovementComponent->bRotationFollowsVelocity = true;
	MovementComponent->ProjectileGravityScale = 0.0f;

	VisionVisualComp = CreateDefaultSubobject<UVision_VisualComp>(TEXT("VisionVisualComp"));
	// 장판은 시야를 밝히는 용도(Vision Provider)가 아니라 안개에 가려지는 대상(Target)이므로 관련 연산 최적화를 위해 끕니다.
	VisionVisualComp->SetIsVisionProvider(false);
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

			float ActivationTime = Context ? Context->ClientActivationTime : 0.0f;
			if (ActualInstigator)
			{
				Registry->GetAndUnregisterVfxActor(ActualInstigator, ActivationTime);
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
	
	// 여기서 SetActorLocation이 호출되면서 위에서 설정한 콜리전 정보로 즉시 Overlap 이벤트가 발생함
	SetActorLocationAndRotation(SpawnLocation, SpawnRotation);

	// 2. 기본 수명 설정 (서버 트래벌 등 예외 상황에서 고아가 되는 것 방지)
	if (GetLifeSpan() <= 0.0f)
	{
		SetLifeSpan(10.0f); // 10초 후 자동 소멸 (핸드셰이크 성공 시 갱신될 수 있음)
	}

	AActor* ActualInstigator = GetActualInstigator(Parameters);
	const FProjectERGameplayEffectContext* Context = static_cast<const FProjectERGameplayEffectContext*>(Parameters.EffectContext.Get());

	float ActivationTime = Context ? Context->ClientActivationTime : 0.0f;

	if (ActualInstigator)
	{
		if (UWorld* World = GetWorld())
		{
			if (UGCN_SummonedRegistrySubsystem* Registry = World->GetSubsystem<UGCN_SummonedRegistrySubsystem>())
			{
				// [중요] Standalone/ListenServer 호스트 등에서의 중복 실행 방지
				if (ActivationTime > 0.0f && Registry->IsVfxActorRegistered(ActualInstigator, ActivationTime))
				{
					Destroy();
					return;
				}

				// 비주얼 액터 등록 (Simulated Proxy의 경우 ActivationTime이 0.0f이므로 와일드카드로 작동)
				Registry->RegisterVfxActor(ActualInstigator, ActivationTime, this);
			}
		}
	}

	// [Fix] 중복 체크를 통과한 경우에만 실제 비주얼/오디오 초기화 수행 (중복 재생 방지)
	InitializeFromGEC(Parameters.SourceObject.Get(), Parameters);
}

void AGCN_SummonedActor::InitializeFromGEC(const UObject* SourceObject, const FGameplayCueParameters& Parameters)
{
	if (!SourceObject) return;
	
	CachedSourceObject = SourceObject;

	// [Refactor] 특정 GEC 클래스나 부모 GEC에 의존하지 않고 인터페이스(ISkillVisualDataProvider)를 사용합니다.
	if (const ISkillVisualDataProvider* VisualSource = Cast<ISkillVisualDataProvider>(SourceObject))
	{
		// 1. 비주얼(VFX) 초기화 - 인터페이스가 제공하는 컨피그를 사용
		SetupVfxComponent(VisualSource->GetAGCN_NiagaraConfig(), Parameters);

		// 2. 사운드(SFX) 초기화 - 인터페이스가 제공하는 컨피그를 사용
		SetupSfxComponent(VisualSource->GetAGCN_SoundConfig());

		// 3. 이동(Movement) 초기화 - GEC인 경우에만 추가 설정 수행
		if (const UBaseGEC* BaseGEC = Cast<UBaseGEC>(SourceObject))
		{
			if (MovementComponent)
			{
				BaseGEC->SetupMovement(MovementComponent);
				
				// 발사체 성격인 경우 즉시 활성화
				if (MovementComponent->InitialSpeed > 0.0f)
				{
					MovementComponent->Velocity = GetActorForwardVector() * MovementComponent->InitialSpeed;
					MovementComponent->Activate(true);

					// [Fix] 논리 발사체가 미처 도착하기 전에 서버에서 파괴되어 고아가 된 시각 발사체가
					// 벽에 부딪혔을 때 무한정 얼어붙는 현상을 방지하기 위해 정지 이벤트를 바인딩합니다.
					if (!MovementComponent->OnProjectileStop.IsAlreadyBound(this, &AGCN_SummonedActor::OnProjectileStop))
					{
						MovementComponent->OnProjectileStop.AddDynamic(this, &AGCN_SummonedActor::OnProjectileStop);
					}
				}
			}
		}
	}
}

void AGCN_SummonedActor::SetupVfxComponent(const USkillNiagaraSpawnConfig* NiagaraConfig, const FGameplayCueParameters& Parameters)
{
	if (!NiagaraConfig)
	{
		return;
	}

	// [Optimization] 전역 시야 및 거리 최적화 판별
	// 타겟 액터에 부착된 형태라면 타겟 액터의 시야를 따라가고, 바닥에 깔리는 장판이라면 소환물 본인(this)의 시야를 따라갑니다.
	AActor* VisionTarget = CachedTargetActor.IsValid() ? CachedTargetActor.Get() : this;
	
	// 소환물 본인 시야를 따르는 경우 (바닥 장판), 시야 시스템에 정상 등록하기 위해 초기화 및 팀 채널 동기화를 수행합니다.
	if (IsValid(VisionVisualComp))
	{
		AActor* InstigatorActor = GetActualInstigator(Parameters);
		if (IsValid(InstigatorActor))
		{
			if (UVision_VisualComp* InstigatorVisionComp = InstigatorActor->FindComponentByClass<UVision_VisualComp>())
			{
				VisionVisualComp->SetVisionChannel(InstigatorVisionComp->GetVisionChannel());
			}
		}
		VisionVisualComp->Initialize();
	}
	
	const EVfxCullState CullState = USkillVfxCullingHelper::CheckVfxCulling(VisionTarget, Parameters, true);
	
	if (CullState == EVfxCullState::SkipSpawn)
	{
		return; // 단발성이면서 시야 밖이면 아예 스폰하지 않음
	}

	// [Refactor] 전역 헬퍼를 사용하여 VFX 생성 및 설정 통합
	VfxComponent = SkillNiagaraSpawnHelper::SpawnNiagara(GetWorld(), NiagaraConfig, GetActorTransform(), this);

	if (IsValid(VfxComponent))
	{
		if (CullState == EVfxCullState::SpawnHidden)
		{
			VfxComponent->SetVisibility(false);
		}

		if (CullState == EVfxCullState::SpawnAndTrackVision || CullState == EVfxCullState::SpawnHidden)
		{
			if (UVisionParticleManagerSubsystem* VisionSubsystem = GetWorld()->GetSubsystem<UVisionParticleManagerSubsystem>())
			{
				// TargetActor가 아닌 '이 비주얼 액터(this)'를 타겟으로 등록합니다.
				// (실제 시야 컴포넌트(VisionVisualComp)는 이 액터에 부착되어 있기 때문입니다.)
				VisionSubsystem->RegisterParticle(VfxComponent, this);
			}
		}
	}
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

	// 1. 아군/적군/몬스터 색상 판단
	FLinearColor TargetColor = FLinearColor::Red; // 적군 기본
	if (const UWorld* World = GetWorld())
	{
		if (const APlayerController* LocalPC = World->GetFirstPlayerController())
		{
			if (const AActor* LocalPawn = LocalPC->GetPawn())
			{
				const ITargetableInterface* LocalTargetable = Cast<ITargetableInterface>(LocalPawn);
				const ITargetableInterface* InstigatorTargetable = Cast<ITargetableInterface>(InInstigatorActor);

				if (LocalTargetable && InstigatorTargetable)
				{
					// if (InstigatorTargetable->GetTeamType() == ETeamType::Neutral)
					// {
					// 	TargetColor = FLinearColor::White; // 몬스터 / 중립
					// }
					if (LocalTargetable->GetTeamType() == InstigatorTargetable->GetTeamType())
					{
						TargetColor = FLinearColor::Green; // 아군
					}
				}
			}
		}
	}

	// 2. 데칼 크기 및 형태 판별
	// 데칼의 투영 깊이(절반 크기). 기존 500은 총 1000(10미터)의 깊이를 가져 천장에 닿았습니다.
	// 이를 150(총 깊이 300)으로 줄여 바닥 근처의 요철만 덮도록 수정합니다.
	const float DecalDepth = 30; 
	FVector DecalSize = FVector(DecalDepth, 100.f, 100.f);
	int32 ShapeType = 0; 
	FVector2D OutlineExtent = FVector2D(100.f, 100.f);

	// [수직 그림자 투영 계산 공통 로직]
	// 데칼이 바라보는 방향(-90도) 기준으로, 컴포넌트의 로컬 축들이 바닥 평면에 투영되는 길이를 구합니다.
	FRotator CompRot = InCollisionComponent->GetComponentRotation();
	
	// 나이아가라 설정(SpawnConfig)에 설정된 상대 회전값(RotationOffset)이 있다면
	// 파티클이 실제로 기울어지는 각도를 그림자 계산에도 완벽히 동기화합니다.
	if (const ISkillVisualDataProvider* VisualSource = Cast<ISkillVisualDataProvider>(CachedSourceObject.Get()))
	{
		if (const USkillNiagaraSpawnConfig* Config = VisualSource->GetAGCN_NiagaraConfig())
		{
			if (!Config->RotationOffset.IsZero())
			{
				const FTransform CompTransform = InCollisionComponent->GetComponentTransform();
				const FTransform OffsetTransform = FTransform(Config->RotationOffset);
				// 추가 회전이 적용된 최종 실제 회전값 산출
				CompRot = (OffsetTransform * CompTransform).Rotator();
			}
		}
	}

	const FRotator DecalRot = FRotator(-90.f, CompRot.Yaw, 0.f);
	const FVector DecalWorldY = FRotationMatrix(DecalRot).GetScaledAxis(EAxis::Y);
	const FVector DecalWorldZ = FRotationMatrix(DecalRot).GetScaledAxis(EAxis::Z);
	
	// FTransform의 전체 복사 생성 및 InverseTransformVectorNoScale 오버헤드를 제거하고
	// 쿼터니언을 통해 필요한 축에 대해서만 직접 unrotation을 수행합니다.
	const FQuat CompQuat = CompRot.Quaternion();
	const FVector DecalLocalY = CompQuat.UnrotateVector(DecalWorldY);
	const FVector DecalLocalZ = CompQuat.UnrotateVector(DecalWorldZ);

	FVector Shape3DExtent = FVector::ZeroVector;
	FVector2D CanvasExtent = FVector2D(100.f, 100.f);

	if (const USphereComponent* SphereComp = Cast<USphereComponent>(InCollisionComponent))
	{
		const float Radius = SphereComp->GetScaledSphereRadius();
		DecalSize = FVector(DecalDepth, Radius, Radius);
		Shape3DExtent = FVector(Radius, 0.f, 0.f);
		CanvasExtent = FVector2D(Radius, Radius);
		ShapeType = 0; 
	}
	else if (const UBoxComponent* BoxComp = Cast<UBoxComponent>(InCollisionComponent))
	{
		const FVector BoxExtent = BoxComp->GetScaledBoxExtent();
		
		const float ProjectedExtentY = BoxExtent.X * FMath::Abs(DecalLocalY.X) + BoxExtent.Y * FMath::Abs(DecalLocalY.Y) + BoxExtent.Z * FMath::Abs(DecalLocalY.Z);
		const float ProjectedExtentZ = BoxExtent.X * FMath::Abs(DecalLocalZ.X) + BoxExtent.Y * FMath::Abs(DecalLocalZ.Y) + BoxExtent.Z * FMath::Abs(DecalLocalZ.Z);

		// 데칼이 짤리지 않게 가장 넓은 투영 바운드를 캔버스 크기로 잡습니다.
		const float MaxExtent = FMath::Max(ProjectedExtentY, ProjectedExtentZ);
		DecalSize = FVector(DecalDepth, MaxExtent, MaxExtent);
		
		Shape3DExtent = BoxExtent;
		CanvasExtent = FVector2D(MaxExtent, MaxExtent);
		ShapeType = 1; 
	}
	else if (const UCapsuleComponent* CapsuleComp = Cast<UCapsuleComponent>(InCollisionComponent))
	{
		const float Radius = CapsuleComp->GetScaledCapsuleRadius();
		const float HalfHeight = CapsuleComp->GetScaledCapsuleHalfHeight();
		
		const FVector CapExtent(Radius, Radius, HalfHeight);
		const float ProjectedExtentY = CapExtent.X * FMath::Abs(DecalLocalY.X) + CapExtent.Y * FMath::Abs(DecalLocalY.Y) + CapExtent.Z * FMath::Abs(DecalLocalY.Z);
		const float ProjectedExtentZ = CapExtent.X * FMath::Abs(DecalLocalZ.X) + CapExtent.Y * FMath::Abs(DecalLocalZ.Y) + CapExtent.Z * FMath::Abs(DecalLocalZ.Z);

		const float MaxExtent = FMath::Max(ProjectedExtentY, ProjectedExtentZ);
		DecalSize = FVector(DecalDepth, MaxExtent, MaxExtent);
		
		Shape3DExtent = FVector(Radius, HalfHeight, 0.f);
		CanvasExtent = FVector2D(MaxExtent, MaxExtent);
		ShapeType = 2;
	}

	// 4. 컴포넌트 생성/재사용
	bool bIsNewComponent = false;
	if (!CollisionIndicatorComp)
	{
		CollisionIndicatorComp = NewObject<UGroundIndicatorComponent>(this, TEXT("CollisionIndicatorComp"));
		if (!CollisionIndicatorComp) return;
		bIsNewComponent = true;
	}

	// 5. 스케일 세팅 (Plane 기본 100x100 크기 기준)
	// CanvasExtent(반지름) 기준 50으로 나누어 스케일을 맞춥니다.
	CollisionIndicatorComp->SetIndicatorScale(FVector(CanvasExtent.X / 50.0f, CanvasExtent.Y / 50.0f, 1.0f));

	// 6. 동적 머터리얼 세팅
	UMaterialInterface* BaseMaterial = LoadObject<UMaterialInterface>(nullptr, TEXT("/Game/KHB/M_RangeDecal.M_RangeDecal"));
	if (BaseMaterial)
	{
		UMaterialInstanceDynamic* DynMaterial = nullptr;
		if (!bIsNewComponent && CollisionIndicatorComp)
		{
			DynMaterial = Cast<UMaterialInstanceDynamic>(CollisionIndicatorComp->GetIndicatorMaterial(0));
		}

		if (!DynMaterial || DynMaterial->Parent != BaseMaterial)
		{
			// [Fix] UE5 에디터 크래시(ObjectCacheEventSink)를 우회하기 위해 Outer로 GetTransientPackage()를 전달합니다.
			DynMaterial = UMaterialInstanceDynamic::Create(BaseMaterial, GetTransientPackage());
			if (DynMaterial && CollisionIndicatorComp)
			{
				CollisionIndicatorComp->SetIndicatorMaterial(0, DynMaterial);
			}
		}

		if (DynMaterial)
		{
			DynMaterial->SetVectorParameterValue(TEXT("Color"), TargetColor);
			DynMaterial->SetScalarParameterValue(TEXT("ShapeType"), static_cast<float>(ShapeType));
			DynMaterial->SetVectorParameterValue(TEXT("ShapeExtent"), FLinearColor(Shape3DExtent.X, Shape3DExtent.Y, Shape3DExtent.Z, 0.0f));
			DynMaterial->SetVectorParameterValue(TEXT("CanvasExtent"), FLinearColor(CanvasExtent.X, CanvasExtent.Y, 0.0f, 0.0f));
			DynMaterial->SetVectorParameterValue(TEXT("DecalLocalY"), FLinearColor(DecalLocalY.X, DecalLocalY.Y, DecalLocalY.Z, 0.0f));
			DynMaterial->SetVectorParameterValue(TEXT("DecalLocalZ"), FLinearColor(DecalLocalZ.X, DecalLocalZ.Y, DecalLocalZ.Z, 0.0f));
			DynMaterial->SetScalarParameterValue(TEXT("SpawnTime"), GetWorld()->GetTimeSeconds());

			float FadeDuration = 2.0f;
			if (const USummonRangeBaseGEC* SummonGEC = Cast<USummonRangeBaseGEC>(GetSourceObject()))
			{
				FadeDuration = SummonGEC->LifeSpan;
			}
			else
			{
				float Lifespan = GetLifeSpan();
				if (Lifespan > 0.0f)
				{
					FadeDuration = Lifespan;
				}
			}

			if (FadeDuration <= 0.0f)
			{
				FadeDuration = 10.0f;
			}
			DynMaterial->SetScalarParameterValue(TEXT("FadeDuration"), FadeDuration);
		}
	}

	// 7. 부착 및 렌더링 활성화
	if (bIsNewComponent && CollisionIndicatorComp)
	{
		// GEC 옵션을 확인하여 인디케이터가 뼈 움직임(상하 흔들림)을 실시간 추적해야 하는지 설정합니다.
		bool bIsBoneAttached = false;
		if (const USummonRangeAtBone* BoneGEC = Cast<USummonRangeAtBone>(GetSourceObject()))
		{
			bIsBoneAttached = BoneGEC->bAttachToBone;
		}
		CollisionIndicatorComp->SetTrackingDynamicGround(bIsBoneAttached);

		// UGroundIndicatorComponent 자체에 스프링암 상속 규칙이 내재되어 있으므로 단순 부착만 수행합니다.
		CollisionIndicatorComp->AttachToComponent(InCollisionComponent, FAttachmentTransformRules::SnapToTargetNotIncludingScale);
		CollisionIndicatorComp->RegisterComponent();
	}
}

void AGCN_SummonedActor::AttachToTargetActor(AActor* InTargetActor)
{
	if (!IsValid(InTargetActor)) return;

	CachedTargetActor = InTargetActor;

	// [Fix] 비주얼 액터 본체(this)도 논리 액터에 부착하여, 시야 컴포넌트(VisionVisualComp)가 파티클과 함께 움직이도록 보장합니다.
	FAttachmentTransformRules AttachRules(EAttachmentRule::SnapToTarget, true);
	AttachToActor(InTargetActor, AttachRules);

	const ISkillVisualDataProvider* VisualSource = Cast<ISkillVisualDataProvider>(GetSourceObject());
	if (VisualSource)
	{
		// VFX 이전
		if (UNiagaraComponent* NiagaraComp = GetVfxComponent())
		{
			SkillNiagaraSpawnHelper::AttachNiagaraByConfig(NiagaraComp, InTargetActor->GetRootComponent(), VisualSource->GetAGCN_NiagaraConfig());
		}

		// SFX 이전
		if (UAudioComponent* SfxComp = GetSfxComponent())
		{
			SkillSoundSpawnHelper::AttachSoundByConfig(SfxComp, InTargetActor->GetRootComponent(), VisualSource->GetAGCN_SoundConfig());
		}
	}

	// 중복 바인딩 방지 (ensure 방지)
	if (!InTargetActor->OnDestroyed.IsAlreadyBound(this, &AGCN_SummonedActor::OnTargetActorDestroyed))
	{
		InTargetActor->OnDestroyed.AddDynamic(this, &AGCN_SummonedActor::OnTargetActorDestroyed);
	}

	// 콜리전 메쉬의 아군/적군 테두리 렌더링 활성화
	if (UShapeComponent* ShapeComp = InTargetActor->FindComponentByClass<UShapeComponent>())
	{
		SetupCollisionOutline(ShapeComp, InTargetActor->GetInstigator());
	}
}

void AGCN_SummonedActor::OnProjectileStop(const FHitResult& ImpactResult)
{
	// 논리 발사체와 아직 결합(Handshake)되지 않은 고아(Orphaned) 시각 발사체가 
	// 벽이나 바닥에 부딪혀 정지한 경우, 무한정 얼어붙는 현상을 방지하기 위해 스스로 파괴합니다.
	if (!CachedTargetActor.IsValid())
	{
		Destroy();
	}
}

