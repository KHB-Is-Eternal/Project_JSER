#include "ItemSystem/Actor/BaseWardActor.h"
#include "Components/StaticMeshComponent.h"
#include "Components/SphereComponent.h"
#include "LineOfSight/VisionComps/Vision_EvaluatorComp.h"
#include "LineOfSight/VisionComps/Vision_VisualComp.h"
#include "LineOfSight/LOSVisual/VisibilityMeshComp.h"
#include "AbilitySystemComponent.h"
#include "CharacterSystem/GAS/ProjectERASC.h"
#include "ItemSystem/GAS/WardAttributeSet.h"
#include "ItemSystem/UI/UI_WardHPBar.h"
#include "GlobalUtil/StaticGlobalUtils.h"
#include "Components/WidgetComponent.h"
#include "Blueprint/UserWidget.h"
#include "Engine/Engine.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/Pawn.h"
#include "Net/UnrealNetwork.h"

ABaseWardActor::ABaseWardActor()
{
	PrimaryActorTick.bCanEverTick = false;
	bReplicates = true;

	WardTeamChannel = 255; // EVisionChannel::None (255)로 초기화하여, 0(TeamA)이나 1(TeamB)로 변경 시 클라이언트에서 무조건 OnRep이 발생하도록 강제함
	MaxHealth = 4; // 적 평타 4회 피격 시 파괴
	CurrentHealth = MaxHealth;
	WardLifeSpan = 60.f;
	VisionRadius = 800.f;

	WardMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("WardMesh"));
	RootComponent = WardMesh;
	WardMesh->SetCollisionProfileName(TEXT("BlockAllDynamic"));
	WardMesh->SetCollisionResponseToChannel(ECC_GameTraceChannel3, ECR_Block); // VisionSensor (비전 센서 감지)
	WardMesh->SetCollisionResponseToChannel(ECC_GameTraceChannel5, ECR_Block); // CursorTrace (적 평타 커서 타겟 선택)
	WardMesh->SetCollisionResponseToChannel(ECC_Pawn, ECR_Ignore); // 캐릭터가 와드를 관통해 지나가도록 물리 블록 해제
	WardMesh->SetGenerateOverlapEvents(true); // 오버랩 이벤트는 쌍방 모두 true여야 발생
	WardMesh->ComponentTags.Add(TEXT("VisibilityMesh"));
	WardMesh->ComponentTags.Add(TEXT("VisionTarget"));
	WardMesh->SetHiddenInSceneCapture(true);

	HitCollision = CreateDefaultSubobject<USphereComponent>(TEXT("HitCollision"));
	HitCollision->SetupAttachment(RootComponent);
	HitCollision->SetSphereRadius(50.f);
	HitCollision->SetCollisionProfileName(TEXT("BlockAllDynamic"));
	HitCollision->SetCollisionResponseToChannel(ECC_GameTraceChannel3, ECR_Block); // VisionSensor (비전 센서 감지)
	HitCollision->SetCollisionResponseToChannel(ECC_GameTraceChannel5, ECR_Block); // CursorTrace (적 평타 커서 타겟 선택)
	HitCollision->SetCollisionResponseToChannel(ECC_Pawn, ECR_Ignore); // 캐릭터가 와드를 관통해 지나가도록 물리 블록 해제
	HitCollision->SetGenerateOverlapEvents(true); // 오버랩 이벤트는 쌍방 모두 true여야 발생
	HitCollision->ComponentTags.Add(TEXT("VisionTarget"));
	HitCollision->SetHiddenInSceneCapture(true);

	VisionEvaluatorComp = CreateDefaultSubobject<UVision_EvaluatorComp>(TEXT("VisionEvaluatorComp"));
	VisionVisualComp = CreateDefaultSubobject<UVision_VisualComp>(TEXT("VisionVisualComp"));

	// GAS: 평타 GE 수신용 ASC + 전용 어트리뷰트셋 직접 소유 (몬스터 패턴, Minimal 복제)
	ASC = CreateDefaultSubobject<UProjectERASC>(TEXT("ASC"));
	ASC->SetIsReplicated(true);
	ASC->SetReplicationMode(EGameplayEffectReplicationMode::Minimal);
	WardAttributes = CreateDefaultSubobject<UWardAttributeSet>(TEXT("WardAttributes"));

	// 머리 위 HP 바 위젯 (화면 정렬, 4칸 분절). 실제 위젯 클래스는 BP에서 지정.
	HPBarWidget = CreateDefaultSubobject<UWidgetComponent>(TEXT("HPBarWidget"));
	HPBarWidget->SetupAttachment(RootComponent);
	HPBarWidget->SetRelativeLocation(FVector(0.f, 0.f, 120.f));
	HPBarWidget->SetWidgetSpace(EWidgetSpace::Screen);
	HPBarWidget->SetDrawSize(FVector2D(120.f, 16.f));
	HPBarWidget->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	HPBarWidget->SetHiddenInSceneCapture(true);

	// (제거됨) 풀 시스템 바인딩 시도했던 유효하지 않은 함수 연결 제거

	// 시야 시스템에서 타겟으로 감지될 수 있도록 태그 추가
	Tags.Add(TEXT("VisionTarget"));
}

