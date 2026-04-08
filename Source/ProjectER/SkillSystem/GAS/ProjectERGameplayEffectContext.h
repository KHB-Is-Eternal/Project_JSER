// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameplayEffectTypes.h"
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
		*NewContext = *this;
		if (GetInstigator())
		{
			NewContext->AddInstigator(GetInstigator(), GetEffectCauser());
		}
		if (GetHitResult())
		{
			NewContext->AddHitResult(*GetHitResult(), true);
		}
		NewContext->AddOrigin(GetOrigin());
		NewContext->SourceObject = SourceObject;
		NewContext->AbilityCDO = AbilityCDO;
		NewContext->AbilityInstanceNotReplicated = AbilityInstanceNotReplicated;
		NewContext->AbilityLevel = AbilityLevel;
		NewContext->bHasWorldOrigin = bHasWorldOrigin;
		NewContext->Duplicate(NewContext);
		return NewContext;
	}

	/**
	 * 네트워크 전송을 위해 직렬화 정보를 구성합니다.
	 */
	virtual bool NetSerialize(FArchive& Ar, class UPackageMap* Map, bool& bOutSuccess) override;
};

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
