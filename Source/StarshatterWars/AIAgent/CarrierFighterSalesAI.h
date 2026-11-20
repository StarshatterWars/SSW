#pragma once
#include "GameStructs.h"
#include "GameStructs_System.h"
class Hangar;
class Ship;
struct FCarrierFighterOffer
{
    FName Carrier,Fighter;
    FString Squadron;
    int32 Quantity=0;
    int64 Price=0;
    TArray<TPair<FString,int32>> Squadrons;
};
class CarrierFighterSalesAI
{
public:
    static bool IsTrainingPilot(const FS_PlayerGameInfo&);
    static TArray<FCarrierFighterOffer> Offers(const FS_PlayerGameInfo&,const Ship& Carrier);
    static TOptional<FCarrierFighterOffer> Offer(const FS_PlayerGameInfo&,const Ship& Carrier,FName Fighter);
    static FName Inventory(FName System,const FString& CarrierName,FName Fighter);
    static bool Prepare(FS_PlayerGameInfo&,Ship& Carrier,FName System,TFunctionRef<bool()> Save,FText&);
    static bool FindReserve(Hangar&,const FCarrierFighterOffer&,int32& Squadron,int32& Slot);
    static int32 Available(const FEconomyState&,FName Inventory,Hangar&,const FCarrierFighterOffer&);
    static bool Purchase(FS_PlayerGameInfo&,const FShipDesign&,Hangar&,const FCarrierFighterOffer&,FName Inventory,
        const FString& Location,FGuid Receipt,TFunctionRef<bool()> Save,FText&);
};
