#pragma once

#include "CoreMinimal.h"
#include "AbilitySystemComponent.h"
#include "ProjectERASC.generated.h"

UCLASS()
class PROJECTER_API UProjectERASC : public UAbilitySystemComponent
{
	GENERATED_BODY()

public:
	/**
	 * SourceObject가 일치하는 특정 GameplayCue 엔트리만 선택적으로 제거합니다.
	 * 엔진 기본 RemoveGameplayCue는 같은 태그의 모든 엔트리를 삭제하므로,
	 * 동일 태그를 공유하는 다른 이펙트가 영향받지 않도록 이 함수를 사용합니다.
	 */
	void RemoveGameplayCueBySource(const FGameplayTag& GameplayCueTag, const UObject* SourceObject);
};
