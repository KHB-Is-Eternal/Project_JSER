#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "AbilitySystemInterface.h"
#include "CharacterSystem/Interface/TargetableInterface.h"
#include "GameplayEffectTypes.h"
#include "LineOfSight/VisionData.h"
#include "BaseCharacter.generated.h"

struct FWeaponVisualData;

class UCameraComponent;
class USpringArmComponent;
class UNiagaraSystem;
class UAbilitySystemComponent;
class UBaseAttributeSet;
class UGameplayEffect;
class UCharacterData;
class UWidgetComponent; // 체력 바 머리 위에 띄우는 용
class UUI_HP_Bar; // 체력 바 머리 위에 띄우는 용
class AW_FloatingRecoveryTextActor; // [김현수 추가분] 체력,마나 회복 아이템 플로팅 텍스트용

class UTopDownCameraComp;//main camera comp

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnDeath);

UCLASS()
class PROJECTER_API ABaseCharacter : public ACharacter,  public IAbilitySystemInterface, public ITargetableInterface
{
	GENERATED_BODY()

public:
	ABaseCharacter();

protected:
	virtual void BeginPlay() override;

	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	virtual void Tick( float DeltaTime ) override;
	
	virtual void PossessedBy(AController* NewController) override;
	
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	
#pragma region Debug
public:
	UPROPERTY(EditAnywhere, Category = "Debug")
	bool bShowDebug = true;
	
#pragma endregion
	
#pragma region Component
protected:
	
	//replacement for camera comp
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Components", meta = (AllowPrivateAccess="true"))
	TObjectPtr<UTopDownCameraComp> TopDownCameraComp=nullptr;
	
	UPROPERTY()
	TWeakObjectPtr<UAbilitySystemComponent> AbilitySystemComponent=nullptr;
	
#pragma endregion

#pragma region Weapon
protected:
	// 무기 장착 (InitVisuals에서 호출)
	void InitWeapons();
	
	// 무기 교체 (아이템 시스템 연동 시 사용)
	void AttachWeapon(const FWeaponVisualData& WeaponData);
	
	// 전체 무기 해제
	void DetachAllWeapons();
	
protected:
	// 현재 장착 중인 무기 메시 컴포넌트
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Weapon")
	TArray<TObjectPtr<UStaticMeshComponent>> WeaponMeshComponents;
#pragma endregion
	
#pragma region TargetableInterface
public:
	// 팀 정보 반환
	virtual ETeamType GetTeamType() const override;

	// 타겟팅 가능 여부 반환
	UFUNCTION(BlueprintCallable, Category = "Targetable")
	virtual bool IsTargetable() const override;
    
	// [인터페이스 구현] 하이라이트 (나중에 포스트 프로세스로 구현)
	virtual void HighlightActor(bool bIsHighlight, int32 StencilValue = 0) override;
	
	UFUNCTION(BlueprintCallable, Server, Reliable)
	void Server_SetTeamID(ETeamType NewTeamID);
	
protected:
	UFUNCTION()
	void OnRep_TeamID();
	

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Team")
	EVisionChannel ConvertTeamToVisionChannel(ETeamType InTeamType);

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Team")
	EVisionChannel GetVisionChannelFromPlayerStateComp();
	
protected:
	// 팀 변수
	UPROPERTY(ReplicatedUsing = OnRep_TeamID, EditAnywhere, BlueprintReadWrite, Category = "Team")
	ETeamType TeamID;
	
#pragma endregion
	
#pragma region AbilitySystemInterface
public:
	virtual UAbilitySystemComponent* GetAbilitySystemComponent() const override;
	
protected:
	virtual void OnRep_PlayerState() override;

	//BP Exposed Function
	UFUNCTION(BlueprintImplementableEvent)
	void OnPlayerStateChosen();

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Ability")
	bool IsLocalPlayerPawn();
	
#pragma endregion
	
#pragma region GAS
public:
	// 레벨업 시 AttributeSet에서 호출
	UFUNCTION(BlueprintCallable, Category = "GAS")
	virtual void HandleLevelUp();
	
	UFUNCTION(BlueprintCallable, Category = "GAS")
	float GetCharacterLevel() const;
	
	UFUNCTION(BlueprintCallable, Category = "GAS")
	float GetAttackRange() const;
	
	// Tag 기반 몽타주 검색 및 반환
	UFUNCTION(BlueprintCallable, Category = "GAS")
	UAnimMontage* GetCharacterMontageByTag(FGameplayTag MontageTag);
	
