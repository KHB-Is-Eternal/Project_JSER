#include "Monster/MonsterRangeComponent.h"

#include "Net/UnrealNetwork.h"
#include "Components/SphereComponent.h"
#include "Components/StateTreeComponent.h"
#include "CharacterSystem/Character/BaseCharacter.h"
#include "Monster/BaseMonster.h"

UMonsterRangeComponent::UMonsterRangeComponent()
{
	PrimaryComponentTick.bCanEverTick = false;

}

void UMonsterRangeComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(UMonsterRangeComponent, PlayerCount);
}

void UMonsterRangeComponent::BeginPlay()
{
	Super::BeginPlay();

	AActor* OwnerActor = GetOwner();
	if (IsValid(OwnerActor) == false)
	{
		UE_LOG(LogTemp, Warning, TEXT("UMonsterRangeComponent::BeginPlay : Not OwnerActor"));
		return;
	}
	if (OwnerActor->HasAuthority() == false)
	{
		return;
	}

	FCollisionResponseTemplate Template;
	if (UCollisionProfile::Get()->GetProfileTemplate("PlayerCounter", Template) == false)
	{
		UE_LOG(LogTemp, Warning, TEXT("FSTT_SetCollisionProfile::EnterState : Not CollisionPlayerCounter"));
		return;
	}

	// 서버에서만 생성하여 체크
	RangeSphere = NewObject<USphereComponent>(this);
	if (IsValid(RangeSphere) == false)
	{
		UE_LOG(LogTemp, Warning, TEXT("UMonsterRangeComponent::BeginPlay : Not RangeSphere"));
		return;
	}
    RangeSphere->InitSphereRadius(PlayerCountSphereRadius);
    RangeSphere->SetCollisionProfileName(TEXT("PlayerCounter"));
    RangeSphere->SetGenerateOverlapEvents(true);

	RangeSphere->OnComponentBeginOverlap.AddDynamic(
		this, &UMonsterRangeComponent::OnPlayerCountingBeginOverlap);
	RangeSphere->OnComponentEndOverlap.AddDynamic(
		this, &UMonsterRangeComponent::OnPlayerCountingEndOverlap);

	RangeSphere->RegisterComponent();
	RangeSphere->SetWorldLocation(OwnerActor->GetActorLocation());


	// 서버에서만 생성하여 체크
	OutSphere = NewObject<USphereComponent>(this);
	if (IsValid(OutSphere) == false)
	{
		UE_LOG(LogTemp, Warning, TEXT("UMonsterRangeComponent::BeginPlay : Not OutSphere"));
		return;
	}
	OutSphere->InitSphereRadius(PlayerOutSphereRadius);
	OutSphere->SetCollisionProfileName(TEXT("PlayerCounter"));
	OutSphere->SetGenerateOverlapEvents(true);

	OutSphere->OnComponentEndOverlap.AddDynamic(
		this, &UMonsterRangeComponent::OnPlayerOutEndOverlap);

	OutSphere->RegisterComponent();
	OutSphere->SetWorldLocation(OwnerActor->GetActorLocation());
}

void UMonsterRangeComponent::SetPlayerCount(int32 Amount)
{
	PlayerCount = Amount;
}

int32 UMonsterRangeComponent::GetPlayerCount()
{
	return PlayerCount;
}

//GameManager에서 스폰하고 실행
void UMonsterRangeComponent::InitMonsterGroup()
{
	AActor* OwnerActor = GetOwner();
	if (IsValid(OwnerActor) == false)
	{
		UE_LOG(LogTemp, Warning, TEXT("UMonsterRangeComponent::InitMonsterGroup : Not OwnerActor"));
		return;
	}
	if (OwnerActor->HasAuthority() == false)
	{
		return;
	}

	ABaseMonster* OwnerMonster = Cast<ABaseMonster>(GetOwner());
	if (IsValid(OwnerMonster) == false)
	{
		UE_LOG(LogTemp, Warning, TEXT("UMonsterRangeComponent::InitMonsterGroup : Not OwnerMonster"));
		return;
	}
	FPrimaryAssetId MyId = OwnerMonster->GetMonsterId();

	if (MyId.IsValid() == false)
	{
		UE_LOG(LogTemp, Warning, TEXT("UMonsterRangeComponent::InitMonsterGroup : Not MonsterId"));
		return;
	}
	if (IsValid(RangeSphere) == false)
	{
		UE_LOG(LogTemp, Warning, TEXT("UMonsterRangeComponent::InitMonsterGroup : Not RangeSphere"));
		return;
	}

	TArray<AActor*> GroupActors;
	RangeSphere->GetOverlappingActors(GroupActors, AActor::StaticClass());
	
	for (AActor* Actor : GroupActors)
	{
		ABaseMonster* Monster = Cast<ABaseMonster>(Actor);
		if (IsValid(Monster) == false)
		{
			continue;
		}
		if (Monster->GetMonsterId() != MyId)
		{
			continue;
		}
		if (Monster->GetbIsDead() == true)
		{
			continue;
		}
		
		MonsterGroup.AddUnique(Monster);

		// 따로 스폰된 경우 자기를 다른 그룹에 넣어주려고
		UMonsterRangeComponent* OtherRangeComp = Monster->GetMonsterRangeComp();
		if (IsValid(OtherRangeComp) == false)
		{
			UE_LOG(LogTemp, Warning, TEXT("UMonsterRangeComponent::InitMonsterGroup : Not OtherRangeComp"));
			continue;
		}
		OtherRangeComp->GetMonsterGroup().AddUnique(OwnerMonster);
	}
}


