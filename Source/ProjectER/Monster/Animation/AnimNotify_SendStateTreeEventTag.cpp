#include "Monster/Animation/AnimNotify_SendStateTreeEventTag.h"

#include "Components/StateTreeComponent.h"
#include "Monster/BaseMonster.h"

void UAnimNotify_SendStateTreeEventTag::Notify(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, const FAnimNotifyEventReference& EventReference)
{
	Super::Notify(MeshComp, Animation, EventReference);

	if (IsValid(MeshComp) == false)
	{
		UE_LOG(LogTemp, Warning, TEXT("UAnimNotify_SendStateTreeEventTag::Notify : Not MeshComp"));
		return;
	}
	AActor* Owner = MeshComp->GetOwner();
	if (IsValid(Owner) == false)
	{
		UE_LOG(LogTemp, Warning, TEXT("UAnimNotify_SendStateTreeEventTag::Notify : Not Owner"));
		return;
	}
	if (Owner->HasAuthority() == false)
	{
		return;
	}

	ABaseMonster* Monster = Cast<ABaseMonster>(Owner);
	if (IsValid(Monster) == false)
	{
		UE_LOG(LogTemp, Warning, TEXT("UAnimNotify_SendStateTreeEventTag::Notify : Not Monster"));
		return;
	}

	Monster->SendStateTreeEvent(EventTag);
}
