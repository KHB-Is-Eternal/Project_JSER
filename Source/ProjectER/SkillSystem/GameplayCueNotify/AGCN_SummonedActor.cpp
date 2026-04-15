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
	InitializeFromGEC(Parameters.SourceObject.Get());

	AActor* ActualInstigator = GetActualInstigator(Parameters);
	const FProjectERGameplayEffectContext* Context = static_cast<const FProjectERGameplayEffectContext*>(Parameters.EffectContext.Get());

	if (ActualInstigator && Context && Context->ClientActivationTime > 0.0f)
	{
		UE_LOG(LogTemp, Log, TEXT("AGCN_SummonedActor::HandleSummonedVfx - Registering VFX: %s for Instigator: %s, Time: %f"), 
			*GetName(), *ActualInstigator->GetName(), Context->ClientActivationTime);

		if (UWorld* World = GetWorld())
		{
			if (UGCN_SummonedRegistrySubsystem* Registry = World->GetSubsystem<UGCN_SummonedRegistrySubsystem>())
			{
				// 시전자 + 시전 시간을 키로 사용하여 비주얼 액터 등록
				Registry->RegisterVfxActor(ActualInstigator, Context->ClientActivationTime, this);
			}
		}
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("AGCN_SummonedActor::HandleSummonedVfx - Skip Registration. Instigator: %s, Time: %f"), 
			ActualInstigator ? *ActualInstigator->GetName() : TEXT("nullptr"),
			Context ? Context->ClientActivationTime : 0.0f);
	}
}

void AGCN_SummonedActor::InitializeFromGEC(const UObject* SourceObject)
{
	if (!SourceObject) return;
	
	CachedSourceObject = const_cast<UObject*>(SourceObject);

	UE_LOG(LogTemp, Log, TEXT("AGCN_SummonedActor::InitializeFromGEC - SourceObject: %s"), *SourceObject->GetName());

	// 1. 공통 설정 (수명 등)
	if (const USummonRangeBaseGEC* RangeGEC = Cast<USummonRangeBaseGEC>(SourceObject))
	{
		SetLifeSpan(RangeGEC->LifeSpan);
		
		// 2. 비주얼(VFX) 초기화
		SetupVfxComponent(RangeGEC->RangeSpawnVfx.Get());
	}
	else if (const ULaunchHomingMissile* MissileGEC = Cast<ULaunchHomingMissile>(SourceObject))
	{
		SetLifeSpan(MissileGEC->LifeSpan);
		
		// 2. 비주얼(VFX) 초기화
		SetupVfxComponent(MissileGEC->MissileVfx.Get());
	}

	// 3. 이동(Movement) 초기화
	SetupMovementComponent(SourceObject);
}

void AGCN_SummonedActor::SetupVfxComponent(const USkillNiagaraSpawnConfig* NiagaraConfig)
{
	if(!NiagaraConfig){
		return;
	}

	if(NiagaraConfig->NiagaraSystem.IsNull()){
		return;
	}

	// if (!NiagaraConfig || NiagaraConfig->NiagaraSystem.IsNull())
	// {
	// 	return;
	// }

	// 기존 컴포넌트가 없다면 생성 (미래의 풀링을 위해 별도 함수화 고려 가능)
	if (!VfxComponent)
	{
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

void AGCN_SummonedActor::SetupMovementComponent(const UObject* SourceObject)
{
	if (!MovementComponent || !SourceObject) return;

	// 발사체 타입인 경우 속도 적용
	if (const ULaunchProjectile* ProjectileGEC = Cast<ULaunchProjectile>(SourceObject))
	{
		MovementComponent->InitialSpeed = ProjectileGEC->InitialSpeed;
		MovementComponent->MaxSpeed = ProjectileGEC->InitialSpeed;
		MovementComponent->ProjectileGravityScale = ProjectileGEC->GravityScale;

		// 즉시 활성화 및 초기 속도 설정
		MovementComponent->Velocity = GetActorForwardVector() * ProjectileGEC->InitialSpeed;
		MovementComponent->Activate(true);
	}
	else if (const ULaunchHomingMissile* MissileGEC = Cast<ULaunchHomingMissile>(SourceObject))
	{
		MovementComponent->InitialSpeed = MissileGEC->InitialSpeed;
		MovementComponent->MaxSpeed = MissileGEC->MaxSpeed;
		MovementComponent->ProjectileGravityScale = 0.0f; // 미사일 상수로 0 설정
		
		// 미사일 특화 설정이 필요하다면 추가 (예: 가속도 정보 등)
		
		// 즉시 활성화 및 초기 속도 설정
		MovementComponent->Velocity = GetActorForwardVector() * MissileGEC->InitialSpeed;
		MovementComponent->Activate(true);
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
