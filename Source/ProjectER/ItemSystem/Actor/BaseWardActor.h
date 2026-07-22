#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "LineOfSight/Management/VisionProviderInterface.h"
#include "BaseWardActor.generated.h"

class UStaticMeshComponent;
class USphereComponent;
class UVision_EvaluatorComp;
class UVision_VisualComp;

UCLASS()
class PROJECTER_API ABaseWardActor : public AActor, public IVisionProviderInterface
{
	GENERATED_BODY()
	
public:	
	ABaseWardActor();

protected:
	virtual void BeginPlay() override;

public:	
	// 체력 감소 처리용 데미지 수신 함수
	virtual float TakeDamage(float DamageAmount, struct FDamageEvent const& DamageEvent, class AController* EventInstigator, AActor* DamageCauser) override;

	// 팀 정보 초기화 (설치 시 호출)
	UFUNCTION(BlueprintCallable, Category = "Ward")
	void InitializeWardTeam(uint8 InTeamChannel);

	// IVisionProviderInterface 구현
	virtual uint8 GetVisionTeam() const override;
	virtual FVector GetVisionOrigin() const override;
	virtual float GetVisionRadius() const override;
	virtual void GetVisibleActors(TArray<AActor*>& OutActors) const override;

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Ward|Components")
	UStaticMeshComponent* WardMesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Ward|Components")
	USphereComponent* HitCollision;

	// 시야를 제공하는 컴포넌트
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Ward|Components")
	UVision_EvaluatorComp* VisionEvaluatorComp;

	// 안개에 의해 시각적으로 가려지는 컴포넌트 (적팀에게 안 보이도록)
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Ward|Components")
	UVision_VisualComp* VisionVisualComp;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ward|Stats")
	int32 MaxHealth;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Ward|Stats")
	int32 CurrentHealth;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ward|Stats")
	float WardLifeSpan;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ward|Stats")
	float VisionRadius;

	// 와드의 팀 채널 저장 (클라이언트 동기화를 위해 ReplicatedUsing 사용)
	UPROPERTY(ReplicatedUsing = OnRep_WardTeamChannel)
	uint8 WardTeamChannel;

	UFUNCTION()
	void OnRep_WardTeamChannel();

	// 시야 이벤트 구독 핸들러 — 캐릭터 BP와 동일한 패턴.
	// 머티리얼에 VisibilityAlpha 파라미터가 없어도 메시 가시성으로 숨김/표시를 보장한다.
	UFUNCTION()
	void HandleWardRevealed();

	UFUNCTION()
	void HandleWardHidden();

	// WardTeamChannel 기준으로 시야 컴포넌트 초기화 (서버 InitializeWardTeam / 클라 OnRep 공통 경로)
	void ApplyWardTeamChannel();

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

private:
	// 타이머 핸들러
	FTimerHandle LifeSpanTimerHandle;

	UFUNCTION()
	void OnWardExpired();

	void DestroyWard();
};
