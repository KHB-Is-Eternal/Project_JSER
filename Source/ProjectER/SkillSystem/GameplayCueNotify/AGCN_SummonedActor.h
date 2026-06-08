#pragma once

#include "CoreMinimal.h"
#include "GameplayCueNotify_Actor.h"
#include "AGCN_SummonedActor.generated.h"

class UNiagaraComponent;
class UAudioComponent;
class UProjectileMovementComponent;
class USkillNiagaraSpawnConfig;
class USkillSoundSpawnConfig;
class UStaticMeshComponent;
class UShapeComponent;

/**
 * 소환물 비주얼을 담당하며 예측 키를 통해 판정 액터와 동기화되는 GCN 액터
 */
UCLASS()
class PROJECTER_API AGCN_SummonedActor : public AGameplayCueNotify_Actor
{
	GENERATED_BODY()

public:
	AGCN_SummonedActor();

protected:
	virtual bool OnExecute_Implementation(AActor* MyTarget, const FGameplayCueParameters& Parameters) override;
	virtual bool WhileActive_Implementation(AActor* MyTarget, const FGameplayCueParameters& Parameters) override;
	virtual bool OnRemove_Implementation(AActor* MyTarget, const FGameplayCueParameters& Parameters) override;

	/** 공통 등록 및 초기화 로직 */
	void HandleSummonedVfx(const FGameplayCueParameters& Parameters);

	/** GEC 데이터로부터 속성 초기화 */
	void InitializeFromGEC(const UObject* SourceObject);

	/** Parameters와 Context로부터 실제 시전자를 찾아 반환합니다. */
	AActor* GetActualInstigator(const FGameplayCueParameters& Parameters) const;

protected:
	/** 나이아가라 컴포넌트 초기화 및 재생 */
	void SetupVfxComponent(const USkillNiagaraSpawnConfig* NiagaraConfig);

	/** 오디오 컴포넌트 초기화 및 재생 */
	void SetupSfxComponent(const USkillSoundSpawnConfig* SoundConfig);


public:
	/** 비주얼을 담당하는 나이아가라 컴포넌트 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "ProjectER | Visual")
	TObjectPtr<UNiagaraComponent> VfxComponent;

	/** 사운드를 담당하는 오디오 컴포넌트 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "ProjectER | Audio")
	TObjectPtr<UAudioComponent> SfxComponent;

	/** 예측 이동을 담당하는 컴포넌트 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "ProjectER | Movement")
	TObjectPtr<UProjectileMovementComponent> MovementComponent;

	/** 콜리전 영역의 시각적 테두리(아웃라인)를 그려주는 메쉬 컴포넌트 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "ProjectER | Visual")
	TObjectPtr<UStaticMeshComponent> CollisionOutlineMesh;

public:
	/** 캐싱된 GEC 데이터를 반환합니다. */
	const UObject* GetSourceObject() const { return CachedSourceObject.Get(); }

	/** 콜리전 메쉬의 아웃라인을 아군/적군 여부에 따라 설정합니다. */
	UFUNCTION(BlueprintCallable, Category = "ProjectER|GameplayCue")
	void SetupCollisionOutline(UShapeComponent* InCollisionComponent, AActor* InInstigatorActor);

private:
	/** 비주얼/물리 설정값이 담긴 GEC 객체 */
	TWeakObjectPtr<UObject> CachedSourceObject;

	/** 같은 인스턴스에서 HandleSummonedVfx의 중복 호출(OnExecute + WhileActive)을 방지합니다. */
	bool bIsAlreadyInitialized = false;

public:
	/** 대상 판정 액터(장판 등)에 자신을 부착하고 비주얼/사운드 설정 및 생명주기를 연동합니다. */
	UFUNCTION(BlueprintCallable, Category = "ProjectER|GameplayCue")
	void AttachToTargetActor(AActor* InTargetActor);

	/** 타겟 액터가 파괴될 때 호출되어 자신도 파괴합니다. */
	UFUNCTION()
	void OnTargetActorDestroyed(AActor* DestroyedActor);

	/** 나이아가라 컴포넌트를 외부에서 부착할 수 있도록 반환합니다. */
	UFUNCTION(BlueprintCallable, Category = "ProjectER|GameplayCue")
	UNiagaraComponent* GetVfxComponent() const { return VfxComponent; }

	/** 오디오 컴포넌트를 외부에서 부착할 수 있도록 반환합니다. */
	UFUNCTION(BlueprintCallable, Category = "ProjectER|GameplayCue")
	UAudioComponent* GetSfxComponent() const { return SfxComponent; }
};