void ABaseWardActor::BeginPlay()
{
	Super::BeginPlay();

	CurrentHealth = MaxHealth;

	// GAS 액터 정보 초기화(평타 GE 수신) + 평타 피격 구독
	if (ASC)
	{
		ASC->InitAbilityActorInfo(this, this);
	}
	if (WardAttributes)
	{
		WardAttributes->OnWardAutoAttackHit.AddUObject(this, &ABaseWardActor::HandleAutoAttackHit);
	}

	// 캐릭터 BP와 동일한 패턴 — 시야 이벤트로 메시 가시성 토글.
	// 표시(Revealed)는 즉시, 숨김은 페이드 완료(HideComplete) 후에 꺼서 페이드와 공존한다.
	if (VisionVisualComp)
	{
		VisionVisualComp->OnTargetRevealed.AddUniqueDynamic(this, &ABaseWardActor::HandleWardRevealed);
		VisionVisualComp->OnTargetHideComplete.AddUniqueDynamic(this, &ABaseWardActor::HandleWardHidden);
	}

	// HP 바 위젯 초기화 후 1회 갱신 (로컬 클라이언트 전용 — 서버/데디는 위젯 없음)
	if (HPBarWidget)
	{
		if (HPBarWidgetClass)
		{
			HPBarWidget->SetWidgetClass(HPBarWidgetClass);
		}
		HPBarWidget->InitWidget();
	}
	RefreshHPBar();

	// 수명은 엔진 SetLifeSpan으로 처리. 복제 액터의 파괴는 서버가 주도하므로 서버 권한에서만 설정.
	if (HasAuthority() && WardLifeSpan > 0.f)
	{
		SetLifeSpan(WardLifeSpan);
	}
}

void ABaseWardActor::InitializeWardTeam(ETeamType InTeamType)
{
	WardTeamType = InTeamType;
	WardTeamChannel = static_cast<uint8>(UStaticGlobalUtils::ConvertTeamToVisionChannel(InTeamType));
	ApplyWardTeamChannel();
}

void ABaseWardActor::HandleWardRevealed()
{
	if (WardMesh)
	{
		WardMesh->SetVisibility(true);
	}
	if (HPBarWidget)
	{
		HPBarWidget->SetVisibility(true);
	}
	// 로컬에 표시되는 시점 — 색/칸 갱신(이때는 로컬 폰 팀이 확정돼 있음)
	RefreshHPBar();
}

void ABaseWardActor::HandleWardHidden()
{
	if (WardMesh)
	{
		WardMesh->SetVisibility(false);
	}
	if (HPBarWidget)
	{
		HPBarWidget->SetVisibility(false);
	}
}

void ABaseWardActor::OnRep_WardTeamChannel()
{
	// 클라이언트 측에서 팀 정보가 동기화되면 초기화를 수행합니다.
	ApplyWardTeamChannel();
}

