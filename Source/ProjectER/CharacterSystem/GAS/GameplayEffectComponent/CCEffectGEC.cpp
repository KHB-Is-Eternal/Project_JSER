// Fill out your copyright notice in the Description page of Project Settings.


#include "CharacterSystem/GAS/GameplayEffectComponent/CCEffectGEC.h"

#include "AbilitySystemComponent.h"
#include "GameplayEffect.h"
#include "CharacterSystem/Character/BaseCharacter.h"
#include "CharacterSystem/GAS/AttributeSet/BaseAttributeSet.h"
#include "CharacterSystem/GameplayTags/GameplayTags.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"

UCCEffectGEC::UCCEffectGEC()
{
    
}

void UCCEffectGEC::OnGameplayEffectApplied(FActiveGameplayEffectsContainer& ActiveGEContainer, FGameplayEffectSpec& GESpec, FPredictionKey& PredictionKey) const
{
    Super::OnGameplayEffectApplied(ActiveGEContainer, GESpec, PredictionKey);
    
    UAbilitySystemComponent* const TargetASC = ActiveGEContainer.Owner;
    if (!IsValid(TargetASC))
    {
        return;
    }
    
    // CC 면역 체크 (ApplicationTagRequirements와 이중 확인)
    if (TargetASC->HasMatchingGameplayTag(ProjectER::State::Buff::Immune::CC))
    {
        return;
    }
    
    // Slow 중첩 정책: 가장 강한 Slow만 유지
    if (bIsSlowEffect)
    {
        if (ShouldBlockWeakerSlow(TargetASC, GESpec))
        {
            // 이번 Slow가 기존보다 약하면 → 아무것도 안 함
            // (GE 자체는 이미 적용되었지만, 기존 강한 Slow가 있으므로
            //  MoveSpeed Modifier가 곱연산되어 최종적으로 가장 강한 것만 실질적으로 적용됨)
            // 그러나 "가장 강한 것만 적용" 정책이므로 약한 것을 제거해야 함
            return;
        }
        // 이번 Slow가 더 강하면 → 기존 약한 Slow들 제거
        RemoveWeakerSlowEffects(TargetASC, GESpec);
    }
    
    // Tenacity 기반 Duration 보정
    if (bAffectedByTenacity)
    {
        AdjustDurationByTenacity(GESpec, TargetASC);
    }
    
    // CC 부가 동작 실행
    AActor* const TargetActor = TargetASC->GetAvatarActor();
    if (IsValid(TargetActor))
    {
        ExecuteCCBehavior(TargetActor);
    }
}

void UCCEffectGEC::AdjustDurationByTenacity(FGameplayEffectSpec& GESpec, const UAbilitySystemComponent* TargetASC) const
{
    if (!IsValid(TargetASC))
    {
        return;
    }
    
    const float Tenacity = TargetASC->GetNumericAttribute(UBaseAttributeSet::GetTenacityAttribute());
    if (Tenacity <= 0.0f)
    {
        return;
    }
    
    // 공식: 최종 Duration = 기본 Duration × (100 / (100 + Tenacity))
    const float OriginalDuration = GESpec.GetDuration();
    const float ReductionFactor = 100.0f / (100.0f + Tenacity);
    const float AdjustedDuration = OriginalDuration * ReductionFactor;
    
    // 최소 Duration 보장 (0.1초)
    GESpec.SetDuration(FMath::Max(AdjustedDuration, 0.1f), false);
}

void UCCEffectGEC::ExecuteCCBehavior(AActor* TargetActor) const
{
    check(TargetActor);
    
    // 이동 중지
    if (bStopMovement)
    {
        ABaseCharacter* const TargetCharacter = Cast<ABaseCharacter>(TargetActor);
        if (TargetCharacter != nullptr)
        {
            TargetCharacter->StopMove();
        }
    }
    
    // 현재 어빌리티 캔슬 (Hard CC)
    if (bCancelCurrentAbilities)
    {
        ABaseCharacter* const TargetCharacter = Cast<ABaseCharacter>(TargetActor);
        UAbilitySystemComponent* const ASC = (TargetCharacter != nullptr) ? TargetCharacter->GetAbilitySystemComponent() : nullptr;
        if (IsValid(ASC))
        {
            FGameplayTagContainer CancelTags;
            CancelTags.AddTag(FGameplayTag::RequestGameplayTag(FName("Skill.Animation.Active")));
            CancelTags.AddTag(FGameplayTag::RequestGameplayTag(FName("Skill.Animation.Casting")));
            ASC->CancelAbilities(&CancelTags);
        }
    }
    
    // 에어본: LaunchCharacter
    if (AirborneHeight > 0.0f)
    {
        ACharacter* const Character = Cast<ACharacter>(TargetActor);
        if (Character != nullptr)
        {
            Character->LaunchCharacter(
                FVector(0.0f, 0.0f, AirborneHeight),
                false,  // bXYOverride
                true    // bZOverride
            );
        }
    }
}

