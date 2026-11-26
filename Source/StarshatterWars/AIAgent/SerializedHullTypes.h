#pragma once
#include "CoreMinimal.h"
#include "SerializedHullTypes.generated.h"

USTRUCT()
struct FSerializedHullStock
{
 GENERATED_BODY()
 UPROPERTY(SaveGame) FGuid HullId;
 UPROPERTY(SaveGame) FName Inventory;
 UPROPERTY(SaveGame) FName Design;
 UPROPERTY(SaveGame) FString Squadron;
 UPROPERTY(SaveGame) int32 Slot=INDEX_NONE;
 UPROPERTY(SaveGame) FGuid ReservedBy;
 UPROPERTY(SaveGame) FGuid SoldReceipt;
 UPROPERTY(SaveGame) FString Delivery;
 UPROPERTY(SaveGame) bool bWithdrawn=false;
};
