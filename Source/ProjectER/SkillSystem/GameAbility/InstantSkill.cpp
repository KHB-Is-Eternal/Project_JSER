// Fill out your copyright notice in the Description page of Project Settings.


#include "SkillSystem/GameAbility/InstantSkill.h"
#include "SkillSystem/GameplayAbilityTargetActor/MouseLocationTargetActor.h"

void UInstantSkill::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);

	// 스마트 캐스트(이벤트 트리거)인 경우 즉시 시전
	if (TriggerEventData && TriggerEventData->TargetData.IsValid(0))
	{
		PrepareToActiveSkill();
		return;
	}

	// 수동 조준(매뉴얼 에이밍) 모드 진입
	const bool bIsManual = (TriggerEventData != nullptr && !TriggerEventData->TargetData.IsValid(0));
	StartIndicatorMode(bIsManual);
}

void UInstantSkill::StartIndicatorMode(bool bIsManual)
{
	Super::StartIndicatorMode(bIsManual);
	SetWaitTargetTask();
}

TSubclassOf<class AGameplayAbilityTargetActor> UInstantSkill::GetTargetActorClass() const
{
	// 인스턴트 스킬은 수동 조준 시에만 클릭을 대기하기 위해 더미 용도로 타겟 액터를 사용합니다.
	if (bIsManualAiming)
	{
		return AMouseLocationTargetActor::StaticClass();
	}
	return nullptr;
}

void UInstantSkill::OnTargetDataReady(const FGameplayAbilityTargetDataHandle& DataHandle)
{
	Super::OnTargetDataReady(DataHandle);

	// 인스턴트 스킬은 타겟 데이터(위치 등)를 무시하고 즉시 시전합니다.
	PrepareToActiveSkill();
}