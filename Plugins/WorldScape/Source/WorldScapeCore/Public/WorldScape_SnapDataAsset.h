// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "WorldScapeRoot.h"
#include "WorldScape_GridStruct.h"
#include "WorldScape_SnapDataAsset.generated.h"

/**
 * 
 */
UCLASS(Blueprintable)
class WORLDSCAPECORE_API UWorldScape_SnapDataAsset : public UDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere,BlueprintReadWrite,Category = "Snap")
	TArray<FWS_Grid_Actor_Data> Collection_ActorsDatas;
	UPROPERTY(EditAnywhere,BlueprintReadWrite,Category = "Snap")
	TArray<FWS_Grid_StaticMesh_Data> Collection_StaticMeshDatas;
	UPROPERTY(EditAnywhere,BlueprintReadWrite,Category = "Snap")
	FVector Extent = FVector(1000,1000,1000);
	// refeerence
	UPROPERTY(EditAnywhere,BlueprintReadWrite,Category = "Snap")
	TArray<UObject*> ReferencedObject;
	
};
