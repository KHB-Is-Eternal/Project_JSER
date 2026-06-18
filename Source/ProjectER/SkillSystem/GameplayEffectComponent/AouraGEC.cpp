// Fill out your copyright notice in the Description page of Project Settings.


#include "SkillSystem/GameplayEffectComponent/AouraGEC.h"

#include "Abilities/GameplayAbilityTypes.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "SkillSystem/Actor/BaseRangeOverlapEffectActor/BaseRangeOverlapEffectActor.h"
#include "SkillSystem/Component/AreaPeriodicEffectComponent.h"


UAouraGEC::UAouraGEC()
{
	// 오라 형태이므로 기본적으로 여러 번 타격 가능해야 함
	this->bHitOncePerTarget = false;
	this->LifeSpan = 5.0f; // 기본 지속시간
}

void UAouraGEC::InitializeRangeActor(ABaseRangeOverlapEffectActor* RangeActor, AActor* Instigator, const FGameplayEffectContextHandle& Context, const FGameplayCueParameters& HitTargetVfxCueParameters, const FGameplayCueParameters& HitTargetSoundCueParameters, const FGameplayEffectSpec& ParentSpec) const
{
	Super::InitializeRangeActor(RangeActor, Instigator, Context, HitTargetVfxCueParameters, HitTargetSoundCueParameters, ParentSpec);
	if (!IsValid(RangeActor) || !IsValid(Instigator))
	{
		return;
	}

	// 2. 캐릭터 본에 부착
	UAbilitySystemComponent* const CauserASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(Instigator);
	if (const USkeletalMeshComponent* const Mesh = Instigator->FindComponentByClass<USkeletalMeshComponent>())
	{
		if (Mesh->DoesSocketExist(this->BoneName))
		{
			// 소켓이 존재하면 해당 본에 부착하여 함께 이동하도록 함
			RangeActor->AttachToComponent(const_cast<USkeletalMeshComponent*>(Mesh), FAttachmentTransformRules::SnapToTargetNotIncludingScale, this->BoneName);

			// 부착 후 SnapToTarget에 의해 초기화된 상대 트랜스폼에 Config의 오프셋을 재적용
			RangeActor->SetActorRelativeLocation(this->LocationOffset);
			RangeActor->SetActorRelativeRotation(this->RotationOffset);
		}
	}

	// 3. 주기적 효과 컴포넌트(AreaPeriodicEffectComponent) 생성 및 설정
	UAreaPeriodicEffectComponent* PeriodicComp = NewObject<UAreaPeriodicEffectComponent>(RangeActor, UAreaPeriodicEffectComponent::StaticClass(), TEXT("AuraPeriodicComponent"));
	if (IsValid(PeriodicComp))
	{
		PeriodicComp->CreationMethod = EComponentCreationMethod::Instance;
		PeriodicComp->SetIsReplicated(true);
		
		// 컴포넌트 등록 및 액터 할당
		PeriodicComp->RegisterComponent();
		RangeActor->AddInstanceComponent(PeriodicComp);
		RangeActor->SetAreaPeriodicComponent(PeriodicComp);

		// 주기 및 즉시 적용 여부 설정
		PeriodicComp->SetupPeriodicEffect(this->Period, this->bApplyImmediately);
	}
}

FSkillTooltipData UAouraGEC::GetTooltipDescription(int32 Level, TSubclassOf<class USkillBase> AbilityClass) const
{
	FSkillTooltipData Data;
	Data.ShortDescription = FText::FromString(TEXT("범위를 생성합니다."));

	FString DetailStr = FString::Printf(TEXT("범위 : 자신 주변에 범위를 생성하여 %.1f초마다 주기적으로 효과를 적용합니다."), Period);
	FText EffectsText = FormatAppliedEffects(Applied, Level);
	if (!EffectsText.IsEmpty())
	{
		DetailStr += TEXT("\n") + EffectsText.ToString();
	}

	Data.DetailedDescription = FText::FromString(DetailStr);
	return Data;
}
