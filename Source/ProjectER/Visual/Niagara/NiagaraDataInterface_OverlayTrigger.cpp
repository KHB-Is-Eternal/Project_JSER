// Fill out your copyright notice in the Description page of Project Settings.

#include "Visual/Niagara/NiagaraDataInterface_OverlayTrigger.h"
#include "NiagaraComponent.h"
#include "NiagaraSystemInstance.h"
#include "Components/SkeletalMeshComponent.h"
#include "Materials/MaterialInterface.h"
#include "Engine/AssetManager.h"
#include "Async/Async.h"
#include "VectorVM.h"

struct FOverlayTriggerInstanceData
{
	TWeakObjectPtr<UNiagaraComponent> NiagaraComponent;
	TWeakObjectPtr<USkeletalMeshComponent> SkeletalMeshComponent;
	bool bIsOverlayApplied = false;
};

UNiagaraDataInterfaceOverlayTrigger::UNiagaraDataInterfaceOverlayTrigger()
{
}

void UNiagaraDataInterfaceOverlayTrigger::PostInitProperties()
{
	Super::PostInitProperties();

	if (HasAnyFlags(RF_ClassDefaultObject))
	{
		FNiagaraTypeRegistry::Register(FNiagaraTypeDefinition(GetClass()), true, false, false);
	}
}

#if WITH_EDITORONLY_DATA
void UNiagaraDataInterfaceOverlayTrigger::GetFunctionsInternal(TArray<FNiagaraFunctionSignature>& OutFunctions) const
{
	{
		FNiagaraFunctionSignature Sig;
		Sig.Name = FName("ApplyOverlayMaterial");
		Sig.bMemberFunction = true;
		Sig.bRequiresContext = false;
		Sig.bRequiresExecPin = true;
		Sig.Inputs.Add(FNiagaraVariable(FNiagaraTypeDefinition(UNiagaraDataInterfaceOverlayTrigger::StaticClass()), TEXT("DataInterface")));
		OutFunctions.Add(Sig);
	}

	{
		FNiagaraFunctionSignature Sig;
		Sig.Name = FName("ClearOverlayMaterial");
		Sig.bMemberFunction = true;
		Sig.bRequiresContext = false;
		Sig.bRequiresExecPin = true;
		Sig.Inputs.Add(FNiagaraVariable(FNiagaraTypeDefinition(UNiagaraDataInterfaceOverlayTrigger::StaticClass()), TEXT("DataInterface")));
		OutFunctions.Add(Sig);
	}
}
#endif

void UNiagaraDataInterfaceOverlayTrigger::GetVMExternalFunction(const FVMExternalFunctionBindingInfo& BindingInfo, void* InstanceData, FVMExternalFunction& OutFunc)
{
	if (BindingInfo.Name == FName("ApplyOverlayMaterial"))
	{
		OutFunc = FVMExternalFunction::CreateUObject(this, &UNiagaraDataInterfaceOverlayTrigger::VMApplyOverlayMaterial);
	}
	else if (BindingInfo.Name == FName("ClearOverlayMaterial"))
	{
		OutFunc = FVMExternalFunction::CreateUObject(this, &UNiagaraDataInterfaceOverlayTrigger::VMClearOverlayMaterial);
	}
}

bool UNiagaraDataInterfaceOverlayTrigger::Equals(const UNiagaraDataInterface* Other) const
{
	if (!Super::Equals(Other))
	{
		return false;
	}

	const UNiagaraDataInterfaceOverlayTrigger* OtherCast = Cast<const UNiagaraDataInterfaceOverlayTrigger>(Other);
	return OtherCast && OtherCast->OverlayMaterial == OverlayMaterial;
}

bool UNiagaraDataInterfaceOverlayTrigger::CopyToInternal(UNiagaraDataInterface* Destination) const
{
	if (!Super::CopyToInternal(Destination))
	{
		return false;
	}

	UNiagaraDataInterfaceOverlayTrigger* DestCast = Cast<UNiagaraDataInterfaceOverlayTrigger>(Destination);
	if (!DestCast)
	{
		return false;
	}

	DestCast->OverlayMaterial = OverlayMaterial;
	return true;
}

int32 UNiagaraDataInterfaceOverlayTrigger::PerInstanceDataSize() const
{
	return sizeof(FOverlayTriggerInstanceData);
}

bool UNiagaraDataInterfaceOverlayTrigger::InitPerInstanceData(void* PerInstanceData, FNiagaraSystemInstance* SystemInstance)
{
	FOverlayTriggerInstanceData* InstData = new (PerInstanceData) FOverlayTriggerInstanceData();
	if (SystemInstance)
	{
		UNiagaraComponent* Comp = Cast<UNiagaraComponent>(SystemInstance->GetAttachComponent());
		InstData->NiagaraComponent = Comp;
		if (Comp)
		{
			InstData->SkeletalMeshComponent = Cast<USkeletalMeshComponent>(Comp->GetAttachParent());

			// 컴포넌트 비활성화(Deactivate) 시 오버레이를 즉시 해제하기 위해 델리게이트 바인딩
			if (!Comp->OnComponentDeactivated.IsAlreadyBound(this, &UNiagaraDataInterfaceOverlayTrigger::OnNiagaraComponentDeactivated))
			{
				Comp->OnComponentDeactivated.AddDynamic(this, &UNiagaraDataInterfaceOverlayTrigger::OnNiagaraComponentDeactivated);
			}
		}
	}
	return true;
}

