#include "SkillSystem/GameplayCueNotify/AGCN_SummonedActor.h"
#include "SkillSystem/GameplayCueNotify/GCN_SummonedRegistrySubsystem.h"
#include "SkillSystem/GameplayEffectComponent/SummonRangeBaseGEC.h"
#include "SkillSystem/GameplayEffectComponent/LaunchHomingMissile.h"
#include "SkillSystem/GAS/ProjectERGameplayEffectContext.h"
#include "GameplayPrediction.h"

AGCN_SummonedActor::AGCN_SummonedActor()
{
	// GCN 액터는 기본적으로 클라이언트에서 실행되므로 리플리케이션은 끄는 것이 일반적입니다.
	bReplicates = false;
}

bool AGCN_SummonedActor::OnExecute_Implementation(AActor* MyTarget, const FGameplayCueParameters& Parameters)
{
	HandleSummonedVfx(Parameters);
	return Super::OnExecute_Implementation(MyTarget, Parameters);
}

bool AGCN_SummonedActor::WhileActive_Implementation(AActor* MyTarget, const FGameplayCueParameters& Parameters)
{
	HandleSummonedVfx(Parameters);
	return Super::WhileActive_Implementation(MyTarget, Parameters);
}

bool AGCN_SummonedActor::OnRemove_Implementation(AActor* MyTarget, const FGameplayCueParameters& Parameters)
{
	Super::OnRemove_Implementation( MyTarget, Parameters);
	
	// 제거 시 레지스트리에서도 명시적으로 정리
	if (UWorld* World = GetWorld())
	{
		if (UGCN_SummonedRegistrySubsystem* Registry = World->GetSubsystem<UGCN_SummonedRegistrySubsystem>())
		{
			// 커스텀 컨텍스트로부터 시전 시간 추출
			const FProjectERGameplayEffectContext* Context = static_cast<const FProjectERGameplayEffectContext*>(Parameters.EffectContext.Get());

			if (Context && Context->ClientActivationTime > 0.0f)
			{
				Registry->GetAndUnregisterVfxActor(Parameters.Instigator.Get(), Context->ClientActivationTime);
			}
		}
	}
	return true;
}

void AGCN_SummonedActor::HandleSummonedVfx(const FGameplayCueParameters& Parameters)
{
	InitializeFromGEC(Parameters.SourceObject.Get());

	// 커스텀 컨텍스트로부터 시전 시간 추출 시도
	const FProjectERGameplayEffectContext* Context = static_cast<const FProjectERGameplayEffectContext*>(Parameters.EffectContext.Get());

	if (Context && Context->ClientActivationTime > 0.0f)
	{
		if (UWorld* World = GetWorld())
		{
			if (UGCN_SummonedRegistrySubsystem* Registry = World->GetSubsystem<UGCN_SummonedRegistrySubsystem>())
			{
				// 시전자 + 시전 시간을 키로 사용하여 비주얼 액터 등록
				Registry->RegisterVfxActor(Parameters.Instigator.Get(), Context->ClientActivationTime, this);
			}
		}
	}
}

void AGCN_SummonedActor::InitializeFromGEC(const UObject* SourceObject)
{
	if (!SourceObject) return;
	
	CachedSourceObject = const_cast<UObject*>(SourceObject);

	// GEC로부터 수명(LifeSpan) 설정 등을 직접 읽어옴
	if (const USummonRangeBaseGEC* RangeGEC = Cast<USummonRangeBaseGEC>(SourceObject))
	{
		SetLifeSpan(RangeGEC->LifeSpan);
	}
	else if (const ULaunchHomingMissile* MissileGEC = Cast<ULaunchHomingMissile>(SourceObject))
	{
		SetLifeSpan(MissileGEC->LifeSpan);
	}
}
