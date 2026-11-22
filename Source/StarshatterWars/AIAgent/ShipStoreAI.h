#pragma once
#include "GameStructs.h"

struct FShipStoreOffer
{
    FName Id, Design, SystemDesign, Component;
    EShipStoreCategory Category = EShipStoreCategory::Hulls;
    FString Name, Group, Description, Access;
    int32 Grade = 0;
    int64 Price = 0;
};

// Native purchase/storage rules. UI supplies no authority and never edits a live fitting.
namespace ShipStoreAI
{
    FName ComponentId(FName System, FName Component, int32 Grade);
    int32 Stored(const FS_PlayerGameInfo&, FName Id, const FString& Dock);
    bool CanPurchase(const FS_PlayerGameInfo&, const FShipStoreOffer&, int32 Quantity,
        const FString& Dock, bool Authority, FText& Reason);
    bool Purchase(FS_PlayerGameInfo&, const FShipStoreOffer&, int32 Quantity,
        const FString& Dock, bool Authority, FGuid Receipt, TFunctionRef<bool()> Persist, FText& Reason);
    bool TakeComponent(FS_PlayerGameInfo&, FName System, FName Component, int32 Grade,
        const FString& Dock, FHangarComponent& Out);
    void StoreComponent(FS_PlayerGameInfo&, const FHangarComponent&, const FString& Dock);
    bool TransferMiningDrone(FS_PlayerGameInfo&,const FString& Dock,FName CargoHold,int32 Quantity,
        bool Authority,TFunctionRef<bool()> Persist,FText& Reason);
}
