#include "Monster/BaseMonster.h"

#include "Monster/GAS/AttributeSet/BaseMonsterAttributeSet.h"
#include "Monster/Data/MonsterDataAsset.h"
#include "Monster/Data/BaseMonsterTableRow.h"
#include "Monster/GAS/GE/GE_AddXP.h"
#include "GameModeBase/GameMode/ER_InGameMode.h"
#include "GameModeBase/State/ER_GameState.h"
#include "GameModeBase/State/ER_PlayerState.h"
#include "CharacterSystem/Character/BaseCharacter.h"
#include "ItemSystem/Data/BaseItemData.h"
#include "SkillSystem/SkillDataAsset.h"

#include "Components/StateTreeComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/BoxComponent.h"
#include "Components/WidgetComponent.h"
#include "Monster/MonsterRangeComponent.h"
#include "Components/ProgressBar.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "ItemSystem/Component/LootableComponent.h"
#include "Components/AudioComponent.h"

#include "Net/UnrealNetwork.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "Engine/AssetManager.h"
#include "Engine/StreamableManager.h"

ABaseMonster::ABaseMonster()
	:
	StartLocation(FVector::ZeroVector),
	StartRotator(FRotator::ZeroRotator),
	TargetPlayer(nullptr),
	bIsCombat(false),
	bIsDead(false),
	bIsFirstAttack(false),
	AttackCount(0)
{
	//액터 복제, UPROPERTY(Replicated)멤버 동기화
	SetReplicates(true);
	//위치 / 회전 / 속도 동기화
	SetReplicateMovement(true);

	//Tick 설정
	PrimaryActorTick.bCanEverTick = false;
	PrimaryActorTick.bStartWithTickEnabled = false;

	// Collision 설정
	GetMesh()->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	GetMesh()->VisibilityBasedAnimTickOption
		= EVisibilityBasedAnimTickOption::OnlyTickMontagesWhenNotRendered;

	GetCharacterMovement()->SetComponentTickEnabled(false);
	GetCharacterMovement()->bOrientRotationToMovement = true;;

	GetCapsuleComponent()->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	GetCapsuleComponent()->SetCollisionProfileName(TEXT("Spectator"));
	GetCapsuleComponent()->SetComponentTickEnabled(false);

	HitBoxComp = CreateDefaultSubobject<UBoxComponent>(TEXT("HitBoxComponent"));
	HitBoxComp->SetComponentTickEnabled(false);
	HitBoxComp->SetupAttachment(RootComponent);
	HitBoxComp->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	HitBoxComp->SetCollisionProfileName(TEXT("Spectator"));
	HitBoxComp->SetCollisionResponseToChannel(ECC_GameTraceChannel5, ECR_Block); // CursorTrace (마우스 타겟팅 감지)
	HitBoxComp->SetCollisionResponseToChannel(ECC_GameTraceChannel3, ECR_Block); // VisionSensor (비전 센서 감지)

	// ASC 복제, 데이터 Minimal로 되는지 확인 필요
	ASC = CreateDefaultSubobject<UAbilitySystemComponent>(TEXT("AbilitySystemComponent"));
	ASC->SetComponentTickEnabled(false);
	ASC->SetIsReplicated(true);
	ASC->SetReplicationMode(EGameplayEffectReplicationMode::Minimal);
	
	AttributeSet = CreateDefaultSubobject<UBaseMonsterAttributeSet>(TEXT("AttributeSet"));

	// StateTree은 각 클라에서 실행
	StateTreeComp = CreateDefaultSubobject<UStateTreeComponent>(TEXT("StateTree"));
	StateTreeComp->SetComponentTickEnabled(false);
	StateTreeComp->SetStartLogicAutomatically(false);

	// 주변 플레이어 감지용 컴포넌트
	MonsterRangeComp = CreateDefaultSubobject<UMonsterRangeComponent>(TEXT("MonsterRangeComponent"));	
	MonsterRangeComp->SetComponentTickEnabled(false);
	MonsterRangeComp->SetIsReplicated(true);

	//UI Component
	HPBarWidgetComp = CreateDefaultSubobject<UWidgetComponent>(TEXT("WidgetComponent"));
	HPBarWidgetComp->SetComponentTickEnabled(false);
	HPBarWidgetComp->SetupAttachment(GetMesh());
	HPBarWidgetComp->SetWidgetSpace(EWidgetSpace::Screen); // 체력바 크기가 일정할거같으니까?
	HPBarWidgetComp->SetVisibility(false);

	SoundComp = CreateDefaultSubobject<UAudioComponent>(TEXT("AudioComponent"));
	SoundComp->SetComponentTickEnabled(false);
	SoundComp->SetupAttachment(RootComponent);

	TeamID = ETeamType::Neutral;

	//ItemBox
	LootableComp = CreateDefaultSubobject<ULootableComponent>(TEXT("LootableComponent"));
	LootableComp->SetComponentTickEnabled(false);
}

