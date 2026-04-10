#pragma once

#include "CoreMinimal.h"
#include "GameplayEffectExecutionCalculation.h"
#include "BaseMonsterDamageExecCalc.generated.h"

UCLASS()
class PROJECTER_API UBaseMonsterDamageExecCalc : public UGameplayEffectExecutionCalculation
{
	GENERATED_BODY()
	
	UBaseMonsterDamageExecCalc();

	virtual void Execute_Implementation(
		const FGameplayEffectCustomExecutionParameters& ExecutionParams, 
		FGameplayEffectCustomExecutionOutput& OutExecutionOutput
	) const override;
};
