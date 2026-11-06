// Copyright 2024 IOLACORP STUDIO. All Rights Reserved

#pragma once

#include "CoreMinimal.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "WorldScape_FCP_Struct.generated.h"


USTRUCT()
struct FWSFCP_TransformArray
{
	GENERATED_BODY()
	UPROPERTY()
	TArray<FTransform> Transforms;
	// -----------
	FWSFCP_TransformArray() = default;
	// -----------
	FWSFCP_TransformArray(const TArray<FTransform>& other)
		: Transforms(other)
	{}
};
// ---------------------------------------
USTRUCT()
struct FWSFCP_ISM_Collection
{
	GENERATED_BODY()
	UPROPERTY()
	TArray<UInstancedStaticMeshComponent*> ISM;
	FWSFCP_ISM_Collection() = default;
	FWSFCP_ISM_Collection( UInstancedStaticMeshComponent* InISM )
	{
		ISM.Add(InISM);
	};
};
// ---------------------------------------
USTRUCT()
struct FWSFCP_AsyncData
{
	GENERATED_BODY()
	UPROPERTY()
	FTransform Transform;
	UPROPERTY()
	TArray<FInstancedStaticMeshInstanceData> Instances;
	FWSFCP_AsyncData() = default;
	FWSFCP_AsyncData( FTransform InTransform , TArray<FInstancedStaticMeshInstanceData> InInstances )
		: Transform(InTransform), Instances(InInstances)
	{}
};