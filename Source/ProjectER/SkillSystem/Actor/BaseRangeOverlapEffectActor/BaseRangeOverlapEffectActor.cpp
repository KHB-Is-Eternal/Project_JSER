// Fill out your copyright notice in the Description page of Project Settings.


#include "SkillSystem/Actor/BaseRangeOverlapEffectActor/BaseRangeOverlapEffectActor.h"
#include "Components/ShapeComponent.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "CharacterSystem/Interface/TargetableInterface.h"
#include "SkillSystem/Component/AreaPeriodicEffectComponent.h"
#include "UObject/Object.h"
#include "Net/UnrealNetwork.h"
#include "SkillSystem/GameplayCueNotify/GCN_SummonedRegistrySubsystem.h"

// Sets default values
ABaseRangeOverlapEffectActor::ABaseRangeOverlapEffectActor()
{
	PrimaryActorTick.bCanEverTick = false;
	bReplicates = true;
}

#include "SkillSystem/GameplayCueNotify/AGCN_SummonedActor.h"
#include "SkillSystem/GameplayEffectComponent/SummonRangeBaseGEC.h"

void ABaseRangeOverlapEffectActor::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(ABaseRangeOverlapEffectActor, ClientActivationTime);
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
			// (시전자 + 시전 시간) 조합으로 검색
			if (AActor* VfxActor = Registry->GetAndUnregisterVfxActor(InstigatorActor, ClientActivationTime))
			{
				// 찾았다면 자신에게 부착 (Snap to Target)
				FAttachmentTransformRules AttachRules(EAttachmentRule::SnapToTarget, true);
				VfxActor->AttachToActor(this, AttachRules);
				
				// [고급 동기화] 비주얼 액터가 들고 있는 SourceObject(GEC)로부터 콜리전 설정값 동기화
				if (AGCN_SummonedActor* SummonedGCN = Cast<AGCN_SummonedActor>(VfxActor))
				{
					if (const USummonRangeBaseGEC* RangeGEC = Cast<USummonRangeBaseGEC>(SummonedGCN->GetSourceObject()))
					{
						// 장판 크기 적용 (CollisionRadius가 FVector 타입이므로 X나 적절한 성분 활용)
						float Radius = (float)RangeGEC->CollisionRadius.X;
						ApplyCollisionSize(FVector(Radius, Radius, 100.0f));
						
						UE_LOG(LogTemp, Log, TEXT("ABaseRangeOverlapEffectActor: Synced CollisionSize from GEC (Radius: %f)"), Radius);
					}
				}
				
				UE_LOG(LogTemp, Log, TEXT("ABaseRangeOverlapEffectActor: Successfully attached VFX Actor with ClientActivationTime"));
			}
		}
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
	if (MyInstigatorTargetable->GetTeamType() == OtherTargetable->GetTeamType()) {
		return;
	}

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

	if (bDestroyOnOverlap)
	{
		Destroy();
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

	for (const FGameplayEffectSpecHandle& EffectSpecHandle : EffectSpecHandles)
	{
		if (EffectSpecHandle.IsValid())
		{
			InstigatorASC->ApplyGameplayEffectSpecToTarget(*EffectSpecHandle.Data.Get(), TargetASC);
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
}

