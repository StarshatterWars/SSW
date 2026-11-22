#pragma once
#include "GameStructs.h"
class Ship;
class WeaponDesign;
namespace OrdnanceMagazine
{
    FName ItemId(const WeaponDesign&);
    FName Key(const Ship& Base);
    FName Ensure(FEconomyState&,const Ship& Base,FText& Reason);
    TMap<FName,int32> Loaded(Ship& Craft);
    bool Required(Ship& Craft,const TArray<int32>& Stations,TMap<FName,int32>& Out,FText& Reason);
    bool Exchange(FS_PlayerGameInfo&,const FString& Dock,FName Magazine,
        const TMap<FName,int32>& Loaded,const TMap<FName,int32>& Required,FText& Reason);
    int32 ReplenishCount(FEconomyState&,FName Magazine,FName Item,int32 Loaded,int32 Capacity);
    void ReplenishAI(Ship& Base,Ship& Craft);
    void ReturnAI(Ship& Base,Ship& Craft);
}
