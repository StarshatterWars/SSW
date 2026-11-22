#pragma once
#include "GameStructs_products.h"
// Runtime JSON records; editor DataTables are optional export artifacts only.
namespace SSWProductionCatalog
{
    bool Reload(FString& Error);
    const TMap<FName,FSSWProductionFacility>& BuildingProfiles();
    FName GroundKey(const FGroundFactorySite& Site);
    bool ResolveGround(const FGroundFactorySite& Site,FSSWProductionFacility& Out);
    bool RegisterGround(FEconomyState& State,const FGroundFactorySite& Site);
    void RestoreGround(const FEconomyState& State);
    const TMap<FName,FSSWProductionFacility>& Facilities();
    const TMap<FName,FSSWProductionRecipe>& Recipes();
}