UAbilitySystemComponent* ABaseMonster::GetAbilitySystemComponent() const
{
	return ASC;
}

void ABaseMonster::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(ABaseMonster, bIsCombat);
	DOREPLIFETIME(ABaseMonster, bIsDead);
	DOREPLIFETIME(ABaseMonster, TeamID);
	DOREPLIFETIME(ABaseMonster, MonsterID);
	DOREPLIFETIME(ABaseMonster, MonsterLevel);
}

void ABaseMonster::PossessedBy(AController* newController)
{
	Super::PossessedBy(newController);

	if (IsValid(ASC) == false)
	{
		UE_LOG(LogTemp, Warning, TEXT("ABaseMonster::PossessedBy : %s Not ASC"), *GetName());
		return;
	}
	if (IsValid(AttributeSet) == false)
	{
		UE_LOG(LogTemp, Warning, TEXT("ABaseMonster::PossessedBy : %s Not AttributeSet"), *GetName());
		return;
	}

	ASC->InitAbilityActorInfo(this, this);

	if (HasAuthority())
	{
		AttributeSet->OnMonsterHit.AddDynamic(this, &ABaseMonster::MonsterGroupHitCall);
		AttributeSet->OnMonsterDeath.AddDynamic(this, &ABaseMonster::SendDeathEvent);
		AttributeSet->OnMoveSpeedChanged.AddDynamic(this, &ABaseMonster::OnMoveSpeedChangedHandle);
		MonsterRangeComp->OnPlayerCountOne.AddDynamic(this, &ABaseMonster::SendBeginSearchEvent);
		MonsterRangeComp->OnPlayerCountZero.AddDynamic(this, &ABaseMonster::SendEndSearchEvent);
		MonsterRangeComp->OnPlayerOut.AddDynamic(this, &ABaseMonster::SendTargetOffEvent);
		
		ASC->RegisterGameplayTagEvent(
			FGameplayTag::RequestGameplayTag("State.Debuff.Hard.Airborne"),
			EGameplayTagEventType::NewOrRemoved
			).AddUObject(this, &ABaseMonster::OnCCChanged);
		ASC->RegisterGameplayTagEvent(
			FGameplayTag::RequestGameplayTag("State.Debuff.Hard.Stun"),
			EGameplayTagEventType::NewOrRemoved
			).AddUObject(this, &ABaseMonster::OnCCChanged);
	}
}

void ABaseMonster::BeginPlay()
{
	Super::BeginPlay();
	
	if (HasAuthority())
	{
		StartLocation = GetActorLocation();
		StartRotator = GetActorRotation();
	}

	if (GetNetMode() != NM_DedicatedServer)
	{
		// UI 로직
		InitHPBar();
		AttributeSet->OnHealthChanged.AddDynamic(this, &ABaseMonster::OnHealthChangedHandle);
	}
}


void ABaseMonster::InitMonsterData(FPrimaryAssetId MonsterAssetId, float Level)
{
	MonsterID = MonsterAssetId;
	MonsterLevel = Level;
	InitMonsterDataLoading(MonsterAssetId, Level);
}

void ABaseMonster::InitMonsterDataLoading(FPrimaryAssetId MonsterAssetId, float Level)
{
	UObject* PreloadedData = UAssetManager::Get().GetPrimaryAssetObject(MonsterAssetId);
	if (PreloadedData)
	{
		// 이전에 ER_AssetPreloadSubsystem 등에 의해 이미 메모리에 올라와있다면 즉시 초기화
		OnMonsterDataLoaded(MonsterAssetId, Level);
	}
	else
	{
		// 로딩이 안된 경우 비동기 로딩 진행 (예방 차원)
		UAssetManager::Get().LoadPrimaryAsset(MonsterAssetId,
			TArray<FName>(),
			FStreamableDelegate::CreateUObject(
				this,
				&ABaseMonster::OnMonsterDataLoaded,
				MonsterAssetId,
				Level
			));
	}
}

