// Fill out your copyright notice in the Description page of Project Settings.

#include "OverflowSetByCallerGEC.h"
#include "AbilitySystemComponent.h"
#include "GameplayEffect.h"
#include "SkillSystem/GameplayEffect/BaseGameplayEffect.h"

UOverflowSetByCallerGEC::UOverflowSetByCallerGEC()
{
}

bool UOverflowSetByCallerGEC::CanGameplayEffectApply(const FActiveGameplayEffectsContainer& ActiveGEContainer, const FGameplayEffectSpec& GESpec) const
{
	UAbilitySystemComponent* TargetASC = ActiveGEContainer.Owner;
	if (!IsValid(TargetASC))
	{
		return true;
	}

	// 적용하려는 GE의 클래스
	UClass* EffectClass = GESpec.Def->GetClass();
	if (!EffectClass)
	{
		return true;
	}

	// 1. 타겟이 현재 가지고 있는 동일 클래스 GE의 스택을 합산합니다.
	FGameplayEffectQuery Query;
	Query.EffectDefinition = EffectClass;
	
	int32 CurrentStackCount = 0;
	TArray<FActiveGameplayEffectHandle> ActiveEffects = TargetASC->GetActiveEffects(Query);
	
	for (const FActiveGameplayEffectHandle& Handle : ActiveEffects)
	{
		const FActiveGameplayEffect* ActiveGE = TargetASC->GetActiveGameplayEffect(Handle);
		if (ActiveGE)
		{
			CurrentStackCount += ActiveGE->Spec.GetStackCount();
		}
	}

	// 스택 제한이 없는 GE라면 오버플로우가 발생할 일이 없습니다.
	if (GESpec.Def->StackingType == EGameplayEffectStackingType::None)
	{
		return true;
	}

	// 2. 한계치 검사 (원본 GE의 한계치 직접 사용)
	if (CurrentStackCount >= GESpec.Def->StackLimitCount)
	{
		// 스택 한계 도달! Overflow 발동
		if (IsValid(OverflowEffect))
		{
			UAbilitySystemComponent* SourceASC = GESpec.GetContext().GetInstigatorAbilitySystemComponent();
			if (IsValid(SourceASC))
			{
				FGameplayEffectSpecHandle OverflowSpecHandle = SourceASC->MakeOutgoingSpec(OverflowEffect, GESpec.GetLevel(), GESpec.GetContext());
				
				if (OverflowSpecHandle.IsValid())
				{
					// 핵심: 원본 스펙의 SetByCaller 매그니튜드를 전부 복사
					OverflowSpecHandle.Data->SetByCallerTagMagnitudes = GESpec.SetByCallerTagMagnitudes;

					// 오버플로우 적용
					SourceASC->ApplyGameplayEffectSpecToTarget(*OverflowSpecHandle.Data.Get(), TargetASC);
				}
			}
		}

		// 원본 GE의 적용 거부 여부 반환
		return !bDenyOverflowApplication;
	}

	return true;
}
