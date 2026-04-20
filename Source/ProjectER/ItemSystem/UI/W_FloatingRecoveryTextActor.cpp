#include "ItemSystem/UI/W_FloatingRecoveryTextActor.h"
#include "ItemSystem/UI/W_FloatingRecoveryText.h"

#include "Components/SceneComponent.h"
#include "Components/WidgetComponent.h"

AW_FloatingRecoveryTextActor::AW_FloatingRecoveryTextActor()
{
	PrimaryActorTick.bCanEverTick = true;
	bReplicates = false;

	SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
	RootComponent = SceneRoot;

	WidgetComp = CreateDefaultSubobject<UWidgetComponent>(TEXT("WidgetComp"));
	WidgetComp->SetupAttachment(RootComponent);
	WidgetComp->SetWidgetSpace(EWidgetSpace::Screen);
	WidgetComp->SetDrawAtDesiredSize(true);
	WidgetComp->SetCollisionEnabled(ECollisionEnabled::NoCollision);
}

void AW_FloatingRecoveryTextActor::BeginPlay()
{
	Super::BeginPlay();
}

void AW_FloatingRecoveryTextActor::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	ElapsedTime += DeltaSeconds;

	// 위로 이동
	FVector NewLocation = GetActorLocation();
	NewLocation.Z += FloatUpSpeed * DeltaSeconds;
	SetActorLocation(NewLocation);

	// 알파 감소
	const float Alpha = 1.0f - (ElapsedTime / FMath::Max(LifeSeconds, KINDA_SMALL_NUMBER));

	if (WidgetComp)
	{
		if (UW_FloatingRecoveryText* Widget = Cast<UW_FloatingRecoveryText>(WidgetComp->GetUserWidgetObject()))
		{
			Widget->SetRecoveryAlpha(Alpha);
		}
	}

	// 수명 종료
	if (ElapsedTime >= LifeSeconds)
	{
		Destroy();
	}
}

void AW_FloatingRecoveryTextActor::InitRecoveryText(int32 Amount, bool bIsMana)
{
	if (!WidgetComp)
	{
		return;
	}

	WidgetComp->InitWidget();

	UW_FloatingRecoveryText* Widget = Cast<UW_FloatingRecoveryText>(WidgetComp->GetUserWidgetObject());
	if (!Widget)
	{
		return;
	}

	Widget->SetRecoveryText(Amount, bIsMana);
	Widget->SetRecoveryAlpha(1.0f);
}