void ABaseMonster::OnMonsterDataLoaded(FPrimaryAssetId MonsterAssetId, float Level)
{
	MonsterData = Cast<UMonsterDataAsset>(
		UAssetManager::Get().GetPrimaryAssetObject(MonsterAssetId)
	);
	if (IsValid(MonsterData) == false)
	{
		UE_LOG(LogTemp, Warning, TEXT("ABaseMonster::InitMonsterData - MonsterData is Not Valid!"));
	}


	if (HasAuthority())
	{
		ASC->AddLooseGameplayTag(MonsterData->AttackType);
		InitAttributes(Level);
		InitGiveAbilities();
	}
	
	InitVisuals();
	InitCollision();
	if (HasAuthority())
	{
		InitStateTree();
	}

	//Trigger BP event function
	OnMonsterDataLoadedEvent(MonsterAssetId, Level);
}

void ABaseMonster::InitGiveAbilities()
{
	if (IsValid(ASC) == false)
	{
		UE_LOG(LogTemp, Warning, TEXT("ABaseMonster::InitGiveAbilities - ASC is Not Valid!"));
		return;
	}

	if (MonsterData->DefaultAbilities.IsEmpty())
	{
		UE_LOG(LogTemp, Warning, TEXT("ABaseMonster::InitGiveAbilities - DefaultAbilities is Empty!"));
		return;
	}
	//행동
	for (auto& AbilityPtr : MonsterData->DefaultAbilities)
	{
		if (IsValid(AbilityPtr) && ASC)
		{
			ASC->GiveAbility(FGameplayAbilitySpec(AbilityPtr.Get(), 1, 0));
		}
	}
	//스킬
	for (TObjectPtr<USkillDataAsset> SkillDataAsset : MonsterData->SkillDataAssets)
	{
		if (IsValid(SkillDataAsset) && ASC)
		{
			FGameplayAbilitySpec Spec = SkillDataAsset->MakeSpec();
			ASC->GiveAbility(Spec);
		}
	}
}

void ABaseMonster::InitAttributes(float Level)
{
	if (IsValid(MonsterData->MonsterDataTable) == false)
	{
		UE_LOG(LogTemp, Warning, TEXT("ABaseMonster::InitAttributes : MonsterDataTable Not"));
		return;
	}
	if (IsValid(MonsterData->MonsterCurveTable) == false)
	{
		UE_LOG(LogTemp, Warning, TEXT("ABaseMonster::InitAttributes : MonsterCurveTable Not"));
		return;
	}

	FBaseMonsterTableRow* MonsterRow = 
		MonsterData->MonsterDataTable
		->FindRow<FBaseMonsterTableRow>(MonsterData->TableRowName, TEXT("MonsterData"));
	if (MonsterRow == nullptr)
	{
		UE_LOG(LogTemp, Warning, TEXT("ABaseMonster::InitAttributes : Not MonsterTableRow"));
		return;
	}
	AttributeSet->SetLevel(Level);
	AttributeSet->SetMaxLevel(MonsterRow->BaseMaxLevel);
	FRealCurve* MaxHealth = MonsterData->MonsterCurveTable->FindCurve(FName("MaxHealth"), TEXT("MonsterCurve"));
	if (MaxHealth)
	{
		AttributeSet->SetMaxHealth(MonsterRow->BaseMaxHealth + MaxHealth->Eval(Level));
		AttributeSet->SetHealth(MonsterRow->BaseMaxHealth + MaxHealth->Eval(Level));
	}
	AttributeSet->SetHealthRegen(MonsterRow->BaseHealthRegen);
	FRealCurve* AttackPower = MonsterData->MonsterCurveTable->FindCurve(FName("AttackPower"), TEXT("MonsterCurve"));
	if (AttackPower)
	{
		AttributeSet->SetAttackPower(MonsterRow->BaseAttackPower + AttackPower->Eval(Level));
	}
	FRealCurve* SkillAmp = MonsterData->MonsterCurveTable->FindCurve(FName("SkillAmp"), TEXT("MonsterCurve"));
	AttributeSet->SetAttackSpeed(MonsterRow->BaseAttackSpeed);
	if (SkillAmp)
	{
		AttributeSet->SetSkillAmp(MonsterRow->BaseSkillAmp + SkillAmp->Eval(Level));
	}
	AttributeSet->SetAttackRange(MonsterRow->BaseAttackRange);
	AttributeSet->SetCriticalChance(MonsterRow->BaseCriticalChance);
	AttributeSet->SetCriticalDamage(MonsterRow->BaseCriticalDamage);
	FRealCurve* Defense = MonsterData->MonsterCurveTable->FindCurve(FName("Defense"), TEXT("MonsterCurve"));
	if (Defense)
	{
		AttributeSet->SetDefense(MonsterRow->BaseDefense + Defense->Eval(Level));
	}
	AttributeSet->SetMoveSpeed(MonsterRow->BaseMoveSpeed);
	
}

