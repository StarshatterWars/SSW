// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "WorldScape_GridCellData.generated.h"

class UWorldScape_GridSerializedData;
struct FWS_Grid_Value;
/**
 * 
 */
UCLASS()
class WORLDSCAPECORE_API UWorldScape_GridCellData : public UDataAsset
{
	GENERATED_BODY()

public:

	UPROPERTY(BlueprintReadWrite, Category = "Grid")
	TArray<FWS_Grid_Value> Grid_StaticData;
	UPROPERTY(BlueprintReadWrite, Category = "Grid")
	TMap<int64,TSoftObjectPtr<UWorldScape_GridSerializedData>> Grid_StorageData;
};
