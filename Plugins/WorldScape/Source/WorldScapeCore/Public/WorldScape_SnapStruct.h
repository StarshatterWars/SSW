// Copyright 2024 IOLACORP STUDIO. All Rights Reserved

#pragma once

#include "CoreMinimal.h"
#include "WorldScape_GridStruct.h"
#include "WorldScape_SnapStruct.generated.h"

class AWorldScape_SnapSpawnerActor;

// --------------------------------------------------------
UENUM(BlueprintType)
enum class EWSSnapDataType : uint8
{
	ManualData,
	SnapData,
	GridData
};
// --------------------------------------------------------
USTRUCT(BlueprintType)
struct FWS_SnapData
{
	GENERATED_BODY()
	UPROPERTY()
	TArray<FWS_Grid_Actor_Data> Actors;
	UPROPERTY()
	TArray<FWS_Grid_StaticMesh_Data> StaticMesh;
	UPROPERTY()
	FVector Extent = FVector(1000,1000,1000);
	FWS_SnapData() = default;
	FWS_SnapData( const TArray<FWS_Grid_Actor_Data>& inActors , TArray<FWS_Grid_StaticMesh_Data>& inStaticMesh , const FVector inExtent )
	{
		Actors = inActors;
		StaticMesh = inStaticMesh;
		Extent = inExtent;
	}
};
// --------------------------------------------------------
USTRUCT()
struct FWS_SnapSpawnData
{
	GENERATED_BODY()
	UPROPERTY()
	TArray<FWS_Grid_Actor_SpawnData> Actor;
	UPROPERTY()
	TArray<FWS_Grid_Static_SpawnData> StaticMesh;
	FWS_SnapSpawnData() = default;
	FWS_SnapSpawnData( TArray<FWS_Grid_Actor_SpawnData>& inArray, TArray<FWS_Grid_Static_SpawnData>& inStaticMesh )
	{
		Actor = MoveTemp(inArray);
		StaticMesh = MoveTemp(inStaticMesh);
	}
};
/*
USTRUCT()
struct FWS_SnapSpawnData_Pending
{
	GENERATED_BODY()
	UPROPERTY()
	TWeakObjectPtr<AWorldScape_SnapSpawnerActor> SnapActor;
	UPROPERTY()
	FWS_SnapSpawnData SpawnData = FWS_SnapSpawnData();
	FWS_SnapSpawnData_Pending() = default;
	FWS_SnapSpawnData_Pending( TWeakObjectPtr<AWorldScape_SnapSpawnerActor>& inSnapActor , FWS_SnapSpawnData& inSpawnData )
	{
		SnapActor = inSnapActor;
		SpawnData = inSpawnData;
	}
};
*/