TArray<TObjectPtr<ABaseMonster>>& UMonsterRangeComponent::GetMonsterGroup()
{
	return MonsterGroup;
}
void UMonsterRangeComponent::OnPlayerCountingBeginOverlap(UPrimitiveComponent* OverlappedComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	if (IsValid(OtherActor) == false)
	{
		UE_LOG(LogTemp, Warning, TEXT("UMonsterRangeComponent::OnPlayerCountingBeginOverlap : Not OtherActor"));
		return;
	}
	if (OtherActor->IsA<ABaseCharacter>() == false)
	{
		//UE_LOG(LogTemp, Warning, TEXT("UMonsterRangeComponent::OnPlayerCountingBeginOverlap : Not ABaseCharacter"));
		return;
	}

	PlayerCount = FMath::Max(0, PlayerCount + 1);
	if (PlayerCount == 1)
	{
		OnPlayerCountOne.Broadcast();	
	}
}

void UMonsterRangeComponent::OnPlayerCountingEndOverlap(UPrimitiveComponent* OverlappedComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex)
{
	if (IsValid(OtherActor) == false)
	{
		UE_LOG(LogTemp, Warning, TEXT("UMonsterRangeComponent::OnPlayerCountingEndOverlap : Not OtherActor"));
		return;
	}
	if (OtherActor->IsA<ABaseCharacter>() == false)
	{
		//UE_LOG(LogTemp, Warning, TEXT("UMonsterRangeComponent::OnPlayerCountingEndOverlap : Not ABaseCharacter"));
		return;
	}

	PlayerCount = FMath::Max(0, PlayerCount - 1);
	if (PlayerCount == 0)
	{
		OnPlayerCountZero.Broadcast();
	}
}

void UMonsterRangeComponent::OnPlayerOutEndOverlap(UPrimitiveComponent* OverlappedComp, AActor* OtherActor,
	UPrimitiveComponent* OtherComp, int32 OtherBodyIndex)
{
	if (IsValid(OtherActor) == false)
	{
		UE_LOG(LogTemp, Warning, TEXT("UMonsterRangeComponent::OnPlayerOutEndOverlap : Not OtherActor"));
		return;
	}

	AActor* OwnerActor = GetOwner();
	if (IsValid(OwnerActor) == false)
	{
		UE_LOG(LogTemp, Warning, TEXT("UMonsterRangeComponent::OnPlayerOutEndOverlap : Not OwnerActor"));
		return;
	}
	ABaseMonster* OwnerMonster = Cast<ABaseMonster>(OwnerActor);
	if (IsValid(OwnerMonster) == false)
	{
		UE_LOG(LogTemp, Warning, TEXT("UMonsterRangeComponent::OnPlayerOutEndOverlap : Not OwnerMonster"));
		return;
	}

	if (OtherActor->IsA<ABaseCharacter>())
	{
		AActor* Target = OwnerMonster->GetTargetPlayer();
		if (IsValid(Target) == false)
		{
			//UE_LOG(LogTemp, Warning, TEXT("UMonsterRangeComponent::OnPlayerOutEndOverlap : Not Target"));
			return;
		}

		// 타겟이 나갔을 때
		if (Target == OtherActor)
		{
			OnPlayerOut.Broadcast();
		}
	}
	else if (OtherActor->IsA<ABaseMonster>())
	{
		// 자기 자신이 나갔을 때
		if (OwnerActor == OtherActor)
		{
			OnPlayerOut.Broadcast();
		}
	}
}
