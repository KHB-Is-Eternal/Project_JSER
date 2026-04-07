// Fill out your copyright notice in the Description page of Project Settings.

#include "SkillSystem/GameplayEffectComponent/SummonPeriodicPoolGEC.h"
#include "SkillSystem/GameplayEffectComponent/SummonRangeGEC.h"
#include "SkillSystem/Actor/BaseRangeOverlapEffectActor/BaseRangeOverlapEffectActor.h"
#include "SkillSystem/Component/AreaPeriodicEffectComponent.h"
#include "SkillSystem/SkillNiagaraSpawnConfig.h"
#include "SkillSystem/SkillSoundSpawnConfig.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemGlobals.h"
#include "Components/SkeletalMeshComponent.h"

USummonPeriodicPoolGEC::USummonPeriodicPoolGEC()
{
}

FTransform USummonPeriodicPoolGEC::CalculateOriginTransform(const FGameplayEffectSpec& GESpec, const AActor* Instigator, const AActor* TargetActor) const
{
    if (!IsValid(Instigator))
    {
        return FTransform::Identity;
    }

    FTransform OriginTransform = FTransform::Identity;
    if (this->OriginType == ESummonOriginType::InstigatorBone)
    {
        // 시전자의 본 위치 사용
        if (const USkeletalMeshComponent* const Mesh = Instigator->FindComponentByClass<USkeletalMeshComponent>())
        {
            if (Mesh->DoesSocketExist(this->SummonBoneName))
            {
                OriginTransform.SetLocation(Mesh->GetSocketLocation(this->SummonBoneName));
                OriginTransform.SetRotation(Mesh->GetSocketRotation(this->SummonBoneName).Quaternion());
            }
        }

        // 본을 찾지 못했으면 액터 기본 트랜스폼 사용
        if (OriginTransform.GetLocation().IsZero())
        {
            OriginTransform = Instigator->GetActorTransform();
        }
    }
    
    // 만약 Context 타입이거나 위에서 위치를 찾지 못한 경우 부모의 방식(SummonRangeGEC) 사용
    if (OriginTransform.GetLocation().IsZero())
    {
        return Super::CalculateOriginTransform(GESpec, Instigator, TargetActor);
    }
    
    return OriginTransform;
}

void USummonPeriodicPoolGEC::InitializeRangeActor(ABaseRangeOverlapEffectActor* RangeActor, AActor* Instigator, const FGameplayEffectContextHandle& Context, const FGameplayCueParameters& HitTargetVfxCueParameters, const FGameplayCueParameters& HitTargetSoundCueParameters) const
{
    // 부모의 초기화 로직 (Effect Specs 설정 등) 실행
    Super::InitializeRangeActor(RangeActor, Instigator, Context, HitTargetVfxCueParameters, HitTargetSoundCueParameters);
    
    if (IsValid(RangeActor))
    {
        // 1. AreaPeriodicEffectComponent 동적 생성
        UAreaPeriodicEffectComponent* PeriodicComp = NewObject<UAreaPeriodicEffectComponent>(RangeActor, UAreaPeriodicEffectComponent::StaticClass(), TEXT("DynamicAreaPeriodicEffect"));
        if (IsValid(PeriodicComp))
        {
            PeriodicComp->CreationMethod = EComponentCreationMethod::Instance;
			PeriodicComp->SetIsReplicated(true);

            // 2. 컴포넌트 등록 및 액터 할당
            PeriodicComp->RegisterComponent();
            RangeActor->AddInstanceComponent(PeriodicComp);
            RangeActor->SetAreaPeriodicComponent(PeriodicComp);

            // 3. 주기적 효과 설정 (실행은 액터의 BeginPlay에서 담당)
            PeriodicComp->SetupPeriodicEffect(this->Period, this->bApplyImmediately);

            // 4. 주기적 큐 설정
            FGameplayCueParameters PeriodicVfxParams;
            if (IsValid(this->PeriodicVfx.Get()))
            {
                PeriodicVfxParams.OriginalTag = this->PeriodicVfx->CueTag;
                PeriodicVfxParams.SourceObject = this->PeriodicVfx.Get();
            }

            FGameplayCueParameters PeriodicSoundParams;
            if (IsValid(this->PeriodicSound.Get()))
            {
                PeriodicSoundParams.OriginalTag = this->PeriodicSound->CueTag;
                PeriodicSoundParams.SourceObject = this->PeriodicSound.Get();
            }

            RangeActor->InitializePeriodicCues(PeriodicVfxParams, PeriodicSoundParams);
        }
    }
}
