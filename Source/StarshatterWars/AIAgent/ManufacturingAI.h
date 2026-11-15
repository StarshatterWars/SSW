#pragma once

#include "CoreMinimal.h"

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

    static bool Advance(const Job& WorkOrder, const FSSWProductionRecipe& Recipe,
        FSSWProductionStock& Stock, double Capacity, double Seconds);
};