void ABaseMonster::InitVisuals()
{
	if (IsValid(MonsterData->Mesh) == false || !GetMesh())
	{
		UE_LOG(LogTemp, Warning, TEXT("ABaseMonster::InitVisuals : MonsterData->Mesh Not"));
		return;
	}
	GetMesh()->SetSkeletalMesh(MonsterData->Mesh.Get());

	if (IsValid(MonsterData->Anim) == false || !GetMesh())
	{
		UE_LOG(LogTemp, Warning, TEXT("ABaseMonster::InitVisuals : MonsterData->Anim Not"));
		return;
	}
	GetMesh()->SetAnimInstanceClass(MonsterData->Anim.Get());
}

void ABaseMonster::InitCollision()
{
	GetMesh()->SetRelativeScale3D(MonsterData->MeshScale);
	GetCapsuleComponent()->SetCapsuleSize(MonsterData->CollisionRadius, MonsterData->CapsuleHalfHeight);
	GetCapsuleComponent()->SetCollisionProfileName("MonsterObjectCollision");
	HitBoxComp->SetBoxExtent(MonsterData->HitBoxExtent);
	HitBoxComp->SetCollisionProfileName("MonsterTraceCollision");
	HitBoxComp->SetCollisionResponseToChannel(ECC_GameTraceChannel5, ECR_Block); // CursorTrace (마우스 타겟팅 감지)
	HitBoxComp->SetCollisionResponseToChannel(ECC_GameTraceChannel3, ECR_Block); // VisionSensor (비전 센서 감지)
}

void ABaseMonster::InitStateTree()
{
	StateTreeComp->SetStateTree(MonsterData->MonsterStateTree);
	StateTreeComp->StartLogic();
}

void ABaseMonster::OnRep_IsCombat()
{
	// BP_BaseMonster Eventgraph에서 하는중
	//if (HPBarWidgetComp)
	//{
	//	HPBarWidgetComp->SetVisibility(bIsCombat);
	//}
}

void ABaseMonster::OnRep_IsDead()
{
	if (bIsDead && HPBarWidgetComp)
	{
		HPBarWidgetComp->SetVisibility(false);
	}
}

void ABaseMonster::OnRep_MonsterID()
{
	InitMonsterDataLoading(MonsterID, MonsterLevel);
}

void ABaseMonster::OnHealthChangedHandle(float CurrentHP, float MaxHP)
{
	UUserWidget* Widget = HPBarWidgetComp->GetUserWidgetObject();
	UProgressBar* HPBar = Cast<UProgressBar>(Widget->GetWidgetFromName(TEXT("HealthBar")));
	HPBar->SetPercent(CurrentHP / MaxHP);
}

void ABaseMonster::OnMoveSpeedChangedHandle(float OldSpeed, float NewSpeed)
{
	GetCharacterMovement()->MaxWalkSpeed = NewSpeed;
}

void ABaseMonster::Multicast_SetCollisionProfileName_Implementation(FName ProfileName)
{
	GetCapsuleComponent()->SetCollisionProfileName(ProfileName);
}

void ABaseMonster::InitHPBar()
{
	UUserWidget* Widget = HPBarWidgetComp->GetUserWidgetObject();
	if (IsValid(Widget) == false)
	{
		UE_LOG(LogTemp, Warning, TEXT("ABaseMonster::BeginPlay : Not Widget"));
	}
	UProgressBar* HPBar = Cast<UProgressBar>(Widget->GetWidgetFromName(TEXT("HealthBar")));
	if (IsValid(HPBar) == false)
	{
		UE_LOG(LogTemp, Warning, TEXT("ABaseMonster::BeginPlay : Not HPBar"));
	}

	HPBar->SetPercent(1.f);
}

