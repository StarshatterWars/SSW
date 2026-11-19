#pragma once
#include "GameStructs_economy.h"
#include "Engine/DataTable.h"

// Cargo-backed bay changes share the same native inventory and save boundary as mining.
namespace MiningDroneBayService
{
inline bool CanLoad(const FEconomyState& State, FName HoldKey, const FMiningDroneBayDefinition& Bay,
    bool Docked, bool Authority, FText& Reason)
{
    if (!Docked || !Authority)
    { Reason = INVTEXT("Dock your fitted mining ship before loading a spare drone."); return false; }
    const auto* Hold = State.Inventories.Find(HoldKey);
    const auto* Fleet = State.MiningDrones.Find(HoldKey);
    if (!Hold || Hold->Owner != FEconomyLedger::Pilot() || !Fleet || Fleet->ShipDesign != Bay.ShipDesign ||
        Bay.BayCount < 1 || Bay.BayCount > 12 || Fleet->Drones.Num() > 12)
    { Reason = INVTEXT("A persistent owned ship and matching fitted drone bay are required."); return false; }
    int32 AvailableSlots = 0;
    bool LostSlot = false;
    for (const auto& Drone : Fleet->Drones)
    {
        if (Drone.State == EMiningDroneState::Lost) LostSlot = true;
        else ++AvailableSlots;
    }
    if (AvailableSlots >= Bay.BayCount || (!LostSlot && Fleet->Drones.Num() >= Bay.BayCount))
    { Reason = INVTEXT("The drone bay is full. Only a lost or unfilled bay slot can accept a spare."); return false; }
    const FName Item = DataTableUtils::MakeValidName(TEXT("mining drone"));
    const auto* Stack = Hold->Items.FindByPredicate([Item](const auto& Entry) { return Entry.Type == Item; });
    if (!Stack || !FMath::IsFinite(Stack->Quantity) || Stack->Quantity < 1 || !FMath::IsNearlyEqual(Stack->UnitTonnes, .5) ||
        FEconomyLedger::Available(State, HoldKey, Item) < 1)
    { Reason = INVTEXT("Carry one unreserved Mining Drone (0.5 t) in this ship's cargo hold."); return false; }
    Reason = INVTEXT("Load one spare Mining Drone from cargo into an empty or lost bay slot (0.5 t)."); return true;
}

inline bool Load(FEconomyState& State, FName HoldKey, const FMiningDroneBayDefinition& Bay,
    bool Docked, bool Authority, FText& Reason)
{
    if (!CanLoad(State, HoldKey, Bay, Docked, Authority, Reason)) return false;
    const FName Item = DataTableUtils::MakeValidName(TEXT("mining drone"));
    if (!FEconomyLedger::Withdraw(State, FEconomyLedger::Pilot(), HoldKey, Item, 1))
    { Reason = INVTEXT("The spare drone is unavailable or reserved by another operation."); return false; }
    auto& Fleet = State.MiningDrones.FindChecked(HoldKey);
    FMiningDroneState NewDrone; NewDrone.Id = FGuid::NewGuid();
    if (auto* Lost = Fleet.Drones.FindByPredicate([](const auto& Drone) { return Drone.State == EMiningDroneState::Lost; }))
        *Lost = NewDrone;
    else Fleet.Drones.Add(NewDrone);
    Reason = INVTEXT("Spare Mining Drone loaded into the bay. One drone was removed from cargo."); return true;
}
}
