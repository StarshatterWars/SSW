#pragma once
#include "GameStructs.h"
#include "GameStructs_System.h"
class ShipyardAI
{
public:
    static bool Eligible(const FS_PlayerGameInfo&,const FShipDesign&,FText&);
    static double Stock(const FEconomyState&,FName Inventory,FName Design);
    static bool Purchase(FS_PlayerGameInfo&,const FShipDesign&,FName Inventory,const FString& Location,FGuid Receipt,FText&,FGuid OfferedHullId=FGuid());
};
