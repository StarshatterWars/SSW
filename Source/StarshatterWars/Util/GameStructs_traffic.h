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
    UPROPERTY(SaveGame) double TransitionAt=0;
    UPROPERTY(SaveGame) double LastTransitionAt=0;
    UPROPERTY(SaveGame) EGalaxyTrafficPhase ScheduledPhase=EGalaxyTrafficPhase::Waiting;
    UPROPERTY(SaveGame) int32 ScheduledRouteIndex=0;
    UPROPERTY(SaveGame) FString BlockedReason;
    UPROPERTY(SaveGame) bool bDestinationReserved=false;
    UPROPERTY(SaveGame) FName Buyer;
    UPROPERTY(SaveGame) FName Seller;
    UPROPERTY(SaveGame) int64 TradeCredits=0;
    UPROPERTY(SaveGame) bool bTradeSettled=false;
    UPROPERTY(SaveGame) double BlockedSeconds=0;
    UPROPERTY(SaveGame) bool bReturning=false;
    UPROPERTY(SaveGame) bool bPatrol=false;
    UPROPERTY(SaveGame) FName ShipDesign;
    UPROPERTY(SaveGame) FString PilotName;
    UPROPERTY(SaveGame) int32 PatrolIFF=0;
    UPROPERTY(SaveGame) double HullFraction=1;
    UPROPERTY(SaveGame) FVector PatrolPosition=FVector::ZeroVector;
    UPROPERTY(SaveGame) FVector PatrolVelocity=FVector::ZeroVector;
    UPROPERTY(SaveGame) bool bHasPatrolPosition=false;
    UPROPERTY(SaveGame) TArray<int32> PatrolAmmo;
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
