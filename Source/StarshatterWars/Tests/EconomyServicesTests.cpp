#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "GameStructs_economy.h"
#include "GameStructs_products.h"
#include "ManufacturingAI.h"
#include "StationServices.h"
#include "SimSystem.h"
#include "SimComponent.h"
#include "ShipSystemIcon.h"
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FEconomyServicesTest, "Starshatter.Economy.ManifestRecipesAndStationServices",
    EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::EngineFilter)
bool FEconomyServicesTest::RunTest(const FString&)
{
    FEconomyState State;
    const FName Owner = FEconomyLedger::Pilot(), Hold(TEXT("Hold"));
    FEconomyLedger::CreateInventory(State, Hold, Owner, TEXT("Janus"), 20);
    FEconomyLedger::Deposit(State, Hold, TEXT("avionics"), 2, .2);
    const FGuid ElectronicsId = State.Inventories.FindChecked(Hold).Items[0].Id;
    FCommodityAmount Ore; Ore.Item = TEXT("Magnetite"); Ore.Quantity = 4;
    TestTrue(TEXT("Manifest joins existing cargo"), FEconomyLedger::ReconcileManifest(State, Hold, {Ore}));
    TestEqual(TEXT("Other cargo preserved"), FEconomyLedger::Quantity(State, Hold, TEXT("avionics")), 2.0);
    TestEqual(TEXT("Unrelated item identity preserved"), State.Inventories.FindChecked(Hold).Items[0].Id, ElectronicsId);
    const FGuid Claim = FEconomyLedger::Reserve(State, Owner, Hold, Ore.Item, 3);
    Ore.Quantity = 2;
    TestFalse(TEXT("Manifest cannot erase reserved cargo"), FEconomyLedger::ReconcileManifest(State, Hold, {Ore}));
    TestEqual(TEXT("Failed reconciliation preserves amount"), FEconomyLedger::Quantity(State, Hold, Ore.Item), 4.0);
    FEconomyLedger::Release(State, Owner, Claim);
    TestTrue(TEXT("Clear old manifest without erasing other stacks"), FEconomyLedger::ReconcileManifest(State, Hold, {}));
    TestEqual(TEXT("Purchased goods survive manifest clearing"), FEconomyLedger::Quantity(State, Hold, TEXT("avionics")), 2.0);
    Ore.Quantity = 30;
    TestFalse(TEXT("Over-capacity reconciliation is atomic"), FEconomyLedger::ReconcileManifest(State, Hold, {Ore}));
    TestEqual(TEXT("Capacity rejection leaves original hold"), FEconomyLedger::Used(State.Inventories.FindChecked(Hold)), .4);
    const FName Input(TEXT("Input")), Output(TEXT("Output"));
    FEconomyLedger::CreateInventory(State, Input, TEXT("Factory"), TEXT("Janus"), 100);
    FEconomyLedger::CreateInventory(State, Output, TEXT("Factory"), TEXT("Janus"), 100);
    FSSWProductionRecipe Recipe; Recipe.InputTonnes = 3; Recipe.OutputTonnes = 1; Recipe.SecondsPerBatch = 10;
    Recipe.CargoItem = TEXT("avionics"); Recipe.OutputQuantity = 5;
    FCommodityAmount Iron; Iron.Item = TEXT("Magnetite"); Iron.Quantity = 2;
    FCommodityAmount Quartz; Quartz.Item = TEXT("Quartz"); Quartz.Quantity = 1;
    Recipe.Inputs = {Iron, Quartz};
    FEconomyLedger::Deposit(State, Input, Iron.Item, 10, 1);
    TArray<FCommodityAmount> Materials, Products;
    TestFalse(TEXT("Excess iron cannot replace quartz"), ManufacturingAI::BuildBatch(State, Input, Recipe, Materials, Products));
    FEconomyLedger::Deposit(State, Input, Quartz.Item, 1, 1);
    TestTrue(TEXT("Every precise ingredient creates valid batch"), ManufacturingAI::BuildBatch(State, Input, Recipe, Materials, Products));
    TestEqual(TEXT("Product quantity uses catalog units"), Products[0].Quantity, 5.0);
    TestEqual(TEXT("Product unit mass preserves output tonnes"), Products[0].UnitTonnes, .2);
    const FGuid Job = FEconomyLedger::QueueJob(State, TEXT("Factory"), TEXT("Refinery"), Input, Output, TEXT("avionics"), Materials, Products, 10);
    TestTrue(TEXT("Exact recipe job queued"), Job.IsValid());
    TestEqual(TEXT("Job completes with every material consumed"), FEconomyLedger::AdvanceJobs(State, 10, [](FName){return true;}), 1);
    TestEqual(TEXT("Quartz consumed exactly once"), FEconomyLedger::Quantity(State, Input, Quartz.Item), 0.0);
    TestEqual(TEXT("Five manufactured units delivered"), FEconomyLedger::Quantity(State, Output, TEXT("avionics")), 5.0);
    State.Wallets.Add(Owner, 1000); State.Wallets.Add(TEXT("Factory"), 0);
    TestTrue(TEXT("Whole stack sale succeeds"), FEconomyLedger::Trade(State, Owner, TEXT("Factory"), Output, Hold, TEXT("avionics"), 5, 100, FGuid::NewGuid()));
    // Existing avionics merge into one stack; the original destination identity is retained.
    TestEqual(TEXT("Merging goods keeps receiving stack ID"), State.Inventories.FindChecked(Hold).Items[0].Id, ElectronicsId);
    FEconomyLedger::CreateInventory(State, TEXT("NewHold"), TEXT("Other"), TEXT("Janus"), 10);
    State.Wallets.Add(TEXT("Other"), 1000);
    TestTrue(TEXT("Entire stack changes owner atomically"), FEconomyLedger::Trade(State, TEXT("Other"), Owner, Hold, TEXT("NewHold"), TEXT("avionics"), 7, 100, FGuid::NewGuid()));
    TestEqual(TEXT("Full-stack purchase preserves item identity"), State.Inventories.FindChecked(TEXT("NewHold")).Items[0].Id, ElectronicsId);
    SimSystem DamagedSystem(SYSTEM_CATEGORY::DRIVE, 0, "Workshop test drive", 10);
    ComponentDesign PartDesign; PartDesign.name = "Grade IV drive controller";
    PartDesign.base_cost = 2000; PartDesign.performance_multiplier = 1.1;
    PartDesign.power_demand_multiplier = .95; PartDesign.spares = 2;
    PartDesign.affects = SimComponent::DAMAGE_EFFICIENCY | SimComponent::DAMAGE_SAFETY | SimComponent::DAMAGE_STABILITY;
    auto* DamagedPart = new SimComponent(PartDesign, &DamagedSystem);
    DamagedSystem.GetComponents().append(DamagedPart);
    DamagedPart->ApplyDamage(100);
    DamagedPart->Repair(); DamagedPart->ExecMaintFrame(1000);
    TestTrue(TEXT("Emergency engineering repair can leave a jerry-rig"), DamagedPart->IsJerried());
    DamagedSystem.SetPowerLevel(40);
    DamagedSystem.RestoreAtStation();
    TestEqual(TEXT("Workshop restores component to full condition"), DamagedPart->Availability(), 100.f);
    TestFalse(TEXT("Workshop removes emergency jerry-rig penalty"), DamagedPart->IsJerried());
    TestTrue(TEXT("Workshop system status is nominal"), DamagedSystem.GetStatus() == SYSTEM_STATUS::NOMINAL);
    TestEqual(TEXT("Workshop retains installed component grade"), DamagedPart->PerformanceMultiplier(), 1.1);
    TestEqual(TEXT("Workshop does not consume shipboard spares"), DamagedPart->SpareCount(), 2);
    TestTrue(TEXT("Workshop retains pilot power allocation"), FMath::IsNearlyEqual(DamagedSystem.GetPowerLevel(), 40.0, .001));
    SimSystem BareSystem(SYSTEM_CATEGORY::SENSOR, 0, "Bare sensor", 10);
    BareSystem.ApplyDamage(90); BareSystem.RestoreAtStation();
    TestEqual(TEXT("Workshop repairs systems without named components"), BareSystem.GetAvailability(), 100.0);
    TestFalse(TEXT("Station services unavailable outside a docked simulation"), FStationServices::IsDocked());
    TestEqual(TEXT("No unlocated station inventory"), FStationServices::Inventory(TEXT("Player Storage")), NAME_None);
    TestTrue(TEXT("Mineral image key resolves"), SShipSystemIcon::ForCatalogImage(TEXT("Mineral")) == EShipSystemIcon::Mineral);
    TestTrue(TEXT("Unknown image gracefully uses cargo icon"), SShipSystemIcon::ForCatalogImage(TEXT("Future image")) == EShipSystemIcon::Cargo);
    return true;
}
#endif