void ABaseMonster::MonsterGroupHitCall(AActor* Target)
{
	SendHitEvent(Target); // 자신
	// 그룹 전파 
	if (MonsterRangeComp)
	{
		for (TObjectPtr<ABaseMonster> GroupMember : MonsterRangeComp->GetMonsterGroup())
		{
			// 살아있고 현재 전투 중이 아닌 동료만 대상
			if (IsValid(GroupMember) && !GroupMember->GetbIsDead() && !GroupMember->GetbIsCombat() && GroupMember != this)
			{
				// 동료가 직접 맞은 것처럼 OnMonterHitHandle 호출
				GroupMember->SendHitEvent(Target);
			}
		}
	}
}

// 서버에서만
void ABaseMonster::SendHitEvent(AActor* Target)
{
	if (bIsDead) return;

	if (!IsValid(Target))
	{
		return;
	}

	if (IsValid(TargetPlayer) && TargetPlayer != Target)
	{
		const float OldTargetDistance = FVector::DistSquared(TargetPlayer->GetActorLocation(), GetActorLocation());
		const float NewTargetDistance = FVector::DistSquared(Target->GetActorLocation(), GetActorLocation());

		if (NewTargetDistance < OldTargetDistance)
		{
			if (ABaseCharacter* OldTargetPlayer = Cast<ABaseCharacter>(TargetPlayer))
			{
				OldTargetPlayer->OnDeath.RemoveDynamic(this, &ABaseMonster::SendTargetOffEvent);
			}
			TargetPlayer = Target;
		}
		else
		{
			if (ABaseCharacter* BC = Cast<ABaseCharacter>(TargetPlayer))
			{
				if (!BC->OnDeath.IsAlreadyBound(this, &ABaseMonster::SendTargetOffEvent))
				{
					BC->OnDeath.AddDynamic(this, &ABaseMonster::SendTargetOffEvent);
				}
			}
		}
	}
	else if (!IsValid(TargetPlayer))
	{
		TargetPlayer = Target;
	}
	
	if (IsValid(StateTreeComp) == false)
	{
		UE_LOG(LogTemp, Warning, TEXT("ABaseMonster::OnMonterHitHandle : Not StateTree"));
		return;
	}
	if (MonsterTags.HitEventTag.IsValid() == false)
	{
		UE_LOG(LogTemp, Warning, TEXT("ABaseMonster::OnMonterHitHandle : Not HitEventTag"));
		return;
	}

	SendStateTreeEvent(MonsterTags.HitEventTag);
}

void ABaseMonster::SendDeathEvent(AActor* Target)
{
	if (bIsDead)
	{
		UE_LOG(LogTemp, Warning, TEXT("Monster is already dead"));
		return;
	}

	if (ABaseCharacter* TargetChar = Cast<ABaseCharacter>(TargetPlayer))
	{
		if (TargetChar->OnDeath.IsAlreadyBound(this, &ABaseMonster::SendTargetOffEvent))
		{
			TargetChar->OnDeath.RemoveDynamic(this, &ABaseMonster::SendTargetOffEvent);
		}
	}

	Death(); 
	RewardMonsterXP(
		Target,
		MonsterTags.IncomingXPTag, 
		MonsterData->Exp);
}

