// Copyright 2021 IOLACORP STUDIO. All Rights Reserved

#pragma once

#include "CoreMinimal.h"
#include "PCGSettings.h"
#include "PCGElement.h"
#include "PCGContext.h"
#include "WorldScapePCGNodeBase.generated.h"

class AWorldScapeRoot;

/**
 * Base settings class for WorldScape PCG nodes
 */
UCLASS(Abstract, BlueprintType, Blueprintable)
class WORLDSCAPEPCG_API UWorldScapePCGNodeBaseSettings : public UPCGSettings
{
	GENERATED_BODY()

public:
	UWorldScapePCGNodeBaseSettings();

	//~Begin UPCGSettings interface
#if WITH_EDITOR
	virtual FName GetDefaultNodeName() const override { return TEXT("WorldScape PCG Node"); }
	virtual FText GetDefaultNodeTitle() const override { return NSLOCTEXT("WorldScapePCG", "NodeTitle", "WorldScape PCG Node"); }
	virtual FLinearColor GetNodeTitleColor() const override { return FLinearColor::Blue; }
	virtual EPCGSettingsType GetType() const override { return EPCGSettingsType::Spatial; }
#endif
	//~End UPCGSettings interface

protected:
	//~Begin UPCGSettings interface
	virtual TArray<FPCGPinProperties> InputPinProperties() const override;
	virtual TArray<FPCGPinProperties> OutputPinProperties() const override;
	virtual FPCGElementPtr CreateElement() const override;
	//~End UPCGSettings interface

public:
	/** Reference to WorldScape Root actor */
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "WorldScape")
	TSoftObjectPtr<AWorldScapeRoot> WorldScapeRoot;
};

/**
 * Simple PCG element that just passes through data
 */
struct WORLDSCAPEPCG_API FWorldScapePCGNodeBaseElement : public IPCGElement
{
	virtual FPCGContext* Initialize(const FPCGDataCollection& InputData, TWeakObjectPtr<UPCGComponent> SourceComponent, const UPCGNode* Node) override;
	virtual bool ExecuteInternal(FPCGContext* Context) const override;

protected:
	virtual bool ExecuteWorldScapeLogic(FPCGContext* Context, const UWorldScapePCGNodeBaseSettings* Settings) const;

	/** Helper function to get WorldScape root */
	AWorldScapeRoot* GetWorldScapeRoot(const UWorldScapePCGNodeBaseSettings* Settings) const;
};