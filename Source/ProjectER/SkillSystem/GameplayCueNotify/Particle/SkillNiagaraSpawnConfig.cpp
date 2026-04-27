// Fill out your copyright notice in the Description page of Project Settings.

#include "SkillSystem/GameplayCueNotify/Particle/SkillNiagaraSpawnConfig.h"
#include "NiagaraSystem.h"
#include "NiagaraTypes.h"
#include "NiagaraDataInterface.h"

#if WITH_EDITOR
void USkillNiagaraSpawnConfig::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);

	const FName PropertyName = (PropertyChangedEvent.Property != nullptr) ? PropertyChangedEvent.Property->GetFName() : NAME_None;

	if (PropertyName == GET_MEMBER_NAME_CHECKED(USkillNiagaraSpawnConfig, NiagaraSystem))
	{
		RefreshParameters();
	}
}

void USkillNiagaraSpawnConfig::RefreshParameters()
{
	UNiagaraSystem* NS = NiagaraSystem.LoadSynchronous();
	if (!NS)
	{
		FloatParameters.Empty();
		VectorParameters.Empty();
		ColorParameters.Empty();
		BoolParameters.Empty();
		IntParameters.Empty();
		ObjectParameters.Empty();
		DataInterfaceParameters.Empty();
		return;
	}

	const auto& Store = NS->GetExposedParameters();
	TArray<FNiagaraVariable> Variables;
	Store.GetParameters(Variables);

	TSet<FName> NewFloatNames;
	TSet<FName> NewVectorNames;
	TSet<FName> NewColorNames;
	TSet<FName> NewBoolNames;
	TSet<FName> NewIntNames;
	TSet<FName> NewObjectNames;
	TSet<FName> NewDataInterfaceNames;

	for (const FNiagaraVariable& Var : Variables)
	{
		FName VarName = Var.GetName();
		const FNiagaraTypeDefinition& Type = Var.GetType();
		const uint8* Data = Store.GetParameterData(Var);

		if (Type == FNiagaraTypeDefinition::GetFloatDef())
		{
			NewFloatNames.Add(VarName);
			if (!FloatParameters.Contains(VarName))
			{
				float DefaultVal = Data ? *reinterpret_cast<const float*>(Data) : 0.0f;
				FloatParameters.Add(VarName, DefaultVal);
			}
		}
		else if (Type == FNiagaraTypeDefinition::GetVec3Def())
		{
			NewVectorNames.Add(VarName);
			if (!VectorParameters.Contains(VarName))
			{
				FVector DefaultVal = Data ? *reinterpret_cast<const FVector*>(Data) : FVector::ZeroVector;
				VectorParameters.Add(VarName, DefaultVal);
			}
		}
		else if (Type == FNiagaraTypeDefinition::GetColorDef())
		{
			NewColorNames.Add(VarName);
			if (!ColorParameters.Contains(VarName))
			{
				FLinearColor DefaultVal = Data ? *reinterpret_cast<const FLinearColor*>(Data) : FLinearColor::White;
				ColorParameters.Add(VarName, DefaultVal);
			}
		}
		else if (Type == FNiagaraTypeDefinition::GetBoolDef())
		{
			NewBoolNames.Add(VarName);
			if (!BoolParameters.Contains(VarName))
			{
				bool DefaultVal = Data ? (*reinterpret_cast<const int32*>(Data) != 0) : false;
				BoolParameters.Add(VarName, DefaultVal);
			}
		}
		else if (Type == FNiagaraTypeDefinition::GetIntDef())
		{
			NewIntNames.Add(VarName);
			if (!IntParameters.Contains(VarName))
			{
				int32 DefaultVal = Data ? *reinterpret_cast<const int32*>(Data) : 0;
				IntParameters.Add(VarName, DefaultVal);
			}
		}
		else if (Type.IsUObject() || Type.GetClass() != nullptr)
		{
			// Texture, StaticMesh, DataInterface 등 모든 오브젝트 타입 처리
			if (Type.GetClass() && Type.GetClass()->IsChildOf(UNiagaraDataInterface::StaticClass()))
			{
				NewDataInterfaceNames.Add(VarName);
				if (!DataInterfaceParameters.Contains(VarName))
				{
					UNiagaraDataInterface* DefaultDI = Store.GetDataInterface(Var);
					if (DefaultDI)
					{
						// 원본 에셋이 오염되지 않도록 Data Interface를 복제하여 Data Asset이 소유하게 함 (Instanced)
						UNiagaraDataInterface* DuplicatedDI = DuplicateObject<UNiagaraDataInterface>(DefaultDI, this);
						DataInterfaceParameters.Add(VarName, DuplicatedDI);
					}
					else
					{
						DataInterfaceParameters.Add(VarName, nullptr);
					}
				}
			}
			else
			{
				NewObjectNames.Add(VarName);
				if (!ObjectParameters.Contains(VarName))
				{
					UObject* DefaultVal = Store.GetUObject(Var);
					ObjectParameters.Add(VarName, TSoftObjectPtr<UObject>(DefaultVal));
				}
			}
		}
	}

	// Remove parameters that no longer exist in the system (람다 함수로 정리)
	auto CleanMap = [](auto& Map, const TSet<FName>& NewNames)
	{
		TArray<FName> KeysToRemove;
		for (const auto& Pair : Map)
		{
			if (!NewNames.Contains(Pair.Key)) { KeysToRemove.Add(Pair.Key); }
		}
		for (FName Key : KeysToRemove) { Map.Remove(Key); }
	};

	CleanMap(FloatParameters, NewFloatNames);
	CleanMap(VectorParameters, NewVectorNames);
	CleanMap(ColorParameters, NewColorNames);
	CleanMap(BoolParameters, NewBoolNames);
	CleanMap(IntParameters, NewIntNames);
	CleanMap(ObjectParameters, NewObjectNames);
	CleanMap(DataInterfaceParameters, NewDataInterfaceNames);
}
#endif

FSkillNiagaraSpawnSettings USkillNiagaraSpawnConfig::ToSettings() const
{
	FSkillNiagaraSpawnSettings S;
	S.NiagaraSystem = NiagaraSystem;
	S.bAttachToSource = bAttachToSource;
	S.SocketOrBoneName = SocketOrBoneName;
	S.bUseSourceRotationForLocationOffset = bUseSourceRotationForLocationOffset;
	S.LocationOffset = LocationOffset;
	S.RotationMode = RotationMode;
	S.RotationOffset = RotationOffset;
	S.CueTag = CueTag;
	S.FloatParameters = FloatParameters;
	S.VectorParameters = VectorParameters;
	S.ColorParameters = ColorParameters;
	S.BoolParameters = BoolParameters;
	S.IntParameters = IntParameters;

	// SoftPtr -> HardPtr 변환 (Settings 저장용)
	for (const auto& Pair : ObjectParameters)
	{
		if (UObject* LoadedObj = Pair.Value.LoadSynchronous())
		{
			S.ObjectParameters.Add(Pair.Key, LoadedObj);
		}
	}

	for (const auto& Pair : DataInterfaceParameters)
	{
		if (Pair.Value)
		{
			S.ObjectParameters.Add(Pair.Key, Pair.Value.Get());
		}
	}

	return S;
}