	// 몽타주 Pre-Load
	void PreloadMontages();
	
	// 스킬(Gameplay Ability) 레벨업 호출
	UFUNCTION(Server, Reliable, BlueprintCallable, Category = "GAS|Skill")
	void Server_UpgradeSkill(FGameplayTag SkillTag);
	
protected:
	// 이동 속도 스탯 변경 감지 델리게이트
	virtual void OnMoveSpeedChanged(const FOnAttributeChangeData& Data);
	
	UFUNCTION()
	void OnRep_HeroData();

	void InitAbilitySystem(); // ASC 초기화
	void InitAttributes(); // AttributeSet 초기화
	void InitVisuals(); // 메시, 애니메이션 로드
	void InitPlayer(); // 플레이어(카메라 등) 로컬 초기화 통합 함수
	
public:
	UPROPERTY(ReplicatedUsing=OnRep_HeroData, EditAnywhere, BlueprintReadWrite, Category = "Data", meta = (ExposeOnSpawn = true))
	TObjectPtr<UCharacterData> HeroData;
	
	// 스탯 초기화 이펙트 클래스
	UPROPERTY(EditDefaultsOnly,Category = "GAS") 
	TSubclassOf<UGameplayEffect> InitStatusEffectClass;
	
	// 패시브 재생(Regen) 이펙트 클래스
	UPROPERTY(EditDefaultsOnly, Category = "GAS|Life")
	TSubclassOf<UGameplayEffect> RegenEffectClass;
	
	// 기본 상태(Alive) 이펙트 클래스
	UPROPERTY(EditDefaultsOnly, Category = "GAS|Life")
	TSubclassOf<UGameplayEffect> AliveStateEffectClass;

	// 빈사 상태(Down) 이펙트 클래스
	UPROPERTY(EditDefaultsOnly, Category = "GAS|Life")
	TSubclassOf<UGameplayEffect> DownStateEffectClass;
	
	// 사망 상태(Death) 이펙트 클래스
	UPROPERTY(EditDefaultsOnly, Category = "GAS|Life")
	TSubclassOf<UGameplayEffect> DeathStateEffectClass;
	
	// 이동 상태(Moving) 이펙트 클래스
	UPROPERTY(EditDefaultsOnly, Category = "GAS|State")
	TSubclassOf<UGameplayEffect> MovingStateEffectClass;
	
	// 전민성 추가
	UPROPERTY(EditAnywhere, Category = "GAS")
	TSubclassOf<class UGameplayAbility> OpenAbilityClass;

	UPROPERTY(EditAnywhere, Category = "GAS")
	TSubclassOf<class UGameplayAbility> TeleportAbilityClass;
	
protected:
	// Anim Montage 하드 참조 캐싱 맵
	UPROPERTY(Transient)
	TMap<FGameplayTag, TObjectPtr<UAnimMontage>> CachedMontages;
	
	// GE_Moving 핸들 (추적용)
	FActiveGameplayEffectHandle MovingEffectHandle;
#pragma endregion 

#pragma region MoveToLocation
public:
	/** 클라이언트 → 서버: 목적지만 전달 (서버가 비동기 경로 계산) */
	UFUNCTION(Server, Reliable, WithValidation)
	void Server_MoveToLocation(FVector TargetLocation); 
	
	UFUNCTION(Server, Reliable)
	void Server_StopMove();
	
	/** 클라이언트 진입점 — 서버에 요청 + 임시 직선 이동 시작 */
	void MoveToLocation(FVector TargetLocation);
	void StopMove();
	
protected:
	void UpdatePathFollowing();
	void StopPathFollowing();

	// ─── Phase 2: 비동기 길찾기 ───

	/** 서버 전용 — 비동기 경로 탐색 요청 */
	void RequestAsyncPath(const FVector& Destination);

	/** 비동기 경로 탐색 완료 콜백 (서버에서만 호출됨) */
	void OnAsyncPathFound(uint32 QueryID, ENavigationQueryResult::Type Result, FNavPathSharedPtr NavPath);

	/** 경로 성공 시 공통 적용 로직 (서버) */
	void ApplyPathResult(const TArray<FVector>& InPathPoints, const FVector& Destination);

	/** Moving GE 적용 헬퍼 */
	void ApplyMovingStateEffect();

private:
	// 현재 추적 중인 경로 포인트들
	UPROPERTY()
	TArray<FVector> PathPoints;

	// 현재 목표 웨이포인트 인덱스
	int32 CurrentPathIndex = INDEX_NONE;

