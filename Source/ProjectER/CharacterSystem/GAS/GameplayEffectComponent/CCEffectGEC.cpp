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
    
    // CC 면역 체크 (이중 안전장치 — 메인 방어선은 GE BP의 ApplicationTagRequirements)
    // GE BP에서 MustNotHaveTags에 State.Buff.Immune.CC를 설정하면 적용 자체가 차단됨
    // 여기까지 도달했다면 ApplicationTagRequirements를 우회한 예외 상황
    if (TargetASC->HasMatchingGameplayTag(ProjectER::State::Buff::Immune::CC))
    {
        UE_LOG(LogTemp, Warning, TEXT("UCCEffectGEC: CC면역 대상에 CC GE가 적용됨 — GE BP의 ApplicationTagRequirements 설정을 확인하세요."));
        return;
    }
    
    // Slow 중첩 정책: 가장 강한 Slow만 유지
    if (bIsSlowEffect)
    {
        if (ShouldBlockWeakerSlow(TargetASC, GESpec))
        {
            // 이번 Slow가 기존보다 약하면 → 아무 부가 동작도 실행하지 않음
            return;
        }
        // 이번 Slow가 더 강하면 → 기존 약한 Slow들 제거
        RemoveWeakerSlowEffects(TargetASC, GESpec);
    }
    
    AActor* const TargetActor = TargetASC->GetAvatarActor();
    
    // Duration 보정 (Tenacity + Diminishing Returns)
    AdjustDuration(GESpec, TargetASC, TargetActor);
    
    // CC 부가 동작 실행
    if (IsValid(TargetActor))
    {
        ExecuteCCBehavior(TargetActor, GESpec);
    }
}

void UCCEffectGEC::AdjustDuration(FGameplayEffectSpec& GESpec, const UAbilitySystemComponent* TargetASC, AActor* TargetActor) const
{
    if (!IsValid(TargetASC))
    {
        return;
    }
    
    float FinalDuration = GESpec.GetDuration();
    
    // 1. Tenacity 보정
    if (bAffectedByTenacity)
    {
        const float Tenacity = TargetASC->GetNumericAttribute(UBaseAttributeSet::GetTenacityAttribute());
        if (Tenacity > 0.0f)
        {
            // 공식: Duration × (100 / (100 + Tenacity))
            const float TenacityFactor = 100.0f / (100.0f + Tenacity);
            FinalDuration *= TenacityFactor;
        }
    }
    
    // 2. Diminishing Returns 보정
    if (bApplyDiminishingReturns && CCTypeTag.IsValid() && IsValid(TargetActor))
    {
        ABaseCharacter* const TargetCharacter = Cast<ABaseCharacter>(TargetActor);
        if (TargetCharacter != nullptr)
        {
            const float DRFactor = TargetCharacter->GetCCDiminishingFactor(CCTypeTag);
            FinalDuration *= DRFactor;
            
            // DR 적용 기록
            TargetCharacter->RecordCCApplication(CCTypeTag);
        }
    }
    
    // 최소 Duration 보장 (0.1초)
    GESpec.SetDuration(FMath::Max(FinalDuration, 0.1f), false);
}

void UCCEffectGEC::ExecuteCCBehavior(AActor* TargetActor, FGameplayEffectSpec& GESpec) const
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
    
    // 에어본: 높이에서 발사 속도를 역산하여 LaunchCharacter
    if (DesiredAirborneHeight > 0.0f)
    {
        ACharacter* const Character = Cast<ACharacter>(TargetActor);
        if (Character != nullptr)
        {
            const float Gravity = FMath::Abs(Character->GetCharacterMovement()->GetGravityZ());
            
            // v₀ = √(2gh) → 해당 높이에 도달하는 초기 속도
            const float LaunchSpeed = FMath::Sqrt(2.0f * Gravity * DesiredAirborneHeight);
            
            // 체공 시간 = 2v₀/g → GE Duration을 물리 궤적에 맞게 덮어쓰기
            if (Gravity > 0.0f)
            {
                const float FlightTime = (2.0f * LaunchSpeed) / Gravity;
                GESpec.SetDuration(FMath::Max(FlightTime, 0.1f), false);
            }
            
            Character->LaunchCharacter(
                FVector(0.0f, 0.0f, LaunchSpeed),
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
    
    // 기존 Slow들과 비교 — 기존이 더 강하면(값이 더 작으면) 차단
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
    
    // 기존 Slow 중 더 약한 것들 수집
    FGameplayTagContainer SlowTag;
    SlowTag.AddTag(ProjectER::State::Debuff::Soft::Slow);
    FGameplayEffectQuery SlowQuery = FGameplayEffectQuery::MakeQuery_MatchAnyOwningTags(SlowTag);
    TArray<FActiveGameplayEffectHandle> ActiveSlows = TargetASC->GetActiveEffects(SlowQuery);
    
    // 제거 대상을 먼저 수집 (순회 중 컨테이너 수정 방지)
    TArray<FActiveGameplayEffectHandle> ToRemove;
    
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
                
                // 기존이 더 약하면(값이 더 크면) 제거 대상에 추가
                if (ExistingMagnitude > IncomingSlowMagnitude)
                {
                    ToRemove.Add(Handle);
                }
                
                break;
            }
        }
    }
    
    // 수집 완료 후 일괄 제거
    for (const FActiveGameplayEffectHandle& Handle : ToRemove)
    {
        TargetASC->RemoveActiveGameplayEffect(Handle);
    }
}