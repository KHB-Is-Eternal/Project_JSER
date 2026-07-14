#include "ItemSystem/Actor/BaseItemActor.h"
#include "ItemSystem/Data/BaseItemData.h"
#include "ItemSystem/Component/BaseInventoryComponent.h"
#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/WidgetComponent.h"
#include "ItemSystem/UI/ItemNameWidget.h"
#include "Net/UnrealNetwork.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "GameplayTagContainer.h"

ABaseItemActor::ABaseItemActor()
{
	PrimaryActorTick.bCanEverTick = false;

	bReplicates = true;
	SetReplicateMovement(true);

	ItemMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("ItemMesh"));
	RootComponent = ItemMesh;

	InteractionSphere = CreateDefaultSubobject<USphereComponent>(TEXT("InteractionSphere"));
	InteractionSphere->SetupAttachment(RootComponent);
	InteractionSphere->SetSphereRadius(150.f);

	NameTagWidget = CreateDefaultSubobject<UWidgetComponent>(TEXT("NameTagWidget"));
	NameTagWidget->SetupAttachment(RootComponent);
	NameTagWidget->SetRelativeLocation(FVector(0.f, 0.f, 50.f)); // 메시 위쪽 50유닛 지점
	NameTagWidget->SetWidgetSpace(EWidgetSpace::Screen); // 화면 공간 위젯으로 설정
	NameTagWidget->SetDrawAtDesiredSize(true);

	ApplyWorldItemCollisionSettings();
}

void ABaseItemActor::BeginPlay()
{
	Super::BeginPlay();

	RefreshVisualFromItemData();

	if (InteractionSphere)
	{
		InteractionSphere->OnComponentBeginOverlap.AddDynamic(this, &ABaseItemActor::OnOverlapBegin);
	}
}

void ABaseItemActor::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(ABaseItemActor, ItemData);
}

void ABaseItemActor::InitializeFromItemData(UBaseItemData* InItemData, APawn* InDropperPawn)
{
	if (!HasAuthority())
	{
		return;
	}

	ItemData = InItemData;
	LastDropperPawn = InDropperPawn;
	DroppedAtTime = GetWorld() ? GetWorld()->GetTimeSeconds() : 0.f;

	RefreshVisualFromItemData();
	ForceNetUpdate();
}

void ABaseItemActor::OnRep_ItemData()
{
	RefreshVisualFromItemData();
}

void ABaseItemActor::RefreshVisualFromItemData()
{
	if (!ItemMesh)
	{
		return;
	}

	if (ItemData && !ItemData->ItemMesh.IsNull())
	{
		if (UStaticMesh* Mesh = ItemData->ItemMesh.LoadSynchronous())
		{
			ItemMesh->SetStaticMesh(Mesh);

			// 블루프린트에서 위젯 텍스트를 갱신하도록 이벤트 호출 (추가 연출용)
			UpdateNameTagUI();

			// [추가] C++에서 직접 위젯 텍스트 설정 (BP 로직 최소화)
			if (NameTagWidget)
			{
				if (UItemNameWidget* NameWidget = Cast<UItemNameWidget>(NameTagWidget->GetUserWidgetObject()))
				{
					NameWidget->SetItemName(ItemData->ItemName, ItemData->GetRarityTextColor());
				}
			}

			// [중요]
			// 메시 에셋 자체에 박혀 있는 Collision 설정이 다시 살아날 수 있으므로
			// 메시를 갈아끼운 뒤에도 월드 드랍 아이템용 충돌 설정을 다시 강제 적용
			ApplyWorldItemCollisionSettings();
			return;
		}
	}

	ItemMesh->SetStaticMesh(nullptr);
	ApplyWorldItemCollisionSettings();
}

void ABaseItemActor::ApplyWorldItemCollisionSettings()
{
	if (ItemMesh)
	{
		ItemMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		ItemMesh->SetCollisionResponseToAllChannels(ECR_Ignore);
		ItemMesh->SetGenerateOverlapEvents(false);
		ItemMesh->SetSimulatePhysics(false);
		ItemMesh->SetEnableGravity(false);
		ItemMesh->CanCharacterStepUpOn = ECB_No;
		ItemMesh->SetCanEverAffectNavigation(false);
	}

	if (InteractionSphere)
	{
		InteractionSphere->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
		InteractionSphere->SetCollisionObjectType(ECC_WorldDynamic);
		InteractionSphere->SetCollisionResponseToAllChannels(ECR_Ignore);

		// 자동 줍기
		InteractionSphere->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);

		// 기본 마우스 트레이스
		InteractionSphere->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block);

		// 프로젝트에서 실제 사용하는 CursorTrace 채널 번호로 바꿔야 함
		InteractionSphere->SetCollisionResponseToChannel(ECC_GameTraceChannel5, ECR_Block);

		InteractionSphere->SetGenerateOverlapEvents(true);
		InteractionSphere->CanCharacterStepUpOn = ECB_No;
		InteractionSphere->SetCanEverAffectNavigation(false);
	}
}

void ABaseItemActor::OnOverlapBegin(UPrimitiveComponent* OverlappedComp, AActor* OtherActor,
	UPrimitiveComponent* OtherComp, int32 OtherBodyIndex,
	bool bFromSweep, const FHitResult& SweepResult)
{
	if (OtherActor && OtherActor == LastDropperPawn)
	{
		const float Now = GetWorld() ? GetWorld()->GetTimeSeconds() : 0.f;
		if (Now - DroppedAtTime < InitialPickupIgnoreTime)
		{
			return;
		}
	}

	if (OtherActor && OtherActor != this && ItemData)
	{
		if (ItemData->PickupType == EItemPickupType::Automatic)
		{
			APawn* OverlappedPawn = Cast<APawn>(OtherActor);
			if (OverlappedPawn)
			{
				PickupItem(OverlappedPawn);
			}
		}
	}
}

void ABaseItemActor::PickupItem(APawn* InHandler)
{
	if (!HasAuthority())
	{
		return;
	}

	if (!InHandler || !ItemData)
	{
		return;
	}

	UAbilitySystemComponent* ASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(InHandler);
	if (!IsValid(ASC))
	{
		return;
	}

	const bool bIsDown = ASC->HasMatchingGameplayTag(
		FGameplayTag::RequestGameplayTag(FName("State.Life.Down")));
	const bool bIsDead = ASC->HasMatchingGameplayTag(
		FGameplayTag::RequestGameplayTag(FName("State.Life.Death")));

	if (bIsDown || bIsDead)
	{
		return;
	}

	UBaseInventoryComponent* Inventory = InHandler->FindComponentByClass<UBaseInventoryComponent>();
	if (Inventory)
	{
		if (Inventory->AddItem(ItemData))
		{
			UE_LOG(LogTemp, Warning, TEXT("Item Picked Up: %s"), *ItemData->ItemName.ToString());
			Destroy();
		}
	}
}