// Copyright 2021 IOLACORP STUDIO. All Rights Reserved

#pragma once

#include "CoreMinimal.h"
#include "WorldScapePCGNodeBase.h"
#include "WorldScapeTestPCGNode.generated.h"

/**
 * Simple test PCG node for WorldScape that gets height values at points
 */
UCLASS(BlueprintType, Blueprintable, Category = "WorldScape PCG")
class WORLDSCAPEPCG_API UWorldScapeTestPCGNodeSettings : public UWorldScapePCGNodeBaseSettings
{
	GENERATED_BODY()

public:
	UWorldScapeTestPCGNodeSettings();

	//~Begin UPCGSettings interface
#if WITH_EDITOR
	virtual FName GetDefaultNodeName() const override { return TEXT("WorldScape Test Node"); }
	virtual FText GetDefaultNodeTitle() const override { return NSLOCTEXT("WorldScapePCG", "TestNodeTitle", "WorldScape Test Node"); }
	virtual FText GetNodeTooltipText() const override { return NSLOCTEXT("WorldScapePCG", "TestNodeTooltip", "A simple test node that samples WorldScape height at point locations"); }
#endif
	//~End UPCGSettings interface

protected:
	//~Begin UPCGSettings interface
	virtual FPCGElementPtr CreateElement() const override;
	//~End UPCGSettings interface

public:
	/** Height offset to apply to sampled points */
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Settings", meta = (ClampMin = "-1000.0", ClampMax = "1000.0"))
	float HeightOffset = 0.0f;

	/** Whether to include water height in the calculation */
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Settings")
	bool bIncludeWater = false;

	/** Scale factor for the height values */
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Settings", meta = (ClampMin = "0.1", ClampMax = "10.0"))
	float HeightScale = 1.0f;
};

/**
 * Element implementation for the WorldScape Test PCG node
 */
struct WORLDSCAPEPCG_API FWorldScapeTestPCGNodeElement : public FWorldScapePCGNodeBaseElement
{
protected:
	virtual bool ExecuteWorldScapeLogic(FPCGContext* Context, const UWorldScapePCGNodeBaseSettings* Settings) const override;
};