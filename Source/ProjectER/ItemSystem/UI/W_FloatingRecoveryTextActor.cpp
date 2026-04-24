#include "ItemSystem/UI/W_FloatingRecoveryTextActor.h"
#include "ItemSystem/UI/W_FloatingRecoveryText.h"

#include "Components/SceneComponent.h"
#include "Components/WidgetComponent.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"

AW_FloatingRecoveryTextActor::AW_FloatingRecoveryTextActor()
{
	PrimaryActorTick.bCanEverTick = true;
	bReplicates = false;

	SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
	RootComponent = SceneRoot;

	// 이제 이 컴포넌트는 "BP에서 WidgetClass를 지정하는 용도"로만 사용
	WidgetComp = CreateDefaultSubobject<UWidgetComponent>(TEXT("WidgetComp"));
	WidgetComp->SetupAttachment(RootComponent);
	WidgetComp->SetWidgetSpace(EWidgetSpace::Screen);
	WidgetComp->SetDrawAtDesiredSize(true);
	WidgetComp->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	WidgetComp->SetVisibility(false);
}

void AW_FloatingRecoveryTextActor::BeginPlay()
{
	Super::BeginPlay();

	// 로컬 플레이어 컨트롤러 캐싱
	CachedLocalPC = UGameplayStatics::GetPlayerController(this, 0);
}

void AW_FloatingRecoveryTextActor::InitRecoveryText(int32 Amount, bool bIsMana)
{
	if (!WidgetComp)
	{
		return;
	}

	if (!CachedLocalPC)
	{
		CachedLocalPC = UGameplayStatics::GetPlayerController(this, 0);
	}

	if (!CachedLocalPC || !CachedLocalPC->IsLocalController())
	{
		return;
	}

	// BP_FloatingRecoveryTextActor의 WidgetComp에 지정해둔 WidgetClass를 그대로 재사용
	TSubclassOf<UUserWidget> WidgetClass = WidgetComp->GetWidgetClass();
	if (!WidgetClass)
	{
		return;
	}

	// 이미 있으면 재사용, 없으면 생성
	if (!FloatingWidget)
	{
		FloatingWidget = CreateWidget<UW_FloatingRecoveryText>(CachedLocalPC, WidgetClass);
		if (!FloatingWidget)
		{
			return;
		}

		// 핵심: Viewport 최상단에 올림
		FloatingWidget->AddToViewport(9999);
	}

	FloatingWidget->SetRecoveryText(Amount, bIsMana);
	FloatingWidget->SetRecoveryAlpha(1.0f);

	UpdateWidgetScreenPosition();
}

void AW_FloatingRecoveryTextActor::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	ElapsedTime += DeltaSeconds;

	// 액터 위치를 위로 올려서 텍스트가 떠오르는 느낌 유지
	FVector NewLocation = GetActorLocation();
	NewLocation.Z += FloatUpSpeed * DeltaSeconds;
	SetActorLocation(NewLocation);

	// 화면 좌표 갱신
	UpdateWidgetScreenPosition();

	// 알파 감소
	const float Alpha = 1.0f - (ElapsedTime / FMath::Max(LifeSeconds, KINDA_SMALL_NUMBER));
	if (FloatingWidget)
	{
		FloatingWidget->SetRecoveryAlpha(Alpha);
	}

	// 수명 종료
	if (ElapsedTime >= LifeSeconds)
	{
		Destroy();
	}
}

void AW_FloatingRecoveryTextActor::UpdateWidgetScreenPosition()
{
	if (!FloatingWidget || !CachedLocalPC)
	{
		return;
	}

	FVector2D ScreenPosition;
	const bool bProjected = UGameplayStatics::ProjectWorldToScreen(
		CachedLocalPC,
		GetActorLocation(),
		ScreenPosition,
		true
	);

	if (!bProjected)
	{
		return;
	}

	ScreenPosition.Y += ScreenOffsetY;

	FloatingWidget->SetPositionInViewport(ScreenPosition, true);
}

void AW_FloatingRecoveryTextActor::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (FloatingWidget)
	{
		FloatingWidget->RemoveFromParent();
		FloatingWidget = nullptr;
	}

	Super::EndPlay(EndPlayReason);
}