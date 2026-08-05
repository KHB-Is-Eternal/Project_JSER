#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "AbilitySystemInterface.h"
#include "CharacterSystem/Interface/TargetableInterface.h"
#include "LineOfSight/Management/VisionProviderInterface.h"
#include "BaseWardActor.generated.h"

class UStaticMeshComponent;
class USphereComponent;
class UVision_EvaluatorComp;
class UVision_VisualComp;
class UProjectERASC;
class UWardAttributeSet;
class UAbilitySystemComponent;
class UWidgetComponent;
class UUserWidget;

UCLASS()
class PROJECTER_API ABaseWardActor : public AActor, public IVisionProviderInterface, public IAbilitySystemInterface, public ITargetableInterface
{
	GENERATED_BODY()
	
public:	
	ABaseWardActor();

protected:
	virtual void BeginPlay() override;

	// 엔진 수명(SetLifeSpan) 만료 시 호출 — 공통 파괴 경로로 라우팅
	virtual void LifeSpanExpired() override;

public:
	// 팀 정보 초기화 (설치 시 호출). 팀에서 시야 채널을 파생한다.
	UFUNCTION(BlueprintCallable, Category = "Ward")
	void InitializeWardTeam(ETeamType InTeamType);

	// [김현수 추가분] 지면(ch9)을 트레이스해 메시 바닥이 땅에 닿도록 Z 보정 (설치 시 서버에서 호출).
	// 피벗이 밑면이 아니어도 메시 바운드 바닥을 지면에 맞춘다.
	UFUNCTION(BlueprintCallable, Category = "Ward")
	void SnapToGround();

	// IAbilitySystemInterface 구현 (평타 GE 수신용 ASC 제공)
	virtual UAbilitySystemComponent* GetAbilitySystemComponent() const override;

	// ITargetableInterface 구현 (적 평타 대상 판정용)
	virtual ETeamType GetTeamType() const override;
	virtual bool IsTargetable() const override;
	virtual void HighlightActor(bool bIsHighlight, int32 StencilValue = 0) override;

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

	UPROPERTY(ReplicatedUsing = OnRep_CurrentHealth, VisibleAnywhere, BlueprintReadOnly, Category = "Ward|Stats")
	int32 CurrentHealth;

	UFUNCTION()
	void OnRep_CurrentHealth();

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ward|Stats")
	float WardLifeSpan;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ward|Stats")
	float VisionRadius;

	// [김현수 추가분] 지면에 스냅할 때 얹을 Z 오프셋. 메시 피벗이 밑면이 아니면 이 값으로 보정한다.
	// (이상적으로는 메시 에디터에서 피벗을 밑면으로 옮기는 것이 정석. 그 전까진 이 값으로 조정.)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ward|Placement")
	float GroundZOffset = 0.f;

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

	// GAS: 평타 GE 수신용 ASC + 전용 어트리뷰트셋 (직접 소유, 몬스터 패턴)
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Ward|GAS")
	TObjectPtr<UProjectERASC> ASC;

	UPROPERTY()
	TObjectPtr<UWardAttributeSet> WardAttributes;

	// 게임플레이 팀 (ITargetableInterface). 설치 시 저장.
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Ward|Team")
	ETeamType WardTeamType = ETeamType::None;

	// 머리 위 HP 바 위젯 (4칸 분절). 아군=연두 / 적=붉은. 양 팀 모두에게 표시.
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Ward|UI")
	TObjectPtr<UWidgetComponent> HPBarWidget;

	// 표시할 위젯 클래스 (WBP_WardHPBar 지정). BP에서 세팅.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Ward|UI")
	TSubclassOf<UUserWidget> HPBarWidgetClass;

	// HP 바 갱신: 남은 칸 + 로컬 뷰어 기준 아군/적 색상
	void RefreshHPBar();

	// 로컬 플레이어 기준 이 와드가 아군인지
	bool IsAllyOfLocalPlayer() const;

	// 평타 피격 1회 처리 (서버 전용): 남은 체력 감소 및 0이면 파괴
	void HandleAutoAttackHit();

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

private:
	void DestroyWard();
};
