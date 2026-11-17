#include "StationServices.h"
#include "EconomyRuntime.h"
#include "StarshatterPlayerSubsystem.h"
#include "Ship.h"
#include "GameStructs_products.h"
#include "Engine/DataTable.h"
#include "ShipDesign.h"
#include "SimSystem.h"
#include "SimComponent.h"
#include "Sim.h"
#include "SimRegion.h"
#include "StarSystem.h"
#include "Engine/GameInstance.h"

bool FStationServices::IsDocked()
{
    auto* Simulation = Sim::GetSim(); const auto* Craft = Simulation ? Simulation->GetPlayerShip() : nullptr;
    const auto* Base = Craft ? Craft->GetCarrier() : nullptr;
    return Simulation && !Simulation->IsNetworked() && Simulation->HasSimulationAuthority() && Craft && !Craft->IsDead() && !Craft->IsDying() &&
        Craft->GetFlightPhase() <= EOPSMode::ALERT && Base && !Base->IsDead() && !Base->IsDying() && !Base->HasNetworkPilot();
}
namespace
{
const FSSWProductionFacility* DockedFacility()
{
    if (!FStationServices::IsDocked()) return nullptr;
    auto* Craft = Sim::GetSim()->GetPlayerShip();
    const auto* Table = LoadObject<UDataTable>(nullptr, TEXT("/Game/Game/DT_ProductionFacilities.DT_ProductionFacilities"));
    if (!Table || Table->GetRowStruct() != FSSWProductionFacility::StaticStruct()) return nullptr;
    for (const auto& Pair : Table->GetRowMap())
    {
        const auto* Site = reinterpret_cast<const FSSWProductionFacility*>(Pair.Value);
        auto* Region = Craft->GetRegion();
        if (Site->Facility == UTF8_TO_TCHAR(Craft->GetCarrier()->GetName()) && Region && Region->GetSystem() &&
            Site->System == UTF8_TO_TCHAR(Region->GetSystem()->GetName())) return Site;
    }
    return nullptr;
}
}
bool FStationServices::HasIndustry() { return DockedFacility() != nullptr; }
const FSSWProductionFacility* FStationServices::Facility() { return DockedFacility(); }
FName FStationServices::Inventory(const TCHAR* Compartment)
{
    if (!IsDocked()) return NAME_None;
    auto* Craft = Sim::GetSim()->GetPlayerShip(); auto* Region = Craft->GetRegion();
    const FString System = Region && Region->GetSystem() ? UTF8_TO_TCHAR(Region->GetSystem()->GetName()) : TEXT("");
    return FEconomyRuntime::Warehouse(System, UTF8_TO_TCHAR(Craft->GetCarrier()->GetName()), Compartment);
}
bool FStationServices::MoveCargo(UGameInstance* Instance, FName Item, bool ToStation, const TCHAR* Compartment, FText& Reason)
{
    if (!Instance || !IsDocked() || Item.IsNone() || Item.ToString().StartsWith(TEXT("Courier/")))
    { Reason = INVTEXT("Dock at an NPC base. Sealed courier packages must be delivered through Contracts."); return false; }
    const FString Area(Compartment);
    const auto* Site = DockedFacility();
    if (Area != TEXT("Player Storage") && Area != TEXT("Player Input") && Area != TEXT("Player Output")) return false;
    if (Area != TEXT("Player Storage") && !Site) { Reason = INVTEXT("This base has no production service."); return false; }
    // Industry output is collection-only: never allow a job's reserved output to be replaced by cargo.
    if (ToStation && FString(Compartment) == TEXT("Player Output")) return false;
    auto* Players = Instance->GetSubsystem<UStarshatterPlayerSubsystem>();
    auto* Craft = Sim::GetSim()->GetPlayerShip(); Craft->SyncCargoInventory();
    auto& State = Sim::GetSim()->GetEconomy(); const auto Before = State;
    const FName Store = Inventory(Compartment), Hold = Craft->GetCargoInventoryKey();
    const auto* ShipInventory = State.Inventories.Find(Hold);
    if (!Players || !ShipInventory) return false;
    FEconomyLedger::CreateInventory(State, Store, FEconomyLedger::Pilot(), ShipInventory->System, Site && Area != TEXT("Player Storage") ? Site->CapacityTonnes : 1000000);
    const FName Source = ToStation ? Hold : Store, Destination = ToStation ? Store : Hold;
    double Count = FEconomyLedger::Available(State, Source, Item);
    if (!ToStation)
    {
        const auto* SourceInventory = State.Inventories.Find(Source);
        const auto* Stack = SourceInventory ? SourceInventory->Items.FindByPredicate([Item](const auto& Entry) { return Entry.Type == Item; }) : nullptr;
        if (!Stack || Stack->UnitTonnes <= 0) return false;
        Count = FMath::Min(Count, FMath::Max(0.0, State.Inventories.FindChecked(Hold).CapacityTonnes - FEconomyLedger::Used(State.Inventories.FindChecked(Hold))) / Stack->UnitTonnes);
    }
    if (const auto* Load = Craft->GetCargoManifest().GetPtrOrNull(); Load && Load->Unit != TEXT("t") &&
        DataTableUtils::MakeValidName(Load->Item) == Item) Count = FMath::FloorToDouble(Count);
    if (Count <= UE_SMALL_NUMBER || !FEconomyLedger::Transfer(State, FEconomyLedger::Pilot(), Source, Destination, Item, Count) || !Players->SaveActivePlayerProgress())
    { State = Before; Reason = INVTEXT("Transfer unavailable: insufficient space, reserved cargo or save failure."); return false; }
    Craft->ApplyCargoInventoryToManifest(); Reason = INVTEXT("Cargo transferred to the selected inventory."); return true;
}
int64 FStationServices::RepairQuote(Ship& Craft)
{
    if (!Craft.Design()) return 0;
    double Cost = FMath::Max(0.0, double(Craft.Design()->integrity) - Craft.GetIntegrity());
    auto& Systems = Craft.GetSystems();
    for (int Index = 0; Index < Systems.size(); ++Index)
    {
        auto* System = Systems[Index]; if (!System) continue;
        auto& Parts = System->GetComponents();
        for (int PartIndex = 0; PartIndex < Parts.size(); ++PartIndex)
        {
            const auto* Part = Parts[PartIndex]; if (!Part) continue;
            const double Damage = FMath::Clamp(1.0 - Part->Availability() / 100.0, 0.0, 1.0);
            if (Damage > 0 || Part->IsJerried() || Part->GetStatus() != SYSTEM_STATUS::NOMINAL)
                Cost += FMath::Max(1.0, double(Part->Cost()) * FMath::Max(Damage, Part->IsJerried() ? .1 : .01));
        }
        if (Parts.size() == 0 && System->GetAvailability() < 100)
            Cost += FMath::Max(1.0, System->GetMaxValue() * (1.0 - System->GetAvailability() / 100.0));
    }
    return FMath::IsFinite(Cost) ? FMath::CeilToInt64(FMath::Clamp(Cost, 0.0, 1.e15)) : MAX_int64;
}
int64 FStationServices::FullRepairCost()
{
    return IsDocked() ? RepairQuote(*Sim::GetSim()->GetPlayerShip()) : 0;
}
bool FStationServices::RepairAll(UGameInstance* Instance, FText& Reason)
{
    const int64 Cost = FullRepairCost();
    if (!Instance || !IsDocked() || Cost <= 0) { Reason = INVTEXT("Dock with a damaged fighter or ship to use the repair service."); return false; }
    auto* Players = Instance->GetSubsystem<UStarshatterPlayerSubsystem>(); if (!Players) return false;
    auto& State = Sim::GetSim()->GetEconomy(); const auto Before = State;
    if (!FEconomyLedger::Pay(State, FEconomyLedger::Pilot(), TEXT("Station repair service"), Cost, FGuid::NewGuid(), TEXT("Full ship repairs")) || !Players->SaveActivePlayerProgress())
    { State = Before; Reason = INVTEXT("Repair failed. No credits were charged."); return false; }
    if (!Sim::GetSim()->GetPlayerShip()->RepairAtStation())
    { State = Before; Players->SaveActivePlayerProgress(); Reason = INVTEXT("Repair requires docking. Payment was reversed."); return false; }
    Reason = INVTEXT("Hull and all installed systems fully repaired. Installed grades and loadout retained."); return true;
}
