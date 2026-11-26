#pragma once
#include "CoreMinimal.h"
#include "GameStructs_insurance.h"
#include "RepairTypes.generated.h"

UENUM()
enum class EFleetRepairState : uint8 { Queued, Supplies, Shipyard, Running, Paused, Completed, Cancelled };

// Native work-order data. Kept with the economy debit so a split save cannot
// consume supplies without retaining the paid work that can reconcile the OOB.
USTRUCT()
struct FFleetRepairOrder
{
 GENERATED_BODY()
 UPROPERTY(SaveGame) FGuid Id;
 UPROPERTY(SaveGame) FGuid FleetId;
 UPROPERTY(SaveGame) FGuid HullId;
 UPROPERTY(SaveGame) FGuid ReplacementId;
 UPROPERTY(SaveGame) FName System;
 UPROPERTY(SaveGame) FName Depot;
 UPROPERTY(SaveGame) FName Shipyard;
 UPROPERTY(SaveGame) bool bReplacement=false;
 UPROPERTY(SaveGame) bool bResourcesConsumed=false;
 UPROPERTY(SaveGame) EFleetRepairState State=EFleetRepairState::Queued;
 UPROPERTY(SaveGame) FString Reason;
 UPROPERTY(SaveGame) double CreatedAt=0;
 UPROPERTY(SaveGame) double LastAt=0;
 UPROPERTY(SaveGame) double WorkedSeconds=0;
 UPROPERTY(SaveGame) double DurationSeconds=0;
 UPROPERTY(SaveGame) FPlayerHangarShip Baseline;
 UPROPERTY(SaveGame) TMap<FName,double> Materials;
 bool IsFinished() const {return State==EFleetRepairState::Completed || State==EFleetRepairState::Cancelled;}
 double Progress() const {return DurationSeconds>0?FMath::Clamp(WorkedSeconds/DurationSeconds,0.,1.):0.;}
};
