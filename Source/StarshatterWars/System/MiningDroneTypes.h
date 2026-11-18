#pragma once
#include "CoreMinimal.h"
#include "MiningDroneTypes.generated.h"

UENUM()
enum class EMiningDroneState : uint8 { Docked, Holding, Approaching, Mining, Returning, Lost };
USTRUCT()
struct FMiningDroneBayDefinition
{
    GENERATED_BODY()
    UPROPERTY() FName ShipDesign;
    UPROPERTY() int32 BayCount=0;
    UPROPERTY() int32 Bandwidth=0;
    UPROPERTY() int32 DroneBandwidth=1;
    UPROPERTY() double CapacityTonnes=.25;
    UPROPERTY() double Speed=250;
    UPROPERTY() double ControlRange=25000;
    UPROPERTY() double MiningRange=150;
    UPROPERTY() double MiningRateScale=.25;
    UPROPERTY() FName MiningWeapon=TEXT("Mining Laser");
};
USTRUCT()
struct FMiningDroneState
{
    GENERATED_BODY()
    UPROPERTY(SaveGame) FGuid Id;
    UPROPERTY(SaveGame) EMiningDroneState State=EMiningDroneState::Docked;
    UPROPERTY(SaveGame) FVector Position=FVector::ZeroVector;
    UPROPERTY(SaveGame) int32 TargetContact=INDEX_NONE;
    UPROPERTY(SaveGame) FName Ore;
    UPROPERTY(SaveGame) double OreTonnes=0;
};
USTRUCT()
struct FMiningDroneFleetState
{
    GENERATED_BODY()
    UPROPERTY(SaveGame) FName ShipDesign;
    UPROPERTY(SaveGame) FName Region;
    UPROPERTY(SaveGame) TArray<FMiningDroneState> Drones;
};
