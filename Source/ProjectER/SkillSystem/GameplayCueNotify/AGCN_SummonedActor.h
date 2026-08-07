#pragma once

#include "CoreMinimal.h"
#include "GameplayCueNotify_Actor.h"
#include "Components/SceneComponent.h"
#include "AGCN_SummonedActor.generated.h"

class UNiagaraComponent;
class UAudioComponent;
class UProjectileMovementComponent;
class USkillNiagaraSpawnConfig;
class USkillSoundSpawnConfig;
class UDecalComponent;
class UTexture2D;
class UVision_VisualComp;

/**
 * 소환물 비주얼을 담당하며 예측 키를 통해 판정 액터와 동기화되는 GCN 액터
 */
UCLASS()
class PROJECTER_API AGCN_SummonedActor : public AGameplayCueNotify_Actor
{
	GENERATED_BODY()

public:
	AGCN_SummonedActor();

	/** 캐싱된 GEC 데이터를 반환합니다. */
	const UObject* GetSourceObject() const { return CachedSourceObject.Get(); }

	/** 콜리전 메쉬의 아웃라인을 아군/적군 여부에 따라 설정합니다. */
	UFUNCTION(BlueprintCallable, Category = "ProjectER|GameplayCue")
	void SetupCollisionOutline(UShapeComponent* InCollisionComponent, AActor* InInstigatorActor);

	/** 대상 판정 액터(장판 등)에 자신을 부착하고 비주얼/사운드 설정 및 생명주기를 연동합니다. */
	UFUNCTION(BlueprintCallable, Category = "ProjectER|GameplayCue")
	void AttachToTargetActor(AActor* InTargetActor);

	/** 부모 이동으로 인해 자식의 트랜스폼이 갱신되었을 때 명시적으로 오버랩을 갱신하여 호스트 Sweep 버그를 방지합니다. */
	void OnTransformUpdated(USceneComponent* UpdatedComponent, EUpdateTransformFlags UpdateTransformFlags, ETeleportType Teleport);

	/** 타겟 액터가 파괴될 때 호출되어 자신도 파괴합니다. */
	UFUNCTION()
	void OnTargetActorDestroyed(AActor* DestroyedActor);

	/** 나이아가라 컴포넌트를 외부에서 부착할 수 있도록 반환합니다. */
	UFUNCTION(BlueprintCallable, Category = "ProjectER|GameplayCue")
	UNiagaraComponent* GetVfxComponent() const { return VfxComponent; }

	/** 오디오 컴포넌트를 외부에서 부착할 수 있도록 반환합니다. */
	UFUNCTION(BlueprintCallable, Category = "ProjectER|GameplayCue")
	UAudioComponent* GetSfxComponent() const { return SfxComponent; }

	/** 고아 발사체가 벽에 부딪혀 정지했을 때 자신을 파괴합니다. */
	UFUNCTION()
	void OnProjectileStop(const FHitResult& ImpactResult);

protected:
	virtual bool OnExecute_Implementation(AActor* MyTarget, const FGameplayCueParameters& Parameters) override;
	virtual bool WhileActive_Implementation(AActor* MyTarget, const FGameplayCueParameters& Parameters) override;
	virtual bool OnRemove_Implementation(AActor* MyTarget, const FGameplayCueParameters& Parameters) override;

	/** 공통 등록 및 초기화 로직 */
	void HandleSummonedVfx(const FGameplayCueParameters& Parameters);

	/** GEC 데이터로부터 속성 초기화 */
	void InitializeFromGEC(const UObject* SourceObject, const FGameplayCueParameters& Parameters);

	/** Parameters와 Context로부터 실제 시전자를 찾아 반환합니다. */
	AActor* GetActualInstigator(const FGameplayCueParameters& Parameters) const;

	/** 나이아가라 컴포넌트 초기화 및 재생 */
	void SetupVfxComponent(const USkillNiagaraSpawnConfig* NiagaraConfig, const FGameplayCueParameters& Parameters);

	/** 오디오 컴포넌트 초기화 및 재생 */
	void SetupSfxComponent(const USkillSoundSpawnConfig* SoundConfig);

private:
	/** 시전자의 비전 채널을 읽어와 이 장판 액터의 비전 채널을 동기화하고 FOW 시스템에 등록/갱신합니다. */
	void SyncVisionChannelWithInstigator(AActor* InInstigator);

	/** 호스트/로컬 컨트롤 환경에서의 물리 이동 누락 시 오버랩 갱신을 수동으로 처리하도록 설정합니다. */
	void SetupManualOverlapUpdate(AActor* InTargetActor);

public:
	/** 비주얼을 담당하는 나이아가라 컴포넌트 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "ProjectER | Visual")
	TObjectPtr<UNiagaraComponent> VfxComponent;

	/** 오디오 컴포넌트 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "ProjectER | Audio")
	TObjectPtr<UAudioComponent> SfxComponent;

	/** 시야 탐지 및 루트 역할을 하는 컴포넌트 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "ProjectER | Collision")
	TObjectPtr<class USphereComponent> SceneRoot;

	/** 예측 이동을 담당하는 컴포넌트 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "ProjectER|GameplayCue|Component")
	TObjectPtr<class UProjectileMovementComponent> MovementComponent;

	/** 데칼 메쉬 컴포넌트: 범위나 경로를 지면에 그려주는 역할 (스스로 지면을 찾아갑니다) */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "ProjectER|GameplayCue|Component")
	TObjectPtr<class UGroundIndicatorComponent> CollisionIndicatorComp;

	/** 시야(Fog of War) 판정을 받기 위한 센서 컴포넌트 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "ProjectER|Vision")
	TObjectPtr<UVision_VisualComp> VisionVisualComp;

protected:

private:
	/** 비주얼/물리 설정값이 담긴 GEC 객체 */
	TWeakObjectPtr<const UObject> CachedSourceObject;

	/** 같은 인스턴스에서 HandleSummonedVfx의 중복 호출(OnExecute + WhileActive)을 방지합니다. */
	bool bIsAlreadyInitialized = false;

	/** 결합(Handshake)된 논리적 타겟 액터를 가리키는 약한 포인터 */
	TWeakObjectPtr<AActor> CachedTargetActor;
};
