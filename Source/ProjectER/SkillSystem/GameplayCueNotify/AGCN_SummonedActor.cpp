#include "SkillSystem/GameplayCueNotify/AGCN_SummonedActor.h"
#include "SkillSystem/GameplayCueNotify/GCN_SummonedRegistrySubsystem.h"
#include "SkillSystem/GameplayEffectComponent/SummonRangeBaseGEC.h"
#include "SkillSystem/GameplayEffectComponent/LaunchProjectile.h"
#include "SkillSystem/GameplayEffectComponent/LaunchHomingMissile.h"
#include "SkillSystem/GameplayCueNotify/Particle/SkillNiagaraSpawnConfig.h"
#include "SkillSystem/GAS/ProjectERGameplayEffectContext.h"
#include "NiagaraComponent.h"
#include "NiagaraFunctionLibrary.h"
#include "GameFramework/ProjectileMovementComponent.h"
#include "GameplayPrediction.h"

AGCN_SummonedActor::AGCN_SummonedActor()
{
	// GCN 액터는 기본적으로 클라이언트에서 실행되므로 리플리케이션은 끄는 것이 일반적입니다.
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

	InitializeFromGEC(Parameters.SourceObject.Get());

	AActor* ActualInstigator = GetActualInstigator(Parameters);
	const FProjectERGameplayEffectContext* Context = static_cast<const FProjectERGameplayEffectContext*>(Parameters.EffectContext.Get());

	if (ActualInstigator && Context && Context->ClientActivationTime > 0.0f)
	{
		if (UWorld* World = GetWorld())
		{
			if (UGCN_SummonedRegistrySubsystem* Registry = World->GetSubsystem<UGCN_SummonedRegistrySubsystem>())
			{
				// [중요] Standalone/ListenServer 호스트 등에서의 중복 실행 방지
				// NetMode 체크 대신 레지스트리에 이미 등록된 키가 있는지 확인하는 데이터 기반 로직으로 처리
				if (Registry->IsVfxActorRegistered(ActualInstigator, Context->ClientActivationTime))
				{
					UE_LOG(LogTemp, Log, TEXT("AGCN_SummonedActor::HandleSummonedVfx - Duplicate VFX suppressed for Instigator: %s at Time: %f"), 
						*ActualInstigator->GetName(), Context->ClientActivationTime);
					Destroy();
					return;
				}

				// 비주얼 액터 등록
				Registry->RegisterVfxActor(ActualInstigator, Context->ClientActivationTime, this);
			}
		}
	}
}

void AGCN_SummonedActor::InitializeFromGEC(const UObject* SourceObject)
{
	if (!SourceObject) return;
	
	CachedSourceObject = const_cast<UObject*>(SourceObject);

	UE_LOG(LogTemp, Log, TEXT("AGCN_SummonedActor::InitializeFromGEC - SourceObject: %s"), *SourceObject->GetName());

	// [Refactor] 특정 GEC 클래스나 부모 GEC에 의존하지 않고 인터페이스(IProjectERSummonedActorInterface)를 사용합니다.
	if (const IProjectERSummonedActorInterface* VisualSource = Cast<IProjectERSummonedActorInterface>(SourceObject))
	{
		// 1. 비주얼(VFX) 초기화 - 인터페이스가 제공하는 컨피그를 사용
		SetupVfxComponent(VisualSource->GetAGCN_NiagaraConfig());

		// 2. 이동(Movement) 초기화 - GEC인 경우에만 추가 설정 수행
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
	if(!NiagaraConfig){
		return;
	}

	if(NiagaraConfig->NiagaraSystem.IsNull()){
		return;
	}


	// 기존 컴포넌트가 없다면 생성 (미래의 풀링을 위해 별도 함수화 고려 가능)
	if (!VfxComponent)
	{
		if (USceneComponent* RootComp = GetRootComponent())
		{
			UE_LOG(LogTemp, Log, TEXT("AGCN_SummonedActor::SetupVfxComponent - Attaching VFX to RootComponent: %s"), *RootComp->GetName());
		}
		else
		{
			UE_LOG(LogTemp, Warning, TEXT("AGCN_SummonedActor::SetupVfxComponent - RootComponent is null!"));
		}

		VfxComponent = UNiagaraFunctionLibrary::SpawnSystemAttached(
			NiagaraConfig->NiagaraSystem.LoadSynchronous(),
			GetRootComponent(),
			NiagaraConfig->SocketOrBoneName,
			NiagaraConfig->LocationOffset,
			NiagaraConfig->RotationOffset,
			EAttachLocation::KeepRelativeOffset,
			true
		);
	}
	else
	{
		VfxComponent->SetAsset(NiagaraConfig->NiagaraSystem.LoadSynchronous());
		VfxComponent->Activate(true);
	}

	if (VfxComponent)
	{
		// 파라미터 적용 로직 (생략 가능하나 확장성을 위해 남겨둠)
		for (auto& Pair : NiagaraConfig->FloatParameters) VfxComponent->SetVariableFloat(Pair.Key, Pair.Value);
		for (auto& Pair : NiagaraConfig->VectorParameters) VfxComponent->SetVariableVec3(Pair.Key, Pair.Value);
		for (auto& Pair : NiagaraConfig->ColorParameters) VfxComponent->SetVariableLinearColor(Pair.Key, Pair.Value);
	}
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
	UE_LOG(LogTemp, Warning, TEXT("[CLIENT] AGCN_SummonedActor::OnTargetActorDestroyed - Time: %f, Target: %s, VFX: %s"), 
		GetWorld()->GetTimeSeconds(),
		DestroyedActor ? *DestroyedActor->GetName() : TEXT("nullptr"),
		*GetName());
		
	if (VfxComponent)
	{
		// 파티클 잔상이나 페이드 아웃 없이 즉시 월드에서 컴포넌트 자체를 강제 파괴/제거합니다.
		VfxComponent->DestroyComponent();
	}

	Destroy();
}
