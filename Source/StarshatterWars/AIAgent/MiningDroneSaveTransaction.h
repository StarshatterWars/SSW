#pragma once
#include "GameStructs_economy.h"
#include "Asteroid.h"

namespace MiningDroneSaveTransaction
{
inline bool Commit(FEconomyState& State,const FEconomyState& Before,const TArray<TPair<Asteroid*,double>>& Extracted,
    TFunctionRef<bool()> Save,TFunctionRef<void()> ApplyCargo)
{
    ApplyCargo();
    if(Save())return true;
    for(const auto& Entry:Extracted)if(Entry.Key)Entry.Key->ReturnExtractedOre(Entry.Value);
    State=Before;ApplyCargo();return false;
}
}
