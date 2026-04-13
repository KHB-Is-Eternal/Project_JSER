#include "ItemSystem/UI/W_FloatingRecoveryTextActor.h"
#include "ItemSystem/UI/W_FloatingRecoveryText.h"

#include "Components/SceneComponent.h"
#include "Components/WidgetComponent.h"

AW_FloatingRecoveryTextActor::AW_FloatingRecoveryTextActor()
{
	PrimaryActorTick.bCanEverTick = false;
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
	SetLifeSpan(LifeSeconds);
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
}