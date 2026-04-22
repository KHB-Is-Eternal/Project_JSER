// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameplayEffectTypes.h"
#include "GameplayPrediction.h"
#include "ProjectERGameplayEffectContext.generated.h"

/**
 * ProjectER 전용 커스텀 GameplayEffectContext
 * 클라이언트 단에서 스킬을 시전한 시간을 서버로 전달하여
 * 발사체나 소환물의 위치 오차(렉 보상)를 처리하기 위해 사용합니다.
 */
USTRUCT(BlueprintType)
struct PROJECTER_API FProjectERGameplayEffectContext : public FGameplayEffectContext
{
	GENERATED_USTRUCT_BODY()

	FProjectERGameplayEffectContext()
		: ClientActivationTime(0.0f)
	{
	}

	virtual ~FProjectERGameplayEffectContext() {}



	/** 클라이언트가 스킬을 시전한 시점의 정확한 서버 시간 (GetServerWorldTime() 기준) */
	UPROPERTY(NotReplicated)
	float ClientActivationTime;

	/**
	 * 실제 구조체를 반환합니다. (직렬화 및 생성에 사용됨)
	 */
	virtual UScriptStruct* GetScriptStruct() const override
	{
		return FProjectERGameplayEffectContext::StaticStruct();
	}

	/**
	 * 데이터를 다른 컨텍스트로 복사합니다.
	 */
	virtual void Duplicate(FProjectERGameplayEffectContext* OutContext) const
	{
		if (OutContext)
		{
			OutContext->ClientActivationTime = ClientActivationTime;
		}
	}

	virtual FGameplayEffectContext* Duplicate() const override
	{
		FProjectERGameplayEffectContext* NewContext = new FProjectERGameplayEffectContext();
		*NewContext = *this; // 모든 필드 복사
		
		// [GCN Registry용] 데이터 명시적 복사 확인
		NewContext->ClientActivationTime = this->ClientActivationTime;
		
		return NewContext;
	}

	/**
	 * 네트워크 전송을 위해 직렬화 정보를 구성합니다.
	 */
	virtual bool NetSerialize(FArchive& Ar, class UPackageMap* Map, bool& bOutSuccess) override;

protected:

};

/** FProjectERGameplayEffectContext Handle을 보다 안전하고 편리하게 다루기 위한 헬퍼 함수들 */
namespace ProjectERContextUtils
{
	static FORCEINLINE const FProjectERGameplayEffectContext* GetProjectERContext(const FGameplayEffectContextHandle& Handle)
	{
		return static_cast<const FProjectERGameplayEffectContext*>(Handle.Get());
	}

	static FORCEINLINE FProjectERGameplayEffectContext* GetMutableProjectERContext(const FGameplayEffectContextHandle& Handle)
	{
		return static_cast<FProjectERGameplayEffectContext*>(const_cast<FGameplayEffectContext*>(Handle.Get()));
	}
}

/** FProjectERGameplayEffectContext에 대한 기능 허용 트레이츠 (상속 기반 직렬화 등 활성화) */
template<>
struct TStructOpsTypeTraits<FProjectERGameplayEffectContext> : public TStructOpsTypeTraitsBase2<FProjectERGameplayEffectContext>
{
	enum
	{
		WithNetSerializer = true,
		WithCopy = true
	};
};
