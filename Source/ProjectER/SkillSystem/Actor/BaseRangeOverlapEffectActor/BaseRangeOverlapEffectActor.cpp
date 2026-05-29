// Fill out your copyright notice in the Description page of Project Settings.


#include "SkillSystem/Actor/BaseRangeOverlapEffectActor/BaseRangeOverlapEffectActor.h"
#include "Components/ShapeComponent.h"
#include "NiagaraComponent.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "CharacterSystem/Interface/TargetableInterface.h"
#include "SkillSystem/Component/AreaPeriodicEffectComponent.h"
#include "UObject/Object.h"
#include "Net/UnrealNetwork.h"
#include "SkillSystem/GameplayCueNotify/GCN_SummonedRegistrySubsystem.h"
#include "SkillSystem/GameplayCueNotify/AGCN_SummonedActor.h"
#include "SkillSystem/GameplayEffectComponent/SummonRangeBaseGEC.h"
#include "SkillSystem/GameplayEffect/BaseGameplayEffect.h"
#include "SkillSystem/GameAbility/SkillBase.h"
#include "Components/AudioComponent.h"
#include "SkillSystem/GameplayCueNotify/Particle/SkillNiagaraSpawnConfig.h"
#include "SkillSystem/GameplayCueNotify/Particle/SkillNiagaraSpawnHelper.h"
#include "SkillSystem/GameplayCueNotify/Sound/SkillSoundSpawnConfig.h"
#include "SkillSystem/GameplayCueNotify/Sound/SkillSoundSpawnHelper.h"
#include "SkillSystem/Interfaces/SkillVisualDataProvider.h"

// Sets default values
ABaseRangeOverlapEffectActor::ABaseRangeOverlapEffectActor()
{
	PrimaryActorTick.bCanEverTick = false;
	bReplicates = true;

	// [Network Optimization] 장판 생성/파괴 소식을 클라이언트에 최고 속도로 전송합니다.
	//SetNetUpdateFrequency(100.0f);
	//NetPriority = 3.0f;
}

void ABaseRangeOverlapEffectActor::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(ABaseRangeOverlapEffectActor, ClientActivationTime);
	DOREPLIFETIME(ABaseRangeOverlapEffectActor, InstigatorActor);
	DOREPLIFETIME(ABaseRangeOverlapEffectActor, PendingCollisionSize);
}

void ABaseRangeOverlapEffectActor::OnRep_InstigatorActor()
{
	// 데이터가 도착하면 핸드셰이크 재시도
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

void ABaseRangeOverlapEffectActor::OnRep_PendingCollisionSize()
{
	ApplyCollisionSize(PendingCollisionSize);

	if (CachedSummonedGCN.IsValid())
	{
		CachedSummonedGCN->SetupCollisionOutline(CollisionComponent, InstigatorActor);
	}
}

void ABaseRangeOverlapEffectActor::PostNetInit()
{
	Super::PostNetInit();

	// 클라이언트에서만 작동 (로컬 서버 포함)
	if (GetNetMode() == NM_DedicatedServer) return;

	// 서브시스템에서 일치하는 비주얼 액터 검색
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
				UE_LOG(LogTemp, Warning, TEXT("ABaseRangeOverlapEffectActor: Handshake Failed in PostNetInit. (Registering as Pending)"));
				Registry->RegisterPendingActorFuzzy(InstigatorActor, ClientActivationTime, this);
			}
		}
	}
}

void ABaseRangeOverlapEffectActor::OnVfxHandshakeCompleted_Implementation(AActor* VfxActor)
{
	if (!VfxActor) return;

	// [Fix] 언리얼 생명주기 싱크 맞추기 위해 OnDestroyed 델리게이트에 VFX 액터를 바인딩
	if (AGCN_SummonedActor* SummonedGCN = Cast<AGCN_SummonedActor>(VfxActor))
	{
		CachedSummonedGCN = SummonedGCN;

		const ISkillVisualDataProvider* VisualSource = Cast<ISkillVisualDataProvider>(SummonedGCN->GetSourceObject());

		// [Refactor] 헬퍼를 사용하여 VFX/SFX 핸드셰이크(이전 부착) 수행
		if (VisualSource)
		{
			// VFX 이전
			if (UNiagaraComponent* NiagaraComp = SummonedGCN->GetVfxComponent())
			{
				SkillNiagaraSpawnHelper::AttachNiagaraByConfig(NiagaraComp, GetRootComponent(), VisualSource->GetAGCN_NiagaraConfig());
			}

			// SFX 이전
			if (UAudioComponent* SfxComp = SummonedGCN->GetSfxComponent())
			{
				SkillSoundSpawnHelper::AttachSoundByConfig(SfxComp, GetRootComponent(), VisualSource->GetAGCN_SoundConfig());
			}
		}

		// [Fix] 중복 바인딩 방지 (ensure 방지)
		if (!this->OnDestroyed.IsAlreadyBound(SummonedGCN, &AGCN_SummonedActor::OnTargetActorDestroyed))
		{
			this->OnDestroyed.AddDynamic(SummonedGCN, &AGCN_SummonedActor::OnTargetActorDestroyed);
		}

		// [고급 동기화] 비주얼 액터가 들고 있는 SourceObject(GEC)로부터 콜리전 설정값 동기화
		if (const USummonRangeBaseGEC* RangeGEC = Cast<USummonRangeBaseGEC>(SummonedGCN->GetSourceObject()))
		{
			// 장판 크기 적용 (CollisionRadius가 FVector 타입이므로 X나 적절한 성분 활용)
			float Radius = (float)RangeGEC->CollisionRadius.X;
			ApplyCollisionSize(FVector(Radius, Radius, 100.0f));
		}

		// [Fix] 콜리전 메쉬의 아군/적군 테두리 렌더링 활성화
		SummonedGCN->SetupCollisionOutline(CollisionComponent, InstigatorActor);
	}
}

