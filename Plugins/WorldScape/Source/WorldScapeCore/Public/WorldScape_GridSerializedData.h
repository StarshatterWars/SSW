// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "WorldScape_GridStruct.h"
#include "Engine/DataAsset.h"
#include "WorldScape_GridSerializedData.generated.h"

/**
 * 
 */
UCLASS(Blueprintable)
class WORLDSCAPECORE_API UWorldScape_GridSerializedData : public UDataAsset
{
	GENERATED_BODY()

public:
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite,Category = "Data")
	int64 StorageIndex = 0;
	UPROPERTY(EditAnywhere, BlueprintReadWrite,Category = "Data")
	FWS_Grid_SerializedData Storage = FWS_Grid_SerializedData();
	
};
