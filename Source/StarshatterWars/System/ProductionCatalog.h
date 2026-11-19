#pragma once
#include "GameStructs_products.h"
// Runtime JSON records; editor DataTables are optional export artifacts only.
namespace SSWProductionCatalog
{
    bool Reload(FString& Error);
    const TMap<FName,FSSWProductionFacility>& Facilities();
    const TMap<FName,FSSWProductionRecipe>& Recipes();
}
