#pragma once
#include "GameStructs.h"
#include "GameStructs_System.h"

struct FShipCommerceRules
{
    int32 BuybackPercent = 70;
    int32 DailyRentalPercent = 2;
    static bool Load(FShipCommerceRules& Out, FString& Error);
};
struct FShipCommerceDock
{
    bool bAuthorized = false;
    FString Location;
    FName Inventory;
    FName Dealer;
    FName System;
};
// Plain simulation transactions. The UI cannot supply wallet credits or override custody.
namespace ShipCommerceAI
{
    int64 SalePrice(const FPlayerHangarShip&,const FShipCommerceRules&);
    int64 RentalFee(int64 HullValue,int32 Days,const FShipCommerceRules&);
    bool CanSell(const FS_PlayerGameInfo&,FGuid,const FShipCommerceDock&,const FShipCommerceRules&,FText&);
    bool Sell(FS_PlayerGameInfo&,FGuid,const FShipCommerceDock&,const FShipCommerceRules&,FGuid Receipt,int64 Now,TFunctionRef<bool()> Save,FText&);
    bool Rent(FS_PlayerGameInfo&,const FShipDesign&,FGuid,const FShipCommerceDock&,const FShipCommerceRules&,int32 Days,int64 Now,FGuid Receipt,FText&);
    bool CanReturn(const FS_PlayerGameInfo&,FGuid,const FShipCommerceDock&,FText&);
    bool Return(FS_PlayerGameInfo&,FGuid,const FShipCommerceDock&,FGuid Receipt,int64 Now,TFunctionRef<bool()> Save,FText&);
    bool SettleRentalLoss(FS_PlayerGameInfo&,FGuid,int64 Now,TFunctionRef<bool()> Save,FText&);
    bool PayRentalDebt(FS_PlayerGameInfo&,FGuid,FGuid Receipt,TFunctionRef<bool()> Save,FText&);
    bool RentalMayLaunch(const FS_PlayerGameInfo&,const FPlayerHangarShip&,int64 Now);
}
