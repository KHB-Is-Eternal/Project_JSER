#include "SkillSystem/GameplayEffectComponent/BaseGEC.h"
#include "SkillSystem/GameplayEffectComponent/AdditionalEffectGEC.h"
#include "AbilitySystemComponent.h"
#include "GameplayEffect.h"
#include "SkillSystem/GameplayEffect/BaseGameplayEffect.h"
#include "SkillSystem/SkillDataAsset.h"
#include "SkillSystem/GameAbility/MouseClickSkill.h"
#include "SkillSystem/GameAbility/MouseTargetSkill.h"
#include "SkillSystem/GameAbility/InstantSkill.h"

UBaseGEC::UBaseGEC()
{
}

void UBaseGEC::OnGameplayEffectExecuted(FActiveGameplayEffectsContainer& ActiveGEContainer, FGameplayEffectSpec& GESpec, FPredictionKey& PredictionKey) const
{
	Super::OnGameplayEffectExecuted(ActiveGEContainer, GESpec, PredictionKey);
}

FSkillTooltipData UBaseGEC::GetTooltipDescription(int32 Level, TSubclassOf<class USkillBase> AbilityClass) const
{
	return FSkillTooltipData();
}

FText UBaseGEC::FormatAppliedEffects(const TArray<TSubclassOf<class UBaseGameplayEffect>>& Effects, int32 Level)
{
	TArray<FString> Lines;

	for (const TSubclassOf<UBaseGameplayEffect>& EffectClass : Effects)
	{
		if (!EffectClass) continue;

		const UBaseGameplayEffect* EffectDef = EffectClass->GetDefaultObject<UBaseGameplayEffect>();
		if (!EffectDef) continue;

		FString TargetPrefix = TEXT("효과");
		if (EffectDef->TargetRelationship == ETargetRelationship::Enemy)
		{
			TargetPrefix = TEXT("효과(대상 적)");
		}
		else if (EffectDef->TargetRelationship == ETargetRelationship::Friend)
		{
			TargetPrefix = TEXT("효과(대상 아군)");
		}

		auto FormatMagnitude = [&](const FGameplayEffectModifierMagnitude& Magnitude, const FString& AttrName) -> FString
		{
			FString ModDesc;
			if (Magnitude.GetMagnitudeCalculationType() == EGameplayEffectMagnitudeCalculation::ScalableFloat)
			{
				float Value = 0.0f;
				if (Magnitude.GetStaticMagnitudeIfPossible(Level, Value))
				{
					ModDesc = FString::Printf(TEXT("%s : %.1f"), *AttrName, Value);
				}
			}
			else if (Magnitude.GetMagnitudeCalculationType() == EGameplayEffectMagnitudeCalculation::AttributeBased)
			{
				const FAttributeBasedFloat* AttrFloatPtr = nullptr;
				if (const FStructProperty* StructProp = CastField<FStructProperty>(FGameplayEffectModifierMagnitude::StaticStruct()->FindPropertyByName(FName("AttributeBasedMagnitude"))))
				{
					AttrFloatPtr = StructProp->ContainerPtrToValuePtr<FAttributeBasedFloat>(&Magnitude);
				}

				if (AttrFloatPtr)
				{
					float Coeff = AttrFloatPtr->Coefficient.GetValueAtLevel(Level);
					float PreAdd = AttrFloatPtr->PreMultiplyAdditiveValue.GetValueAtLevel(Level);
					float PostAdd = AttrFloatPtr->PostMultiplyAdditiveValue.GetValueAtLevel(Level);
					FString BackingAttr = AttrFloatPtr->BackingAttribute.AttributeToCapture.GetName();

					if (PreAdd + PostAdd == 0.f)
					{
						ModDesc = FString::Printf(TEXT("%s : (%.2f * %s)"), *AttrName, Coeff, *BackingAttr);
					}
					else
					{
						ModDesc = FString::Printf(TEXT("%s : %.1f + (%.2f * %s)"), *AttrName, PreAdd + PostAdd, Coeff, *BackingAttr);
					}
				}
			}
			return ModDesc;
		};

		for (const FGameplayModifierInfo& ModInfo : EffectDef->Modifiers)
		{
			FString ModDesc = FormatMagnitude(ModInfo.ModifierMagnitude, ModInfo.Attribute.GetName());
			if (!ModDesc.IsEmpty())
			{
				Lines.Add(FString::Printf(TEXT("%s : %s"), *TargetPrefix, *ModDesc));
			}
		}

		for (const FGameplayEffectExecutionDefinition& ExecDef : EffectDef->Executions)
		{
			for (const FGameplayEffectExecutionScopedModifierInfo& ScopedMod : ExecDef.CalculationModifiers)
			{
				FString AttrName;
				if (ScopedMod.AggregatorType == EGameplayEffectScopedModifierAggregatorType::CapturedAttributeBacked)
				{
					AttrName = ScopedMod.CapturedAttribute.AttributeToCapture.GetName();
				}
				else
				{
					AttrName = ScopedMod.TransientAggregatorIdentifier.ToString();
				}

				FString ModDesc = FormatMagnitude(ScopedMod.ModifierMagnitude, AttrName);
				if (!ModDesc.IsEmpty())
				{
					Lines.Add(FString::Printf(TEXT("%s : %s"), *TargetPrefix, *ModDesc));
				}
			}
		}
	}

	return FText::FromString(FString::Join(Lines, TEXT("\n")));
}