void ABaseMonster::RewardMonsterXP(AActor* Player, FGameplayTag Tag, float Amount)
{
	if (IsValid(Player) == false)
	{
		UE_LOG(LogTemp, Warning, TEXT("ABaseMonster::GiveRewardsToPlayer : Not Player"));
		return;
	}
	if (IsValid(MonsterData.Get()) == false)
	{
		UE_LOG(LogTemp, Warning, TEXT("ABaseMonster::GiveRewardsToPlayer : Not MonsterData"));
		return;
	}
	
	ABaseCharacter* BC = Cast<ABaseCharacter>(Player);
	int TeamIndex = (int)BC->GetTeamType();

	AER_GameState* EPS = GetWorld()->GetGameState<AER_GameState>();
	TArray<FString>& TeamPSArrayIDs = EPS->GetTeamArray(TeamIndex);

	TArray<TWeakObjectPtr<AER_PlayerState>> TeamPSArray;
	for (const FString& IDStr : TeamPSArrayIDs)
	{
		if (AER_PlayerState* PS = EPS->GetPlayerStateByUniqueId(IDStr))
		{
			TeamPSArray.Add(PS);
		}
	}

	FVector MonsterLocation = GetActorLocation();
	for (int32 i = TeamPSArray.Num() - 1; i >= 0; --i)
	{
		if (!TeamPSArray[i].IsValid()) continue;

		APlayerController* PC = TeamPSArray[i]->GetPlayerController();
		if (!PC) continue;

		APawn* Pawn = PC->GetPawn();
		if (!Pawn) continue;

		FVector TeamLocation = Pawn->GetActorLocation();
		float DistSq = FVector::DistSquared(TeamLocation, MonsterLocation);

		if (DistSq > 1000000.f)
		{
			TeamPSArray.RemoveAt(i);
		}
	}

	float OffsetAmount;
	int NearCount = TeamPSArray.Num();
	switch (NearCount)
	{
	case 1: OffsetAmount = Amount * 1;
		break;
	case 2: OffsetAmount = Amount * 0.8f;
		break;
	case 3: OffsetAmount = Amount * 0.7f;
		break;
	default: OffsetAmount = Amount * 0.7f; 
		break;
	}

	for(auto TeamPS : TeamPSArray)
	{
		if (!TeamPS.IsValid()) continue;
		FGameplayEffectContextHandle ContextHandle = ASC->MakeEffectContext();;
		ContextHandle.AddInstigator(this, nullptr);

		TSubclassOf<UGE_AddXP> GE = UGE_AddXP::StaticClass();
		FGameplayEffectSpecHandle SpecHandle = ASC->MakeOutgoingSpec(GE, 1, ContextHandle);
		SpecHandle.Data->SetSetByCallerMagnitude(
			Tag,
			OffsetAmount
		); 

		UAbilitySystemComponent* TargetASC = TeamPS->GetAbilitySystemComponent();
		if (!TargetASC) continue;
		TargetASC->ApplyGameplayEffectSpecToSelf(*SpecHandle.Data);
	}
}

void ABaseMonster::OnCooldown(FGameplayTag CooldownTag, float Cooldown)
{
	AddCooldownTag(CooldownTag);
	FTimerHandle& TimerHandle = CooldownTimerMap.FindOrAdd(CooldownTag);

	GetWorld()->GetTimerManager().SetTimer(
		TimerHandle,
		FTimerDelegate::CreateLambda([this, CooldownTag]()
			{
				RemoveCooldownTag(CooldownTag);
			}),
		Cooldown,
		false
	);
}

void ABaseMonster::AddCooldownTag(FGameplayTag CooldownTag)
{
	ASC->AddLooseGameplayTag(CooldownTag);
}

void ABaseMonster::RemoveCooldownTag(FGameplayTag CooldownTag)
{
	ASC->RemoveLooseGameplayTag(CooldownTag);
}

void ABaseMonster::SendTargetOffEvent()
{
	if (bIsDead) return;

	if (MonsterTags.TargetOffEventTag.IsValid() == false)
	{
		UE_LOG(LogTemp, Warning, TEXT("ABaseMonster::OnMonterHitHandle : Not TargetOffEventTag"));
		return;
	}

	ABaseCharacter* TargetChar = Cast<ABaseCharacter>(TargetPlayer);
	if (IsValid(TargetChar))
	{
		if (TargetChar->OnDeath.IsAlreadyBound(this, &ABaseMonster::SendTargetOffEvent))
		{
			TargetChar->OnDeath.RemoveDynamic(this, &ABaseMonster::SendTargetOffEvent);
		}
	}

	SendStateTreeEvent(MonsterTags.TargetOffEventTag);
	TargetPlayer = nullptr;
}

