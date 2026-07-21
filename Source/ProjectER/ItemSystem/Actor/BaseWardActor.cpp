#include "ItemSystem/Actor/BaseWardActor.h"
#include "Components/StaticMeshComponent.h"
#include "Components/SphereComponent.h"
#include "LineOfSight/VisionComps/Vision_EvaluatorComp.h"
#include "LineOfSight/VisionComps/Vision_VisualComp.h"
#include "TimerManager.h"
#include "Net/UnrealNetwork.h"

ABaseWardActor::ABaseWardActor()
{
	PrimaryActorTick.bCanEverTick = false;
	bReplicates = true;

	WardTeamChannel = 255; // EVisionChannel::None (255)로 초기화하여, 0(TeamA)이나 1(TeamB)로 변경 시 클라이언트에서 무조건 OnRep이 발생하도록 강제함
	MaxHealth = 3;
	CurrentHealth = MaxHealth;
	WardLifeSpan = 60.f;
	VisionRadius = 800.f;

	WardMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("WardMesh"));
	RootComponent = WardMesh;
	WardMesh->SetCollisionProfileName(TEXT("BlockAllDynamic"));

	HitCollision = CreateDefaultSubobject<USphereComponent>(TEXT("HitCollision"));
	HitCollision->SetupAttachment(RootComponent);
	HitCollision->SetSphereRadius(50.f);
	HitCollision->SetCollisionProfileName(TEXT("BlockAllDynamic"));

	VisionEvaluatorComp = CreateDefaultSubobject<UVision_EvaluatorComp>(TEXT("VisionEvaluatorComp"));
	VisionVisualComp = CreateDefaultSubobject<UVision_VisualComp>(TEXT("VisionVisualComp"));

	// (제거됨) 풀 시스템 바인딩 시도했던 유효하지 않은 함수 연결 제거

	// 시야 시스템에서 타겟으로 감지될 수 있도록 태그 추가
	Tags.Add(TEXT("VisionTarget"));
}

void ABaseWardActor::BeginPlay()
{
	Super::BeginPlay();

	CurrentHealth = MaxHealth;

	// 수명 타이머 설정
	if (WardLifeSpan > 0.f)
	{
		GetWorldTimerManager().SetTimer(LifeSpanTimerHandle, this, &ABaseWardActor::OnWardExpired, WardLifeSpan, false);
	}
}

void ABaseWardActor::InitializeWardTeam(uint8 InTeamChannel)
{
	WardTeamChannel = InTeamChannel;
	ApplyWardTeamChannel();
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
}

void ABaseWardActor::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(ABaseWardActor, WardTeamChannel);
}

float ABaseWardActor::TakeDamage(float DamageAmount, struct FDamageEvent const& DamageEvent, class AController* EventInstigator, AActor* DamageCauser)
{
	float ActualDamage = Super::TakeDamage(DamageAmount, DamageEvent, EventInstigator, DamageCauser);

	// 와드는 어떤 데미지든 1의 피해만 입음
	if (ActualDamage > 0.f)
	{
		CurrentHealth -= 1;
		if (CurrentHealth <= 0)
		{
			DestroyWard();
		}
	}

	return ActualDamage;
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

void ABaseWardActor::OnWardExpired()
{
	DestroyWard();
}

void ABaseWardActor::DestroyWard()
{
	// TODO: 파괴 시 이펙트나 사운드 재생 가능
	Destroy();
}
