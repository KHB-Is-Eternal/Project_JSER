// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "NiagaraDataInterface.h"
#include "NiagaraDataInterface_OverlayTrigger.generated.h"

class UMaterialInterface;
class FNiagaraSystemInstance;

/**
 * Custom Niagara Data Interface to apply overlay materials to the attached SkeletalMeshComponent.
 */
UCLASS(EditInlineNew, Blueprintable, BlueprintType, Category = "Overlay", CollapseCategories, meta = (DisplayName = "Overlay Trigger"))
class PROJECTER_API UNiagaraDataInterfaceOverlayTrigger : public UNiagaraDataInterface
{
	GENERATED_BODY()

public:
	UNiagaraDataInterfaceOverlayTrigger();

	virtual void PostInitProperties() override;

	/** The overlay material to apply to the skeletal mesh */
	UPROPERTY(EditAnywhere, Category = "Overlay")
	TSoftObjectPtr<UMaterialInterface> OverlayMaterial;

	//~ Begin UNiagaraDataInterface Interface
	virtual void GetVMExternalFunction(const FVMExternalFunctionBindingInfo& BindingInfo, void* InstanceData, FVMExternalFunction& OutFunc) override;
	virtual bool Equals(const UNiagaraDataInterface* Other) const override;
	virtual bool CopyToInternal(UNiagaraDataInterface* Destination) const override;

	// Per-instance data management
	virtual int32 PerInstanceDataSize() const override;
	virtual bool InitPerInstanceData(void* PerInstanceData, FNiagaraSystemInstance* SystemInstance) override;
	virtual void DestroyPerInstanceData(void* PerInstanceData, FNiagaraSystemInstance* SystemInstance) override;
	//~ End UNiagaraDataInterface Interface

protected:
#if WITH_EDITORONLY_DATA
	virtual void GetFunctionsInternal(TArray<FNiagaraFunctionSignature>& OutFunctions) const override;
#endif

private:
	// VM external function implementations
	void VMApplyOverlayMaterial(FVectorVMExternalFunctionContext& Context);
	void VMClearOverlayMaterial(FVectorVMExternalFunctionContext& Context);

	UFUNCTION()
	void OnNiagaraComponentDeactivated(UActorComponent* Component);
};