void UBaseGEC::InheritHitTags(const FGameplayEffectSpec& ParentSpec, FGameplayEffectSpecHandle& ChildSpecHandle)
{
	if (!ChildSpecHandle.IsValid()) return;

	static const FGameplayTag HitBaseTag = FGameplayTag::RequestGameplayTag(FName("Event.Action.Hit"));

	// 부모 Spec의 DynamicGrantedTags에서 Event.Action.Hit 하위 태그들을 찾아 자식 Spec에 주입
	for (const FGameplayTag& Tag : ParentSpec.DynamicGrantedTags)
	{
		if (Tag.MatchesTag(HitBaseTag))
		{
			ChildSpecHandle.Data.Get()->DynamicGrantedTags.AddTag(Tag);
		}
	}
}

void UBaseGEC::GetSkillProcEffects(UAbilitySystemComponent* InstigatorASC, UGameplayAbility* InstigatorSkill, AActor* InEffectCauser, const FGameplayEffectContextHandle& CurrentContext, TArray<FGameplayEffectSpecHandle>& OutSpecs, bool bDefaultConsume, const FGameplayEffectSpec* ParentSpec)
{
	if (!IsValid(InstigatorASC) || !IsValid(InstigatorSkill))
	{
		return;
	}

	// 1. 버프 태그
	static const FGameplayTag SkillProcTag = FGameplayTag::RequestGameplayTag(FName("Skill.Data.Augments"));

	// 2. 버프 검색
	FGameplayEffectQuery Query = FGameplayEffectQuery::MakeQuery_MatchAnyOwningTags(FGameplayTagContainer(SkillProcTag));
	TArray<FActiveGameplayEffectHandle> FoundHandles = InstigatorASC->GetActiveEffects(Query);

	if (FoundHandles.Num() > 0)
	{
		const FActiveGameplayEffectHandle& Handle = FoundHandles[0];
		const FActiveGameplayEffect* ActiveGE = InstigatorASC->GetActiveGameplayEffect(Handle);
		
		if (ActiveGE)
		{
			bool bShouldConsume = bDefaultConsume;

			// 3. AdditionalEffectGEC 컴포넌트 추출
			const UAdditionalEffectGEC* ExtraGEC = ActiveGE->Spec.Def->FindComponent<UAdditionalEffectGEC>();
			if (IsValid(ExtraGEC))
			{
				// 4. 추가 효과들로부터 스펙 생성
				for (const TSubclassOf<UBaseGameplayEffect>& EffectClass : ExtraGEC->Bonus)
				{
					if (IsValid(EffectClass))
					{
						FGameplayEffectSpecHandle NewSpecHandle = InstigatorASC->MakeOutgoingSpec(EffectClass, InstigatorSkill->GetAbilityLevel(), CurrentContext);
						if (ParentSpec)
						{
							InheritHitTags(*ParentSpec, NewSpecHandle);
						}
						OutSpecs.Add(NewSpecHandle);
					}
				}

				// 5. 서버 설정(Config)에서 소모 여부 결정
				bShouldConsume = ExtraGEC->bConsumeBuff;
			}

			// 6. 버프 소모 처리
			if (bShouldConsume)
			{
				InstigatorASC->RemoveActiveGameplayEffect(Handle);
			}
		}
	}
}

void UBaseGEC::CollectNiagaraPaths(TArray<FSoftObjectPath>& OutPaths) const
{
}

