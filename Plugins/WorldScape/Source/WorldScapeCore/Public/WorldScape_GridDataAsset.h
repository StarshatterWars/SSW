// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "WorldScape_GridStruct.h"
#include "Engine/DataAsset.h"
#include "WorldScape_GridDataAsset.generated.h"

/**
 * 
 */
UCLASS(Blueprintable)
class WORLDSCAPECORE_API UWorldScape_GridDataAsset : public UDataAsset
{
	GENERATED_BODY()

public:

	UPROPERTY(EditAnywhere,BlueprintReadWrite,Category = "Data")
	bool Enabled = true;
	// Spawn Data
	UPROPERTY(EditAnywhere,BlueprintReadWrite,Category = "Data")
	bool SpawnableOnDedicated = true;
	UPROPERTY(EditAnywhere,BlueprintReadWrite,Category = "Data")
	FWS_Grid_SpawnParams SpawnParams;
	UPROPERTY(EditAnywhere,BlueprintReadWrite,Category = "Data")
	double Z_Offset = 0.0f;
	UPROPERTY(EditAnywhere,BlueprintReadWrite,Category = "Data")
	int32 SeedOffset = 0;
	UPROPERTY(EditAnywhere,BlueprintReadWrite,Category = "Data")
	int32 Max_TestToSpawn = 200;
	UPROPERTY(EditAnywhere,BlueprintReadWrite,Category = "Data")
	int32 Max_Spawn = 20;

	
	UPROPERTY(EditAnywhere,BlueprintReadWrite,Category = "Data")
	TEnumAsByte<EWS_Grid_SpawnType> SpawnType = Actor;
	UPROPERTY(EditAnywhere,BlueprintReadWrite,Category = "Data")
	TEnumAsByte<EWS_Grid_SamplingType> SamplingType = CircleRND;
	// Only , Actor , Static , Collection , CollectionStaticMesh or Foliage 
	UPROPERTY(EditAnywhere,BlueprintReadWrite,Category = "Data")
	TArray<FWS_Grid_Spawner_DataAsset> Spawners;
	
	// Multi
	UPROPERTY(EditAnywhere,BlueprintReadWrite,Category = "Data")
	TArray<FWS_Grid_Spawner_Actor> ActorsDatas;
	// Used for StaticMesh and Foliage
	UPROPERTY(EditAnywhere,BlueprintReadWrite,Category = "Data")
	TArray<FWS_Grid_StaticMesh_Data> StaticMeshDatas;
	// Collection
	UPROPERTY(EditAnywhere,BlueprintReadWrite,Category = "Data")
	FWS_Grid_Proxy Collection_ProxyData = FWS_Grid_Proxy();
	UPROPERTY(EditAnywhere,BlueprintReadWrite,Category = "Data")
	TArray<FWS_Grid_Actor_Data> Collection_ActorsDatas;
	UPROPERTY(EditAnywhere,BlueprintReadWrite,Category = "Data")
	TArray<FWS_Grid_StaticMesh_Data> Collection_StaticMeshDatas;
	UPROPERTY(EditAnywhere,BlueprintReadWrite,Category = "Data")
	FVector Extent = FVector(1000,1000,1000);
	// Foliage
	UPROPERTY(EditAnywhere,BlueprintReadWrite,Category = "Data")
	double FoliageSectorSize = 3200000.0f; // 2KM Sector size
	// refeerence
	UPROPERTY(EditAnywhere,BlueprintReadWrite,Category = "Data")
	TArray<UObject*> ReferencedObject;
};