static void SetOverlayMaterial_GameThread(TWeakObjectPtr<USkeletalMeshComponent> SkeletalMesh, TSoftObjectPtr<UMaterialInterface> OverlayMat, bool bApply)
{
	if (!SkeletalMesh.IsValid())
	{
		return;
	}

	USkeletalMeshComponent* MeshComp = SkeletalMesh.Get();
	if (bApply)
	{
		if (OverlayMat.IsNull())
		{
			return;
		}

		if (OverlayMat.IsPending())
		{
			if (UAssetManager::IsValid())
			{
				FSoftObjectPath MaterialPath = OverlayMat.ToSoftObjectPath();
				UAssetManager::Get().GetStreamableManager().RequestAsyncLoad(MaterialPath, FStreamableDelegate::CreateWeakLambda(MeshComp, [MeshComp, OverlayMat]()
				{
					if (IsValid(MeshComp))
					{
						UMaterialInterface* LoadedMaterial = OverlayMat.Get();
						if (LoadedMaterial)
						{
							MeshComp->SetOverlayMaterial(LoadedMaterial);
						}
					}
				}));
			}
			else
			{
				UMaterialInterface* Material = OverlayMat.LoadSynchronous();
				if (Material)
				{
					MeshComp->SetOverlayMaterial(Material);
				}
			}
		}
		else
		{
			UMaterialInterface* Material = OverlayMat.Get();
			if (Material)
			{
				MeshComp->SetOverlayMaterial(Material);
			}
		}
	}
	else
	{
		MeshComp->SetOverlayMaterial(nullptr);
	}
}

void UNiagaraDataInterfaceOverlayTrigger::DestroyPerInstanceData(void* PerInstanceData, FNiagaraSystemInstance* SystemInstance)
{
	FOverlayTriggerInstanceData* InstData = static_cast<FOverlayTriggerInstanceData*>(PerInstanceData);
	if (InstData)
	{
		if (InstData->bIsOverlayApplied && InstData->SkeletalMeshComponent.IsValid())
		{
			TWeakObjectPtr<USkeletalMeshComponent> MeshPtr = InstData->SkeletalMeshComponent;
			auto ClearAction = [MeshPtr]()
			{
				SetOverlayMaterial_GameThread(MeshPtr, nullptr, false);
			};

			if (IsInGameThread())
			{
				ClearAction();
			}
			else
			{
				AsyncTask(ENamedThreads::GameThread, MoveTemp(ClearAction));
			}
		}
		InstData->~FOverlayTriggerInstanceData();
	}
}

void UNiagaraDataInterfaceOverlayTrigger::VMApplyOverlayMaterial(FVectorVMExternalFunctionContext& Context)
{
	VectorVM::FUserPtrHandler<FOverlayTriggerInstanceData> InstData(Context);

	if (InstData && !InstData->bIsOverlayApplied)
	{
		InstData->bIsOverlayApplied = true;

		// 런타임에 부모가 나중에 설정되는 경우에 대비한 보완책
		if (!InstData->SkeletalMeshComponent.IsValid() && InstData->NiagaraComponent.IsValid())
		{
			UNiagaraComponent* Comp = InstData->NiagaraComponent.Get();
			InstData->SkeletalMeshComponent = Cast<USkeletalMeshComponent>(Comp->GetAttachParent());
		}

		TWeakObjectPtr<USkeletalMeshComponent> MeshPtr = InstData->SkeletalMeshComponent;
		TSoftObjectPtr<UMaterialInterface> LocalOverlayMaterial = OverlayMaterial;

		if (IsInGameThread())
		{
			SetOverlayMaterial_GameThread(MeshPtr, LocalOverlayMaterial, true);
		}
		else
		{
			AsyncTask(ENamedThreads::GameThread, [MeshPtr, LocalOverlayMaterial]()
			{
				SetOverlayMaterial_GameThread(MeshPtr, LocalOverlayMaterial, true);
			});
		}
	}
}

void UNiagaraDataInterfaceOverlayTrigger::VMClearOverlayMaterial(FVectorVMExternalFunctionContext& Context)
{
	VectorVM::FUserPtrHandler<FOverlayTriggerInstanceData> InstData(Context);

	if (InstData && InstData->bIsOverlayApplied)
	{
		InstData->bIsOverlayApplied = false;

		TWeakObjectPtr<USkeletalMeshComponent> MeshPtr = InstData->SkeletalMeshComponent;

		if (IsInGameThread())
		{
			SetOverlayMaterial_GameThread(MeshPtr, nullptr, false);
		}
		else
		{
			AsyncTask(ENamedThreads::GameThread, [MeshPtr]()
			{
				SetOverlayMaterial_GameThread(MeshPtr, nullptr, false);
			});
		}
	}
}

void UNiagaraDataInterfaceOverlayTrigger::OnNiagaraComponentDeactivated(UActorComponent* Component)
{
	if (UNiagaraComponent* NiagaraComp = Cast<UNiagaraComponent>(Component))
	{
		if (USkeletalMeshComponent* MeshComp = Cast<USkeletalMeshComponent>(NiagaraComp->GetAttachParent()))
		{
			// 게임 스레드에서 직접 오버레이 머티리얼을 즉시 해제
			MeshComp->SetOverlayMaterial(nullptr);
		}
	}
}
