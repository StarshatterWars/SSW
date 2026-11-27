#include "ManufacturingAI.h"
#include "ReprocessingAI.h"
#include "EconomyRuntime.h"
#include "Engine/DataTable.h"

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


TArray<FName> ManufacturingAI::MineralKeys(const FSSWProductionRecipe& Recipe)
{
    if(!Recipe.NativeMineralKeys.IsEmpty() || Recipe.Minerals.IsEmpty())return Recipe.NativeMineralKeys;
    checkf(IsInGameThread(),TEXT("Mineral names must be resolved before economy worker dispatch."));
    TArray<FName> Names;for(auto Mineral:Recipe.Minerals){FSSWAsteroidResource Resource;Resource.Mineral=Mineral;Names.Add(DataTableUtils::MakeValidName(Resource.GetMineralName().ToString()));};return Names;
}
bool ManufacturingAI::BuildBatch(const FEconomyState& State, FName Input, const FSSWProductionRecipe& Recipe,
    TArray<FCommodityAmount>& Materials, TArray<FCommodityAmount>& Products)
{
    Materials.Reset(); Products.Reset();
    if (!ReprocessingAI::Calculate(Recipe).bValid) return false;
    const auto* Inventory = State.Inventories.Find(Input);
    if (!Inventory) return false;
    if (!Recipe.Inputs.IsEmpty())
    {
        TSet<FName> Seen; double Mass = 0;
        for (const auto& Required : Recipe.Inputs)
        {
            if (Required.Item.IsNone() || Seen.Contains(Required.Item) || !FMath::IsFinite(Required.Quantity) ||
                !FMath::IsFinite(Required.UnitTonnes) || Required.Quantity <= 0 || Required.UnitTonnes <= 0) return false;
            Seen.Add(Required.Item); Mass += Required.Quantity * Required.UnitTonnes;
            const auto* Stack = Inventory->Items.FindByPredicate([&](const auto& Item) { return Item.Type == Required.Item; });
            if (!Stack || !FMath::IsNearlyEqual(Stack->UnitTonnes, Required.UnitTonnes) ||
                FEconomyLedger::Available(State, Input, Required.Item) + UE_SMALL_NUMBER < Required.Quantity) return false;
        }
        if (!FMath::IsNearlyEqual(Mass, Recipe.InputTonnes, 1.e-6)) return false;
        Materials = Recipe.Inputs;
    }
    else
    {
        double Needed = Recipe.InputTonnes;
        const auto AcceptedMinerals=MineralKeys(Recipe);
        for (const auto& Stack : Inventory->Items)
        {
            const bool Accepted = AcceptedMinerals.Contains(Stack.Type);
            if (!Accepted || Stack.UnitTonnes != 1) continue;
            const double Amount = FMath::Min(Needed, FEconomyLedger::Available(State, Input, Stack.Type));
            if (Amount <= 0) continue;
            FCommodityAmount Material; Material.Item = Stack.Type; Material.Quantity = Amount;
            Materials.Add(Material); Needed -= Amount;
            if (Needed <= UE_SMALL_NUMBER) break;
        }
        if (Needed > UE_SMALL_NUMBER) { Materials.Reset(); return false; }
    }
    return Recipe.ResolveOutputs(Products);
}

bool ManufacturingAI::AdvanceLive(FEconomyState& State, FName Facility, FName Input, FName Output, FName RecipeId,
    const FSSWProductionRecipe& Recipe, bool bOperational, double Seconds)
{
    if (!bOperational || !FMath::IsFinite(Seconds) || Seconds <= 0 || !ReprocessingAI::Calculate(Recipe).bValid) return false;
    const bool Active = State.Jobs.ContainsByPredicate([Facility](const auto& Job)
        { return Job.Facility == Facility && Job.State != EIndustryJobState::Completed && Job.State != EIndustryJobState::Cancelled; });
    State.RecipeActivities.Add(RecipeId,Recipe.Activity);
    if (!Active && Recipe.CanStart(Recipe.InputTonnes, Recipe.OutputTonnes))
    {
        TArray<FCommodityAmount> Materials, Products;
        if (BuildBatch(State, Input, Recipe, Materials, Products))
        {
            FGuid License;
            for (const auto& Pair : State.Blueprints)
                if (Pair.Value.Owner == FEconomyRuntime::IndustryOwner() && Pair.Value.Recipe == RecipeId && Pair.Value.Facility == Facility && Pair.Value.bOriginal)
                { License = Pair.Key; break; }
            if (!License.IsValid())
            { License = FEconomyLedger::GrantBlueprint(State, FEconomyRuntime::IndustryOwner(), RecipeId, true, 0);
              if (auto* BP = State.Blueprints.Find(License)) BP->Facility = Facility; }
            FEconomyLedger::QueueBlueprintJob(State, FEconomyRuntime::IndustryOwner(), Facility, Input, Output, License, 1,
                Materials, Products, Recipe.SecondsPerBatch);
        }
    }
    const int32 Completed = FEconomyLedger::AdvanceJobs(State, Seconds, [Facility](FName Name) { return Name == Facility; });
    if (Completed > 0)
        UE_LOG(LogTemp, Display, TEXT("[Production] %s completed %d job(s), produced %.2f t of %s."),
            *Facility.ToString(), Completed, Recipe.OutputTonnes, *Recipe.CargoItem);
    // Keep a bounded recent completion history without disturbing active jobs.
    if (State.Jobs.Num() > 512)
        for (int32 Index = 0; Index < State.Jobs.Num() && State.Jobs.Num() > 512;)
            if (State.Jobs[Index].State == EIndustryJobState::Completed || State.Jobs[Index].State == EIndustryJobState::Cancelled) State.Jobs.RemoveAt(Index);
            else ++Index;
    return Completed > 0;
}
FSSWProductionStock ManufacturingAI::Describe(const FEconomyState& State, FName Facility, FName Input, FName Output)
{
    FSSWProductionStock Stock;
    const auto Sum = [&State](FName Key)
    {
        double Total = 0;
        if (const auto* Inv = State.Inventories.Find(Key))
            for (const auto& Stack : Inv->Items) Total += Stack.Quantity * Stack.UnitTonnes;
        return Total;
    };
    Stock.Inventories.Add(TEXT("Ore Storage"), Sum(Input));
    Stock.Inventories.Add(TEXT("Finished Goods Storage"), Sum(Output));
    for (const auto& Job : State.Jobs)
        if (Job.Facility == Facility && Job.State == EIndustryJobState::Running)
        {
            Stock.RemainingSeconds = Job.Remaining;
            for (const auto& Product : Job.Products) Stock.ReservedOutput += Product.Quantity * Product.UnitTonnes;
        }
    return Stock;
}
