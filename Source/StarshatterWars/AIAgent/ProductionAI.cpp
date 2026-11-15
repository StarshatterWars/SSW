#include "ProductionAI.h"

bool ProductionAI::AdvanceFacility(bool bOperational, const ManufacturingAI::Job& WorkOrder,
    const FSSWProductionRecipe& Recipe, FSSWProductionStock& Stock, double Capacity, double Seconds)
{
    return bOperational && ManufacturingAI::Advance(WorkOrder, Recipe, Stock, Capacity, Seconds);
}