void ABaseWardActor::ApplyWardTeamChannel()
{
	if (VisionVisualComp)
	{
		if (UVisibilityMeshComp* VisMeshComp = VisionVisualComp->GetVisibilityMeshComp())
		{
			// MeshKey 지정 시 내부에서 FindMeshesByTag()까지 수행.
			// LOS Resource Pool 세팅에 "Ward" 키가 등록되어 있으면 풀 MID 사용, 없으면 소유 모드 폴백.
			VisMeshComp->SetMeshKey(TEXT("Ward"));
		}

		// 와드의 시야 반경과 팀을 설정하고 서브시스템에 등록(Initialize)합니다.
		VisionVisualComp->SetVisionRange(VisionRadius);
		VisionVisualComp->SetVisionChannel(static_cast<EVisionChannel>(WardTeamChannel));
		VisionVisualComp->Initialize();
	}

	if (VisionEvaluatorComp && VisionVisualComp)
	{
		// 두 컴포넌트를 연결하고, 감지 반경을 시야 반경(Visual)과 동일하게 동기화시킵니다.
		VisionEvaluatorComp->InitializeEvaluator(VisionVisualComp);
		VisionEvaluatorComp->SyncDetectionRadius();

		// 자기 자신의 팀을 기준으로 안개 걷어낼 대상을 평가하도록 초기화
		VisionEvaluatorComp->InitializeIfSameTeam();
	}

	// 팀이 확정됐으니 HP 바 색상(아군/적) 갱신
	RefreshHPBar();
}

void ABaseWardActor::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(ABaseWardActor, WardTeamChannel);
	DOREPLIFETIME(ABaseWardActor, CurrentHealth);
}

void ABaseWardActor::OnRep_CurrentHealth()
{
	// 클라이언트: 남은 히트가 복제되면 HP 바 갱신
	RefreshHPBar();
}

void ABaseWardActor::RefreshHPBar()
{
	if (!HPBarWidget)
	{
		return;
	}

	if (UUI_WardHPBar* Bar = Cast<UUI_WardHPBar>(HPBarWidget->GetUserWidgetObject()))
	{
		Bar->UpdateBar(CurrentHealth, MaxHealth, IsAllyOfLocalPlayer());
	}
}

bool ABaseWardActor::IsAllyOfLocalPlayer() const
{
	if (!GEngine)
	{
		return false;
	}

	// 리슨서버 대응: 로컬 플레이어 컨트롤러를 명시적으로 조회
	APlayerController* LocalPC = GEngine->GetFirstLocalPlayerController(GetWorld());
	if (!LocalPC)
	{
		return false;
	}

	if (const APawn* LocalPawn = LocalPC->GetPawn())
	{
		if (const ITargetableInterface* LocalTarget = Cast<ITargetableInterface>(LocalPawn))
		{
			return LocalTarget->GetTeamType() == WardTeamType;
		}
	}
	return false;
}

UAbilitySystemComponent* ABaseWardActor::GetAbilitySystemComponent() const
{
	return ASC;
}

ETeamType ABaseWardActor::GetTeamType() const
{
	return WardTeamType;
}

bool ABaseWardActor::IsTargetable() const
{
	// 생존(체력 남음) 중일 때만 타겟 가능
	return CurrentHealth > 0;
}

void ABaseWardActor::HighlightActor(bool bIsHighlight, int32 StencilValue)
{
	// TODO: 필요 시 포스트프로세스 하이라이트. 현재 미사용.
}

void ABaseWardActor::HandleAutoAttackHit()
{
	// 데미지 적용(GE)은 서버 권한에서 실행되므로 여기도 서버 전용
	if (!HasAuthority())
	{
		return;
	}

	CurrentHealth = FMath::Max(0, CurrentHealth - 1);
	UE_LOG(LogTemp, Log, TEXT("[BaseWardActor] Auto-attack hit. Remaining health: %d / %d"), CurrentHealth, MaxHealth);

	// 서버(리슨 호스트 포함) 즉시 갱신. 원격 클라는 CurrentHealth OnRep으로 갱신됨.
	RefreshHPBar();

	if (CurrentHealth <= 0)
	{
		DestroyWard();
	}
}

uint8 ABaseWardActor::GetVisionTeam() const
{
	return WardTeamChannel;
}

FVector ABaseWardActor::GetVisionOrigin() const
{
	return GetActorLocation();
}

float ABaseWardActor::GetVisionRadius() const
{
	return VisionRadius;
}

void ABaseWardActor::GetVisibleActors(TArray<AActor*>& OutActors) const
{
	// 보통 플러그인 내부 캐시를 사용하므로 여기서는 빈 상태로 반환
}

void ABaseWardActor::LifeSpanExpired()
{
	// 엔진 수명 만료 시에도 공통 파괴 경로(DestroyWard)로 통일
	DestroyWard();
}

void ABaseWardActor::DestroyWard()
{
	// TODO: 파괴 시 이펙트나 사운드 재생 가능
	Destroy();
}