void ABaseMonster::SendBeginSearchEvent()
{ 
	if (bIsDead) return;

	if (IsValid(ASC) == false)
	{
		UE_LOG(LogTemp, Warning, TEXT("ABaseMonster::OnPlayerCountOneHandle : Not ASC"));
		return;
	}
	if (IsValid(StateTreeComp) == false)
	{
		UE_LOG(LogTemp, Warning, TEXT("ABaseMonster::OnPlayerCountOneHandle : Not StateTree"));
		return;
	}
	if (StateTreeComp->IsRunning() == false)
	{
		//UE_LOG(LogTemp, Warning, TEXT("ABaseMonster::OnPlayerCountOneHandle : Not StateTree Running"));
		return;
	}
	if (MonsterTags.BeginSearchEventTag.IsValid() == false)
	{
		UE_LOG(LogTemp, Warning, TEXT("ABaseMonster::OnMonterHitHandle : Not BeginSearchEventTag"));
		return;
	}

	FGameplayEventData* Payload = new FGameplayEventData();
	ASC->HandleGameplayEvent(MonsterTags.BeginSearchEventTag, Payload);
	SendStateTreeEvent(MonsterTags.BeginSearchEventTag);
}  

void ABaseMonster::SendEndSearchEvent()
{
	if (bIsDead) return;

	if (IsValid(ASC) == false)
	{
		UE_LOG(LogTemp, Warning, TEXT("ABaseMonster::OnPlayerCountZeroHandle : Not ASC"));
		return;
	}
	if (IsValid(StateTreeComp) == false)
	{
		UE_LOG(LogTemp, Warning, TEXT("ABaseMonster::OnPlayerCountZeroHandle : Not StateTree"));
		return;
	}
	if (StateTreeComp->IsRunning() == false)
	{
		UE_LOG(LogTemp, Warning, TEXT("ABaseMonster::OnPlayerCountZeroHandle : StateTree Loading..."));
		return;
	}
	if (MonsterTags.EndSearchEventTag.IsValid() == false)
	{
		UE_LOG(LogTemp, Warning, TEXT("ABaseMonster::OnPlayerCountZeroHandle : Not EndSearchEventTag"));
		return;
	}

	if (bIsCombat == false)
	{
		FGameplayEventData* Payload = new FGameplayEventData();
		ASC->HandleGameplayEvent(FGameplayTag(MonsterTags.EndSearchEventTag), Payload);
		SendStateTreeEvent(MonsterTags.EndSearchEventTag);
	}
}

void ABaseMonster::SendAttackRangeEvent(float AttackRange)
{
	if (bIsDead) return;

	if (IsValid(TargetPlayer) == false)
	{
		SendStateTreeEvent(MonsterTags.TargetOffEventTag);
		return;
	}
	
	const float Distance = FVector::DistSquared(
		TargetPlayer->GetActorLocation(), GetActorLocation());
	float SquaAttackRange = AttackRange * AttackRange;

	if (Distance <= SquaAttackRange)
	{
		// 공격
		if (MonsterTags.AttackEventTag.IsValid() == false)
		{
			UE_LOG(LogTemp, Warning, TEXT("ABaseMonster::SendAttackRangeEvent : Not AttackEventTag"));
			return;
		}
		SendStateTreeEvent(MonsterTags.AttackEventTag);
	}
	else
	{
		// 다시 체이스
		if (MonsterTags.TargetOnEventTag.IsValid() == false)
		{
			UE_LOG(LogTemp, Warning, TEXT("ABaseMonster::SendAttackRangeEvent : Not TargetOnEventTag"));
			return;
		}
		SendStateTreeEvent(MonsterTags.TargetOnEventTag);
	}
}

void ABaseMonster::SendStateTreeEvent(FGameplayTag InputTag)
{
	if (IsValid(StateTreeComp) == false)
	{
		UE_LOG(LogTemp, Warning, TEXT("ABaseMonster::SendStateTreeEvent : Not StateTreeComp"));
		return;
	}
	if (StateTreeComp->IsRunning() == false)
	{
		//UE_LOG(LogTemp, Warning, TEXT("ABaseMonster::SendStateTreeEvent : Not StateTreeComp Running"));
		return;
	}
	StateTreeComp->SendStateTreeEvent(InputTag);
}



////////////////////////// Get, Set /////////////////////////////

void ABaseMonster::SetTargetPlayer(AActor* Target)
{
	TargetPlayer = Target;
}

void ABaseMonster::SetbIsCombat(bool value)
{
	if (bIsCombat != value)
	{
		bIsCombat = value;

		// 리슨 서버 호스트의 경우 OnRep 함수가 자동 호출되지 않으므로 수동으로 호출해 줍니다.
		if (GetNetMode() != NM_DedicatedServer)
		{
			OnRep_IsCombat();
		}
	}
}

void ABaseMonster::SetbIsDead(bool value)
{
	if (bIsDead != value)
	{
		bIsDead = value;

		if (GetNetMode() != NM_DedicatedServer)
		{
			OnRep_IsDead();
		}
	}
}

