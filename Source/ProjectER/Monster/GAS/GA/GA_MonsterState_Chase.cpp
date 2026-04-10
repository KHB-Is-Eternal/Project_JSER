#include "Monster/GAS/GA/GA_MonsterState_Chase.h"
#include "Monster/BaseMonster.h"
#include "AbilitySystemComponent.h"
#include "Navigation/PathFollowingComponent.h"
#include "Monster/GAS/AttributeSet/BaseMonsterAttributeSet.h"
#include "Components/CapsuleComponent.h"
#include "GameFramework/CharacterMovementComponent.h"



UGA_MonsterState_Chase::UGA_MonsterState_Chase()
{
	StateInitData.MonsterAssetTags = FGameplayTagContainer(FGameplayTag::RequestGameplayTag("Ability.Action.Chase"));
	StateInitData.MontageType = EMonsterMontageType::Move;
	StateInitData.NiagaraCueTag = FGameplayTag::RequestGameplayTag("GameplayCue.Particle.Action.Move");
	StateInitData.SoundCueTag = FGameplayTag::RequestGameplayTag("GameplayCue.Sound.Action.Move");
	StateInitData.WaitTag = FGameplayTag::RequestGameplayTag("State.Action.Move");
	bIsUseWaitTag = true;
	SetAssetTags(StateInitData.MonsterAssetTags);
}

void UGA_MonsterState_Chase::OnGiveAbility(const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilitySpec& Spec)
{
	Super::OnGiveAbility(ActorInfo, Spec);
}

void UGA_MonsterState_Chase::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);

	ABaseMonster* Monster = Cast<ABaseMonster>(GetOwningActorFromActorInfo());
	if (IsValid(Monster) == false)
	{
		UE_LOG(LogTemp, Warning, TEXT("UGA_MonsterState_Chase::ActivateAbility : Not Monster"));
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}
	if (IsValid(Monster->MonsterData) == false)
	{
		UE_LOG(LogTemp, Warning, TEXT("UGA_MonsterState_Chase::ActivateAbility : Not MonsterData"));
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}
	AAIController* AIC = Cast<AAIController>(Monster->GetController());
	if (IsValid(AIC) == false)
	{
		UE_LOG(LogTemp, Warning, TEXT("UGA_MonsterState_Chase::ActivateAbility : Not AIC"));
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}
	AActor* TargetActor = Monster->GetTargetPlayer();
	if (IsValid(TargetActor) == false)
	{
		UE_LOG(LogTemp, Warning, TEXT("UGA_MonsterState_Chase::ActivateAbility : Not TargetActor"));
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}
	UBaseMonsterAttributeSet* AS = Monster->GetAttributeSet();
	if (IsValid(AS) == false)
	{
		UE_LOG(LogTemp, Warning, TEXT("UGA_MonsterState_Chase::ActivateAbility : Not AttributeSet"));
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}
	UCapsuleComponent* MonsterCapsuleComp = Monster->GetCapsuleComponent();
	if (IsValid(MonsterCapsuleComp) == false)
	{
		UE_LOG(LogTemp, Warning, TEXT("UGA_MonsterState_Chase::ActivateAbility : Not MonsterCapsuleComp"));
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}	
	ACharacter* TargetCharacter = Cast<ACharacter>(TargetActor);
	if (IsValid(TargetCharacter) ==false)
	{
		UE_LOG(LogTemp, Warning, TEXT("UGA_MonsterState_Chase::ActivateAbility : Not TargetCharacter"));
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}	
	UCapsuleComponent* TargetCapsuleComp = TargetCharacter->GetCapsuleComponent();
	if (IsValid(TargetCapsuleComp) == false)
	{
		UE_LOG(LogTemp, Warning, TEXT("UGA_MonsterState_Chase::ActivateAbility : Not TargetCapsuleComp"));
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}
	
	Monster->SetbIsCombat(true);

	float AttackRange = 0.0f;
	AttackRange = AS->GetAttackRange();
	float MyCapsuleRadius = 0.0f;
	MyCapsuleRadius = MonsterCapsuleComp->GetScaledCapsuleRadius();
	float TargetRadius = 0.0f;
	TargetRadius = TargetCapsuleComp->GetScaledCapsuleRadius();
	float AcceptanceRadius = FMath::Max(0.0f, AttackRange - (MyCapsuleRadius + TargetRadius));

	AIC->ReceiveMoveCompleted.RemoveAll(this);
	
	EPathFollowingRequestResult::Type ReqResult = AIC->MoveToActor(TargetActor, AcceptanceRadius, false);
	if (ReqResult == EPathFollowingRequestResult::RequestSuccessful)
	{
		MoveRequestID = AIC->GetCurrentMoveRequestID();
		AIC->ReceiveMoveCompleted.AddDynamic(this, &UGA_MonsterState_Chase::OnMoveFinished);
	}
	else if (ReqResult == EPathFollowingRequestResult::AlreadyAtGoal)
	{
		MoveRequestID = AIC->GetCurrentMoveRequestID();
		OnMoveFinished(MoveRequestID, EPathFollowingResult::Success);
	}
	else if (ReqResult == EPathFollowingRequestResult::Failed)
	{
		MoveRequestID = AIC->GetCurrentMoveRequestID();
		OnMoveFinished(MoveRequestID, EPathFollowingResult::Aborted);
	}
}


void UGA_MonsterState_Chase::OnMoveFinished(FAIRequestID RequestID, EPathFollowingResult::Type Result)
{
	if (RequestID != MoveRequestID)
	{
		return;
	}

	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
}

void UGA_MonsterState_Chase::EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled)
{
	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);

	ABaseMonster* Monster = Cast<ABaseMonster>(GetOwningActorFromActorInfo());
	if (IsValid(Monster) == false)
	{
		UE_LOG(LogTemp, Warning, TEXT("UGA_MonsterState_Chase::EndAbility : Not Monster"));
		return;
	}
	AAIController* AIC = Cast<AAIController>(Monster->GetController());
	if (IsValid(Monster) == false)
	{
		UE_LOG(LogTemp, Warning, TEXT("UGA_MonsterState_Chase::EndAbility : Not AIC"));
		return;
	}
	UAnimInstance* AnimInstance = Monster->GetMesh()->GetAnimInstance();
	if (IsValid(AnimInstance) == false)
	{
		UE_LOG(LogTemp, Warning, TEXT("UGA_MonsterState_Chase::EndAbility : Not AnimInstance"));
		return;
	}
	UBaseMonsterAttributeSet* AS = Monster->GetAttributeSet();
	if (IsValid(AS) == false)
	{
		UE_LOG(LogTemp, Warning, TEXT("UGA_MonsterState_Chase::EndAbility : Not AttributeSet"));
		return;
	}
	
	AIC->ReceiveMoveCompleted.RemoveAll(this);
	AnimInstance->StopAllMontages(0.f);
	Monster->SendAttackRangeEvent(AS->GetAttackRange());
}
