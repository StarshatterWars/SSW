// Copyright 2024 IOLACORP STUDIO. All Rights Reserved

#pragma once

#include "CoreMinimal.h"
#include "PhysicsEngine/BodyInstance.h"
#include "WorldScapeFoliageFCP_Struct.generated.h"

// ----------------------------------
UENUM(BlueprintType)
enum class EWS_FCP_TypeCompute : uint8
{
	Auto,
	Near,
	Far
};
// ----------------------------------
USTRUCT(BlueprintType)
struct FWS_FCP_PoolingData
{
	GENERATED_BODY()
	UPROPERTY(EditAnywhere,BlueprintReadWrite, Category = "Collision")
	int32 BasePoolSize = 100;
	UPROPERTY(EditAnywhere,BlueprintReadWrite, Category = "Collision")
	float MinimalScale = 0.1;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category=Collision, meta=(ShowOnlyInnerProperties, SkipUCSModifiedProperties))
	FBodyInstance BodyInstance;
};