	// 도착 판정 임계값 (제곱)
	static constexpr float ArrivalDistanceSq = 2500.f; 
	
	// 갱신 타이머
	float PathfindingTimer = 0.0f;
	
	// ─── 경로 캐싱 (Phase 1) ───
	/** 마지막으로 경로를 계산한 목적지 */
	FVector LastPathDestination = FVector::ZeroVector;
	/** 경로 재계산을 건너뛸 거리 임계값 (cm) */
	static constexpr float PathReuseThreshold = 100.0f;
	/** 캐싱된 경로가 생성된 이후 경과 시간 (초) */
	float TimeSinceLastPathCalc = 0.0f;
	/** 최대 캐싱 유효기간 (초) — 초과 시 강제 재계산 */
	static constexpr float MaxPathCacheAge = 1.0f;

	// ─── Phase 2: 클라 임시 직선 이동 ───
	/** 클라이언트가 서버 응답 전 임시로 향하는 목적지 */
	FVector PendingMoveDestination = FVector::ZeroVector;
	
	/** 임시 직선 이동 중 여부 (서버 경로 도착 전) */
	uint8 bIsStraightLineMoving : 1;

	/** 비동기 경로 요청 진행 중 여부 */
	uint8 bAsyncPathInProgress : 1;

#pragma endregion
	
#pragma region Combat
public:
	// 소켓 위치에서 타겟(혹은 정면) 방향 회전값 반환
	UFUNCTION(BlueprintCallable, Category = "Combat")
	FRotator GetCombatGazeRotation(FName SocketName);
	
	// 소켓의 위치를 반환하는 함수 (편의용)
	UFUNCTION(BlueprintCallable, Category = "Combat")
	FVector GetMuzzleLocation(FName SocketName);
	
	// 타겟 설정 함수
	void SetTarget(AActor* NewTarget);
	
	// 공격 가능 사거리 확인
	void CheckCombatTarget(float DeltaTime);
	
	//  공격 명령 (A키 입력)
	UFUNCTION(Server, Reliable)
	void Server_AttackMoveToLocation(FVector TargetLocation);
	
	// 몽타주 섹션 이름 반환 및 인덱스 관리
	UFUNCTION(BlueprintCallable, Category = "Combat|Combo")
	FName GetNextAutoAttackSectionName();
	
	// 현재 공격 몽타주의 재생 속도 (PlayRate) 반환
	// = 현재APS / 기본APS = 공격속도 증가 비율
	UFUNCTION(BlueprintCallable, Category = "Combat|AttackSpeed")
	float GetAttackPlayRate() const;
	
	// 현재 공격 주기(초) 반환 = 1 / 실제APS
	UFUNCTION(BlueprintCallable, Category = "Combat|AttackSpeed")
	float GetAttackCooldown() const;
	
	// 지정된 몽타주 섹션의 실제 재생 시간(초) 반환
	//UFUNCTION(BlueprintCallable, Category = "Combat|AttackSpeed")
	//float GetCurrentAttackSectionDuration(UAnimMontage* Montage, FName SectionName) const;
	
	// 공격 쿨다운이 지났는지 확인 (공격 가능 여부)
	UFUNCTION(BlueprintCallable, Category = "Combat|AttackSpeed")
	bool CanAttack() const;
	
	// 공격 실행 시각 기록 (GA에서 공격 시작 시 호출)
	UFUNCTION(BlueprintCallable, Category = "Combat|AttackSpeed")
	void MarkAttackExecuted();
	
protected:
	UFUNCTION()
	void OnRep_TargetActor();
	
	UFUNCTION(Server, Reliable)
	void Server_SetTarget(AActor* NewTarget);
	
	// 이동 중 주변 적 검색 함수
	void ScanForEnemiesWhileMoving();
	
protected:
	// 현재 타겟 (적)
	UPROPERTY(ReplicatedUsing = OnRep_TargetActor, VisibleAnywhere, BlueprintReadOnly, Category = "Combat")
	TObjectPtr<AActor> TargetActor;
	
	//  공격 명령 (A키 입력) 상태 확인 플래그
	uint8 bIsAttackMoving : 1;
	
	// 평타 순환용 인덱스 (0, 1, 2)
	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "Combat|Combo")
	int32 AutoAttackIndex = 0;
	
	// 기본 공격속도 (CurveTable Level 1 기준값, InitAttributes에서 자동 캐싱)
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Combat|AttackSpeed")
	float CachedBaseAttackSpeed = 0.625f;
	
	// 마지막 공격 실행 서버 시각 (공격 쿨다운 타이머)
	float LastAttackExecuteTime = -999.f;
	
	// 피격 이펙트 캐싱용 변수
	UPROPERTY(Transient)
	TObjectPtr<UNiagaraSystem> CachedBasicHitVFX;
	
