// Fill out your copyright notice in the Description page of Project Settings.

#include "CooldownRefundGEC.h"
#include "AbilitySystemComponent.h"
#include "SkillSystem/GameAbility/SkillBase.h"

UCooldownRefundGEC::UCooldownRefundGEC()
{
}

void UCooldownRefundGEC::OnGameplayEffectExecuted(FActiveGameplayEffectsContainer& ActiveGEContainer, FGameplayEffectSpec& GESpec, FPredictionKey& PredictionKey) const
{
	Super::OnGameplayEffectExecuted(ActiveGEContainer, GESpec, PredictionKey);

	UAbilitySystemComponent* const TargetASC = ActiveGEContainer.Owner;
	if (TargetASC == nullptr || !TargetASC->IsOwnerActorAuthoritative())
	{
		return;
	}

	const float ReduceSeconds = RefundAmount.GetValueAtLevel(GESpec.GetLevel());
	if (ReduceSeconds <= 0.0f)
	{
		return;
	}

	FGameplayTagContainer TagsToReduce;

	// Config에 명시된 특정 쿨타임 태그가 있다면 우선 적용
	if (!TargetCooldownTags.IsEmpty())
	{
		TagsToReduce.AppendTags(TargetCooldownTags);
	}
	else
	{
		// 비어있다면, Context를 통해 자신을 유발한 원본 어빌리티를 찾음
		if (const UGameplayAbility* InstigatorAbility = GESpec.GetContext().GetAbility())
		{
			// 어빌리티가 USkillBase 타입이라면 쿨타임 태그 획득 시도
			if (const USkillBase* SkillBase = Cast<USkillBase>(InstigatorAbility))
			{
				if (const FGameplayTagContainer* CooldownTags = SkillBase->GetCooldownTags())
				{
					TagsToReduce.AppendTags(*CooldownTags);
				}
			}
		}
	}

	if (!TagsToReduce.IsEmpty())
	{
		FGameplayEffectQuery Query = FGameplayEffectQuery::MakeQuery_MatchAnyOwningTags(TagsToReduce);
		TArray<FActiveGameplayEffectHandle> ActiveHandles = TargetASC->GetActiveEffects(Query);

		float CurrentTime = TargetASC->GetWorld()->GetTimeSeconds();

		for (const FActiveGameplayEffectHandle& ActiveHandle : ActiveHandles)
		{
			const FActiveGameplayEffect* ActiveGE = TargetASC->GetActiveGameplayEffect(ActiveHandle);
			if (ActiveGE != nullptr)
			{
				const float TimeRemaining = ActiveGE->GetTimeRemaining(CurrentTime);

				if (TimeRemaining <= ReduceSeconds)
				{
					TargetASC->RemoveActiveGameplayEffect(ActiveHandle);
				}
				else
				{
					// [Fix] Remove → Apply 패턴 대신, 시작 시간을 직접 앞당겨 남은 시간을 줄입니다.
					// GE 재생성 방식은 GAS 예측 Reconcile 및 콜백 내 컨테이너 수정으로 인해
					// 남은 쿨타임이 길 때 간헐적으로 감소가 적용되지 않는 버그를 유발했습니다.
					FActiveGameplayEffect* MutableGE = const_cast<FActiveGameplayEffect*>(ActiveGE);
					MutableGE->StartWorldTime -= ReduceSeconds;
					MutableGE->StartServerWorldTime -= ReduceSeconds;

					// MarkItemDirty 직접 호출은 PushModel 링커 의존성을 유발하므로,
					// 엔진 export API를 통해 간접적으로 dirty 마킹 + replication을 트리거합니다.
					const FGameplayTag CoolTimeTag = FGameplayTag::RequestGameplayTag(FName("Skill.Data.CoolTime"));
					const float CurrentSetByCaller = ActiveGE->Spec.GetSetByCallerMagnitude(CoolTimeTag, false);
					ActiveGEContainer.UpdateActiveGameplayEffectSetByCallerMagnitude(ActiveHandle, CoolTimeTag, CurrentSetByCaller);
					ActiveGEContainer.CheckDuration(ActiveHandle);

					// 시간 변경 이벤트를 브로드캐스트하여 로컬 리스너가 즉시 감지할 수 있도록 합니다.
					MutableGE->EventSet.OnTimeChanged.Broadcast(ActiveHandle, MutableGE->StartWorldTime, MutableGE->GetDuration());
				}
			}
		}
	}
}
