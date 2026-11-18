#pragma once
#include "CoreMinimal.h"
#include "GameStructs_traffic.generated.h"

UENUM()
enum class EGalaxyTrafficPhase:uint8 { Waiting, Loading, Traveling, Unloading, Completed, Lost, Blocked };
USTRUCT()
struct FGalaxyTrafficJourney
{
    GENERATED_BODY()
    UPROPERTY(SaveGame) FGuid Id;
    UPROPERTY(SaveGame) FName Origin;
    UPROPERTY(SaveGame) FName Destination;
    UPROPERTY(SaveGame) FName CurrentSector;
    UPROPERTY(SaveGame) TArray<FName> Route;
    UPROPERTY(SaveGame) int32 RouteIndex=0;
    UPROPERTY(SaveGame) EGalaxyTrafficPhase Phase=EGalaxyTrafficPhase::Waiting;
    UPROPERTY(SaveGame) FName SourceInventory;
    UPROPERTY(SaveGame) FName DestinationInventory;
    UPROPERTY(SaveGame) FName Hold;
    UPROPERTY(SaveGame) FName Item;
    UPROPERTY(SaveGame) FGuid Reservation;
    UPROPERTY(SaveGame) double Quantity=0;
    UPROPERTY(SaveGame) double UnitTonnes=1;
    UPROPERTY(SaveGame) double Remaining=0;
    UPROPERTY(SaveGame) double Age=0;
    UPROPERTY(SaveGame) FString BlockedReason;
    UPROPERTY(SaveGame) bool bDestinationReserved=false;
    bool IsCargo()const{return !Item.IsNone();}
    bool IsFinished()const{return Phase==EGalaxyTrafficPhase::Completed || Phase==EGalaxyTrafficPhase::Lost;}
};
USTRUCT()
struct FGalaxyTrafficState
{
    GENERATED_BODY()
    UPROPERTY(SaveGame) TArray<FGalaxyTrafficJourney> Journeys;
    UPROPERTY(SaveGame) TMap<FName,double> SectorLastServed;
    UPROPERTY(SaveGame) int32 Sequence=0;
    UPROPERTY(SaveGame) double Clock=0;
};
