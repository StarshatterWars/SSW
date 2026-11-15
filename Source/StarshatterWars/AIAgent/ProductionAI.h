#pragma once

#include "ManufacturingAI.h"
#include "SimDirector.h"

class ProductionAI : public SimDirector
{
public:
    static bool AdvanceFacility(bool bOperational, const ManufacturingAI::Job& WorkOrder,
        const FSSWProductionRecipe& Recipe, FSSWProductionStock& Stock, double Capacity, double Seconds);
};