bool UCCEffectGEC::ShouldBlockWeakerSlow(const UAbilitySystemComponent* TargetASC, const FGameplayEffectSpec& IncomingSpec) const
{
    if (!IsValid(TargetASC))
    {
        return false;
    }
    
    // 현재 적용 중인 Slow GE 검색
    FGameplayTagContainer SlowTag;
    SlowTag.AddTag(ProjectER::State::Debuff::Soft::Slow);
    FGameplayEffectQuery SlowQuery = FGameplayEffectQuery::MakeQuery_MatchAnyOwningTags(SlowTag);
    TArray<FActiveGameplayEffectHandle> ActiveSlows = TargetASC->GetActiveEffects(SlowQuery);
    
    if (ActiveSlows.Num() == 0)
    {
        return false; // 기존 Slow 없음 → 차단하지 않음
    }
    
    // 새로 들어오는 Slow의 MoveSpeed Modifier 값 추출
    float IncomingSlowMagnitude = 1.0f;
    
    if (IncomingSpec.Def != nullptr)
    {
        for (const FGameplayModifierInfo& ModInfo : IncomingSpec.Def->Modifiers)
        {
            if (ModInfo.Attribute == UBaseAttributeSet::GetMoveSpeedAttribute())
            {
                float EvaluatedMagnitude = 0.0f;
                ModInfo.ModifierMagnitude.AttemptCalculateMagnitude(IncomingSpec, EvaluatedMagnitude);
                IncomingSlowMagnitude = EvaluatedMagnitude;
                break;
            }
        }
    }
    
    // 기존 Slow들과 비교 - 기존이 더 강하면(값이 더 작으면) 차단
    for (const FActiveGameplayEffectHandle& Handle : ActiveSlows)
    {
        const FActiveGameplayEffect* const ActiveGE = TargetASC->GetActiveGameplayEffect(Handle);
        if (ActiveGE == nullptr)
        {
            continue;
        }
        for (const FGameplayModifierInfo& ModInfo : ActiveGE->Spec.Def->Modifiers)
        {
            if (ModInfo.Attribute == UBaseAttributeSet::GetMoveSpeedAttribute())
            {
                float ExistingMagnitude = 0.0f;
                ModInfo.ModifierMagnitude.AttemptCalculateMagnitude(ActiveGE->Spec, ExistingMagnitude);
                // Multiply 기준: 0.5(50%감소)가 0.7(30%감소)보다 강함
                // 기존이 더 강하면(값이 더 작으면) 새 Slow를 차단
                if (ExistingMagnitude <= IncomingSlowMagnitude)
                {
                    return true; // 기존이 더 강함 → 새 것 차단
                }
            }
        }
    }
    return false; // 새 Slow가 더 강함 → 차단하지 않음
}

void UCCEffectGEC::RemoveWeakerSlowEffects(UAbilitySystemComponent* TargetASC, const FGameplayEffectSpec& IncomingSpec) const
{
    if (!IsValid(TargetASC))
    {
        return;
    }
    
    // 새 Slow의 Magnitude
    float IncomingSlowMagnitude = 1.0f;
    if (IncomingSpec.Def != nullptr)
    {
        for (const FGameplayModifierInfo& ModInfo : IncomingSpec.Def->Modifiers)
        {
            if (ModInfo.Attribute == UBaseAttributeSet::GetMoveSpeedAttribute())
            {
                ModInfo.ModifierMagnitude.AttemptCalculateMagnitude(IncomingSpec, IncomingSlowMagnitude);
                break;
            }
        }
    }
    
    // 기존 Slow 중 더 약한 것들 제거
    FGameplayTagContainer SlowTag;
    SlowTag.AddTag(ProjectER::State::Debuff::Soft::Slow);
    FGameplayEffectQuery SlowQuery = FGameplayEffectQuery::MakeQuery_MatchAnyOwningTags(SlowTag);
    TArray<FActiveGameplayEffectHandle> ActiveSlows = TargetASC->GetActiveEffects(SlowQuery);
    
    for (const FActiveGameplayEffectHandle& Handle : ActiveSlows)
    {
        const FActiveGameplayEffect* const ActiveGE = TargetASC->GetActiveGameplayEffect(Handle);
        if (ActiveGE == nullptr)
        {
            continue;
        }
        
        for (const FGameplayModifierInfo& ModInfo : ActiveGE->Spec.Def->Modifiers)
        {
            if (ModInfo.Attribute == UBaseAttributeSet::GetMoveSpeedAttribute())
            {
                float ExistingMagnitude = 0.0f;
                ModInfo.ModifierMagnitude.AttemptCalculateMagnitude(ActiveGE->Spec, ExistingMagnitude);
                
                // 기존이 더 약하면(값이 더 크면) 제거
                if (ExistingMagnitude > IncomingSlowMagnitude)
                {
                    TargetASC->RemoveActiveGameplayEffect(Handle);
                }
                
                break;
            }
        }
    }
}