void ABaseMonster::SetIsFirstAttack(bool bIsFirst)
{
	bIsFirstAttack = bIsFirst;
}

void ABaseMonster::SetAttackCount(uint8 Count)
{
	AttackCount = Count;
}




AActor* ABaseMonster::GetTargetPlayer() const
{
	return TargetPlayer;
}

bool ABaseMonster::GetbIsCombat() const
{
	return bIsCombat;
}

bool ABaseMonster::GetbIsDead() const
{
	return bIsDead;
}

FVector ABaseMonster::GetStartLocation() const
{
	return StartLocation;
}

FRotator ABaseMonster::GetStartRotator() const
{
	return StartRotator;
}

UMonsterRangeComponent* ABaseMonster::GetMonsterRangeComp() const
{
	return MonsterRangeComp; 
}

FMonsterTags ABaseMonster::GetMonsterTags() const
{
	return MonsterTags;
}

FPrimaryAssetId ABaseMonster::GetMonsterId() const
{
	return MonsterID;
}

UBaseMonsterAttributeSet* ABaseMonster::GetAttributeSet() const
{
	return AttributeSet;
}

bool ABaseMonster::GetIsFirstAttack() const
{
	return bIsFirstAttack;
}

uint8 ABaseMonster::GetAttackCount() const
{
	return AttackCount;
}





ETeamType ABaseMonster::GetTeamType() const
{
	return TeamID;
}

bool ABaseMonster::IsTargetable() const
{
	if (IsValid(ASC) == false)
	{
		UE_LOG(LogTemp, Warning, TEXT("ABaseMonster::IsTargetable : Not ASC"));
		return false;
	}
	if (MonsterTags.DeathStateTag.IsValid() == false)
	{
		UE_LOG(LogTemp, Warning, TEXT("ABaseMonster::IsTargetable : Not DeathStateTag"));
		return false;
	}

	return !ASC->HasMatchingGameplayTag(MonsterTags.DeathStateTag);
}

void ABaseMonster::Server_SetTeamID_Implementation(ETeamType NewTeamID)
{
	TeamID = NewTeamID;
	OnRep_TeamID();
}

void ABaseMonster::HighlightActor(bool bIsHighlight, int32 StencilValue)
{
	if (USkeletalMeshComponent* MyMesh = GetMesh())
	{
		// 커스텀 뎁스 렌더링 켜기/끄기
		MyMesh->SetRenderCustomDepth(bIsHighlight);

		if (bIsHighlight)
		{
			// 스텐실 값 부여 (어떤 색으로 아웃라인을 그릴지 포스트 프로세스에 전달)
			MyMesh->SetCustomDepthStencilValue(StencilValue);
		}
	}
}

void ABaseMonster::OnRep_TeamID()
{

}

void ABaseMonster::Death()
{
	if (HasAuthority() == false)
	{
		return;
	}
		
	bIsDead = true;
	TargetPlayer = nullptr;
	bIsCombat = false;

	auto InGameMode = Cast<AER_InGameMode>(GetWorld()->GetAuthGameMode());
	InGameMode->NotifyNeutralDied(this);

	if (ASC)
	{
		ASC->CancelAllAbilities();
	}

	SetLifeSpan(20.f);

	if (MonsterTags.DeathEventTag.IsValid() == false)
	{
		UE_LOG(LogTemp, Warning, TEXT("ABaseMonster::OnMonterDeathHandle : Not DeathEventTag"));
		return;
	}
	SendStateTreeEvent(MonsterTags.DeathEventTag);
}

void ABaseMonster::OnCCChanged(FGameplayTag Tag, int32 NewCount)
{
	if (bIsDead) return;

	if (NewCount > 0)
	{
		bIsCombat = true;
		SendStateTreeEvent(FGameplayTag::RequestGameplayTag("Event.State.Debuff"));
		ASC->CancelAllAbilities();
	} 
	else
	{
		// 재검사용
		SendStateTreeEvent(FGameplayTag::RequestGameplayTag("Event.State.Debuff"));
	}
}

void ABaseMonster::OffCCChanged()
{
	// CC 상태 종료 후 Combat상태로 전환용
	bIsCombat = false;
	SendStateTreeEvent(MonsterTags.HitEventTag);
}
