#pragma once

#include "CoreMinimal.h"
#include "GameStructs_economy.h"
#include "GameStructs_products.h"

struct FSSWProductionRecipe;
struct FSSWProductionStock;

class ManufacturingAI
{
public:
    struct Job
    {
        FName Facility;
        FName InputLocation;
        FName OutputLocation;
        double MaterialMultiplier = 1.0;
    };

    static TArray<FName> MineralKeys(const FSSWProductionRecipe&);
    static bool BuildBatch(const FEconomyState&, FName Input, const FSSWProductionRecipe&, TArray<FCommodityAmount>& Materials, TArray<FCommodityAmount>& Products);
    static bool AdvanceLive(FEconomyState&, FName Facility, FName Input, FName Output, FName RecipeId,
        const FSSWProductionRecipe&, bool bOperational, double Seconds);
    static FSSWProductionStock Describe(const FEconomyState&, FName Facility, FName Input, FName Output);
    static bool Advance(const Job& WorkOrder, const FSSWProductionRecipe& Recipe,
        FSSWProductionStock& Stock, double Capacity, double Seconds);
};
