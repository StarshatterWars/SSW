#pragma once
#include "GameStructs.h"
#include "ShipCommerceAI.h"

enum class EOwnedCommand : uint8 { Purchase,Rent,Sell,Return,Debt,Policy,Loss,TransferDrone,Loadout,Fit,Repair };
struct FOwnedActionSelection
{
    EOwnedCommand Action=EOwnedCommand::Purchase;
    FGuid Hull;
    FName Design;
    int32 Days=1,Quantity=1,Slot=0,Grade=1;
    EShipInsuranceTier Tier=EShipInsuranceTier::Basic;
    TArray<int32> Stations;
    FString Loadout;
};
// A projection of ownership only. Career, missions, skills, time and UI preferences
// are never replaced by a commerce result.
struct FNativeOwnedState
{
    TArray<FPlayerHangarShip> Hulls;
    TArray<FShipRentalContract> Rentals;
    TArray<FStoredShipEquipment> Stores;
    TSet<FName> Skins;
    FGuid Assigned,Selected;
    static FNativeOwnedState Capture(const FS_PlayerGameInfo&);
    void Apply(FS_PlayerGameInfo&)const;
    bool Matches(const FS_PlayerGameInfo&)const; // game-thread acceptance only
};
struct FNativeOwnedCommand
{
    EOwnedCommand Action=EOwnedCommand::Purchase;
    FGuid Receipt,Hull;
    FShipCommerceDock Dock;
    FShipCommerceRules Rules;
    FPlayerHangarShip PreparedHull;
    TMap<FName,EShipCategory> Categories;
    int64 Cost=0,BaseCost=0,NowUtc=0;
    int32 Days=1,Quantity=1,Slot=0,Grade=1;
    EShipInsuranceTier Tier=EShipInsuranceTier::Basic;
    FName Hold,Magazine;
    TMap<FName,int32> Loaded,Wanted;
    TArray<int32> Stations;
    FString Loadout;
    bool bValidatedDeath=false;
    FNativeOwnedState Source;
};
struct FNativeOwnedReply
{
    bool bSucceeded=false,bLoss=false;
    FText Reason;
    FNativeOwnedState Source,Owned;
};
namespace OwnedCommandAI
{
    FNativeOwnedReply Execute(FEconomyState&,FNativeOwnedCommand&&);
}