#pragma endregion
	
#pragma region CrowdControl
public:
	// CC 태그 변경 감지 콜백 (ASC 델리게이트 바인딩)
	void OnCCTagChanged(const FGameplayTag Tag, int32 NewCount);

	// 이동 차단 CC가 활성화되어 있는지
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "CC")
	bool IsMovementBlocked() const;

protected:
	// ASC에 CC 태그 감시 델리게이트 등록
	void RegisterCCTagCallbacks();
#pragma endregion


#pragma region DeathRevive
public:
	UFUNCTION(Server, Reliable)
	void Server_Revive(FVector RespawnLocation);
	
	// 사망 처리 호출
	virtual void HandleDeath();
	
	// 부활 처리 호출
	UFUNCTION(BlueprintCallable, Category = "Revive")
	virtual void Revive(FVector RespawnLocation);
	
	// 빈사 상태 진입
	virtual void HandleDown();
	
	UPROPERTY()
	FOnDeath OnDeath;

protected:
	// 사망 동기화
	UFUNCTION(NetMulticast, Reliable)
	void Multicast_Death();
	
	// 부활 상태 동기화
	UFUNCTION(NetMulticast, Reliable)
	void Multicast_Revive(FVector RespawnLocation);
	
	// 빈사 상태 동기화
	UFUNCTION(NetMulticast, Reliable)
	void Multicast_HandleDown();
	
protected:
	
#pragma endregion
	
#pragma region UI
public:
	UFUNCTION()
	void InitUI();

	UFUNCTION()
	void UpdateOverheadUI();

	UFUNCTION()
	void OnHealthChanged();
	UFUNCTION()
	void OnStaminaChanged();
	UFUNCTION()
	void OnLevelChanged();
protected:
	// 미니맵용 씬 캡처 컴포넌트
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "UI|Minimap")
	class USceneCaptureComponent2D* MinimapCaptureComponent;
	
	// 미니맵용 얼굴 아이콘
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "UI|Minimap")
	class UStaticMeshComponent* MinimapIconMesh;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "UI|Minimap")
	class UStaticMeshComponent* MinimapLineMesh;

	// 미니맵용 얼굴 마테리얼
	UPROPERTY()
	TObjectPtr<UMaterialInstanceDynamic> MinimapIconMaterial;

	UPROPERTY()
	TObjectPtr<UMaterialInstanceDynamic> MinimapLineMaterial;

	// HP Bar를 담을 컴포넌트
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "UI|Bar")
	UWidgetComponent* HP_MP_BarWidget;

	UPROPERTY()
	UUI_HP_Bar* HPBarWidgetInstance;

	FTimerHandle UILoadTimerHandle;

	//Added for the Minimap draw interval--> previously drawing per tick
	UFUNCTION()
	void UpdateMinimapCapture();

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "UI|Bar")
	float MinimapUpdateRate=0.1f;
	
	FTimerHandle MinimapCaptureTimerHandle;

public:
	// 팀 구분해서 아이콘 색상 업데이트
	void UpdateMinimapVisuals(FLinearColor n_teamColor);

	// ক্র래프팅 시 머리 위에 띄울 위젯 클래스
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Crafting")
	TSubclassOf<UUserWidget> CraftingWidgetClass;

	// 동적으로 생성된 크래프팅 위젯 컴포넌트
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Crafting")
	class UWidgetComponent* CraftingWidgetComp;

	UFUNCTION(NetMulticast, Reliable)
	void Multicast_ToggleCraftingUI(bool bShow);

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "UI|FloatingText")
	TSubclassOf<AW_FloatingRecoveryTextActor> FloatingRecoveryTextActorClass;

	UFUNCTION(NetMulticast, Unreliable)
	void Multicast_ShowRecoveryText(int32 Amount, bool bIsMana);

protected:
	// 크래프팅 시야 판정용 타이머
	FTimerHandle CraftingUIVisibilityTimer;
	void UpdateCraftingUIVisibility();

#pragma endregion

#pragma region Vision

	UFUNCTION(BlueprintPure, BlueprintCallable, Category = "Vision")//Helper for getting the vision channel from the PlayerStateComp
	EVisionChannel GetVisionChannelFromVisionPlayerStateComp();

	
	
#pragma endregion
	
};
