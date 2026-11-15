#include "ManufacturingAI.h"

#include "GameStructs_products.h"

bool ManufacturingAI::Advance(const Job& WorkOrder, const FSSWProductionRecipe& Recipe,
    FSSWProductionStock& Stock, double Capacity, double Seconds)
{
    if (!FMath::IsFinite(Seconds) || Seconds <= 0.0) return false;

    if (Stock.RemainingSeconds <= 0.0)
    {
        if (WorkOrder.Facility.IsNone() || WorkOrder.InputLocation.IsNone() ||
            WorkOrder.OutputLocation.IsNone() || WorkOrder.InputLocation == WorkOrder.OutputLocation ||
            !FMath::IsFinite(Capacity) || Capacity < 0.0 ||
            !FMath::IsFinite(WorkOrder.MaterialMultiplier) || WorkOrder.MaterialMultiplier <= 0.0)
            return false;

        const double Inputs = Recipe.InputTonnes * WorkOrder.MaterialMultiplier;
        const double Available = Stock.GetTonnes(WorkOrder.InputLocation);
        if (!FMath::IsFinite(Inputs) || !FMath::IsFinite(Recipe.OutputTonnes) ||
            !FMath::IsFinite(Recipe.SecondsPerBatch) || Inputs <= 0.0 || Available < Inputs ||
            !Recipe.CanStart(Recipe.InputTonnes, Capacity - Stock.UsedSpace() + Inputs))
            return false;

        Stock.GetInventory(WorkOrder.InputLocation) -= Inputs;
        Stock.PendingOutput = WorkOrder.OutputLocation;
        Stock.ReservedOutput = Recipe.OutputTonnes;
        Stock.RemainingSeconds = Recipe.SecondsPerBatch;
    }

    Stock.RemainingSeconds = FMath::Max(0.0, Stock.RemainingSeconds - Seconds);
    if (Stock.RemainingSeconds > 0.0) return false;

    Stock.GetInventory(Stock.PendingOutput) += Stock.ReservedOutput;
    Stock.PendingOutput = NAME_None;
    Stock.ReservedOutput = 0.0;
    return true;
}