void ABaseRangeOverlapEffectActor::InitializeEffectData(const TArray<FGameplayEffectSpecHandle>& InEffectSpecHandles, AActor* InInstigatorActor, const FVector& InCollisionSize, bool bInHitOncePerTarget, const UObject* InHitTargetCueSourceObject, const FGameplayCueParameters& InHitTargetVfxCueParameters, const FGameplayCueParameters& InHitTargetSoundCueParameters)
{
	EffectSpecHandles = InEffectSpecHandles;
	InstigatorActor = InInstigatorActor;
	SetInstigator(Cast<APawn>(InInstigatorActor));



	bHitOncePerTarget = bInHitOncePerTarget;
	HitTargetCueSourceObject = InHitTargetCueSourceObject;
	HitTargetVfxCueParameters = InHitTargetVfxCueParameters;
	HitTargetSoundCueParameters = InHitTargetSoundCueParameters;

	PendingCollisionSize = InCollisionSize;
	bHasPendingCollisionSize = true;
	ApplyCollisionSize(PendingCollisionSize);

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

// Called when the game starts or when spawned
void ABaseRangeOverlapEffectActor::BeginPlay()
{
	Super::BeginPlay();
    
	if (HasAuthority() && IsValid(CollisionComponent))
	{
		CollisionComponent->OnComponentBeginOverlap.AddDynamic(this, &ABaseRangeOverlapEffectActor::OnShapeBeginOverlap);
		CollisionComponent->OnComponentEndOverlap.AddDynamic(this, &ABaseRangeOverlapEffectActor::OnShapeEndOverlap);
	}
}

void ABaseRangeOverlapEffectActor::ApplyCollisionSize(const FVector& InCollisionSize)
{
	//
}

void ABaseRangeOverlapEffectActor::SetCollisionComponent(UShapeComponent* InCollisionComponent)
{
	if (!IsValid(InCollisionComponent))
	{
		return;
	}

	// 1. 멤버 변수 할당
	CollisionComponent = InCollisionComponent;

	// 2. 물리 및 충돌 설정 (공통)
	CollisionComponent->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	CollisionComponent->SetCollisionProfileName(TEXT("OverlapAllDynamic"));
	CollisionComponent->SetGenerateOverlapEvents(true);

	if (GetRootComponent() != CollisionComponent)
	{
		SetRootComponent(CollisionComponent);
	}

	if (CollisionComponent->IsRegistered())
	{
		CollisionComponent->UpdateBounds();
		CollisionComponent->MarkRenderStateDirty();
		CollisionComponent->UpdateBodySetup(); // 물리 모양 갱신
	}
}

void ABaseRangeOverlapEffectActor::OnShapeBeginOverlap(UPrimitiveComponent* OverlappedComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	if (!HasAuthority() || !IsValid(OtherActor) || OtherActor == this || OtherActor == InstigatorActor)
	{
		return;
	}

	ITargetableInterface* MyInstigatorTargetable = Cast<ITargetableInterface>(InstigatorActor);
	ITargetableInterface* OtherTargetable = Cast<ITargetableInterface>(OtherActor);
	if(!MyInstigatorTargetable || !OtherTargetable) return;


	// 주기적 효과가 설정되어 있다면 컴포넌트에 타겟 추가
	if (IsValid(AreaPeriodicComponent))
	{
		AreaPeriodicComponent->AddTarget(OtherActor);
		return;
	}

	if (bHitOncePerTarget && HitActors.Contains(OtherActor))
	{
		return;
	}

	ApplyEffectsToTarget(OtherActor);	

	if (bHitOncePerTarget)
	{
		HitActors.Add(OtherActor);
	}
}

void ABaseRangeOverlapEffectActor::OnShapeEndOverlap(UPrimitiveComponent* OverlappedComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex)
{
	if (HasAuthority() && IsValid(OtherActor) && IsValid(AreaPeriodicComponent))
	{
		AreaPeriodicComponent->RemoveTarget(OtherActor);
	}
}

void ABaseRangeOverlapEffectActor::SetAreaPeriodicComponent(UAreaPeriodicEffectComponent* InComponent)
{
	if (IsValid(InComponent))
	{
		AreaPeriodicComponent = InComponent;
		AreaPeriodicComponent->OnAreaPeriodicTrigger.AddDynamic(this, &ABaseRangeOverlapEffectActor::OnAreaPeriodicTrigger);
	}
}

void ABaseRangeOverlapEffectActor::InitializePeriodicCues(const FGameplayCueParameters& InPeriodicVfxCueParameters, const FGameplayCueParameters& InPeriodicSoundCueParameters)
{
	PeriodicVfxCueParameters = InPeriodicVfxCueParameters;
	PeriodicSoundCueParameters = InPeriodicSoundCueParameters;
}

void ABaseRangeOverlapEffectActor::OnAreaPeriodicTrigger(const TArray<AActor*>& Targets)
{
	UAbilitySystemComponent* InstigatorASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(InstigatorActor);
	if (IsValid(InstigatorASC))
	{
		// Periodic VFX
		if (PeriodicVfxCueParameters.OriginalTag.IsValid())
		{
			FGameplayCueParameters Params = PeriodicVfxCueParameters;
			Params.Location = GetActorLocation();
			Params.EffectCauser = this;
			{
				FScopedPredictionWindow ForcedWindow(InstigatorASC, FPredictionKey(), false);
				InstigatorASC->ExecuteGameplayCue(Params.OriginalTag, Params);
			}
		}

		// Periodic Sound
		if (PeriodicSoundCueParameters.OriginalTag.IsValid())
		{
			FGameplayCueParameters Params = PeriodicSoundCueParameters;
			Params.Location = GetActorLocation();
			Params.EffectCauser = this;
			{
				FScopedPredictionWindow ForcedWindow(InstigatorASC, FPredictionKey(), false);
				InstigatorASC->ExecuteGameplayCue(Params.OriginalTag, Params);
			}
		}
	}

	ApplyEffectsToTargets(Targets);
}

void ABaseRangeOverlapEffectActor::ApplyEffectsToTargets(const TArray<AActor*>& Targets)
{
	for (AActor* Target : Targets)
	{
		ApplyEffectsToTarget(Target);
	}
}

void ABaseRangeOverlapEffectActor::ApplyEffectsToTarget(AActor* TargetActor)
{
	UAbilitySystemComponent* TargetASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(TargetActor);
	UAbilitySystemComponent* InstigatorASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(InstigatorActor);

	if (!IsValid(InstigatorASC)) return;
	if (!IsValid(TargetASC)) return;
	if (EffectSpecHandles.Num() <= 0) return;

	bool bSuccessApplyGE = false;

	for (const FGameplayEffectSpecHandle& EffectSpecHandle : EffectSpecHandles)
	{
		if (EffectSpecHandle.IsValid())
		{
			// GE 레벨의 타겟팅 속성을 검사하여 부여 전에 필터링
			if (EffectSpecHandle.Data->Def)
			{
				const UBaseGameplayEffect* BaseGE = Cast<UBaseGameplayEffect>(EffectSpecHandle.Data->Def.Get());
				if (BaseGE)
				{
					if (!USkillBase::IsValidRelationship(InstigatorActor, TargetActor, BaseGE->TargetRelationship))
					{
						continue;
					}
				}
			}

			InstigatorASC->ApplyGameplayEffectSpecToTarget(*EffectSpecHandle.Data.Get(), TargetASC);
			bSuccessApplyGE = true;
		}
	}

	// VFX
	if (HitTargetVfxCueParameters.OriginalTag.IsValid())
	{
		FGameplayCueParameters CueParameters = HitTargetVfxCueParameters;
		CueParameters.Location = TargetActor->GetActorLocation();
		CueParameters.EffectCauser = this;
		CueParameters.TargetAttachComponent = TargetActor->GetRootComponent();

		{
			FScopedPredictionWindow ForcedWindow(InstigatorASC, FPredictionKey(), false);
			InstigatorASC->ExecuteGameplayCue(CueParameters.OriginalTag, CueParameters);
		}
	}

	// Sound
	if (HitTargetSoundCueParameters.OriginalTag.IsValid())
	{
		FGameplayCueParameters CueParameters = HitTargetSoundCueParameters;
		CueParameters.Location = TargetActor->GetActorLocation();
		CueParameters.EffectCauser = this;
		CueParameters.TargetAttachComponent = TargetActor->GetRootComponent();

		{
			FScopedPredictionWindow ForcedWindow(InstigatorASC, FPredictionKey(), false);
			InstigatorASC->ExecuteGameplayCue(CueParameters.OriginalTag, CueParameters);
		}
	}

	if (bSuccessApplyGE && bDestroyOnOverlap)
	{
		Destroy();
	}
}

