#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "BlueprintCatalogSubsystem.h"
#include "EconomyRuntime.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FIndustryPreflightTest, "Starshatter.Economy.PlayerIndustryPreflightAndCancellation",
    EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::EngineFilter)
bool FIndustryPreflightTest::RunTest(const FString&)
{
    FEconomyState State;
    const FName Owner = FEconomyLedger::Pilot();
    FSSWProductionFacility Site;
    Site.System = TEXT("Janus"); Site.Facility = TEXT("Test Works"); Site.Recipe = TEXT("TestRecipe"); Site.CapacityTonnes = 100;
    const FName Input = FEconomyRuntime::Warehouse(Site.System, Site.Facility, TEXT("Player Input"));
    const FName Output = FEconomyRuntime::Warehouse(Site.System, Site.Facility, TEXT("Player Output"));
    FEconomyLedger::CreateInventory(State, Input, Owner, TEXT("Janus"), 100);
    FEconomyLedger::CreateInventory(State, Output, Owner, TEXT("Janus"), 100);
    FSSWProductionRecipe Recipe;
    Recipe.InputTonnes = 10; Recipe.OutputTonnes = 2; Recipe.SecondsPerBatch = 10;
    Recipe.CargoItem = TEXT("concentrate"); Recipe.OutputQuantity = 4;
    FCommodityAmount Ore; Ore.Item = TEXT("Magnetite"); Ore.Quantity = 10; Ore.UnitTonnes = 1;
    Recipe.Inputs.Add(Ore);
    const FGuid Blueprint = FEconomyLedger::GrantBlueprint(State, Owner, Site.Recipe, false, 2, .1, .2);
    const FGuid Receipt = FGuid::NewGuid(); State.BlueprintReceipts.Add(Blueprint, Receipt); State.CommittedTransactions.Add(Receipt);
    FEconomyLedger::Deposit(State, Input, Ore.Item, 18, 1);
    FPlayerIndustryQuote Quote; FText Reason;
    TestTrue(TEXT("Discounted stock suffices for two efficient runs"), UBlueprintCatalogSubsystem::QuotePlayerJob(State, Site, Recipe, Blueprint, 2, Quote, Reason));
    if (Quote.Materials.IsEmpty() || Quote.Products.IsEmpty()) return false;
    TestEqual(TEXT("Preview includes total input"), Quote.Materials[0].Quantity, 18.0);
    TestEqual(TEXT("Preview includes total products"), Quote.Products[0].Quantity, 8.0);
    TestEqual(TEXT("Preview includes time efficiency"), Quote.Duration, 16.0);
    TestEqual(TEXT("Preview does not install jobs"), State.Jobs.Num(), 0);
    TestEqual(TEXT("Preview does not consume licensed runs"), State.Blueprints.FindChecked(Blueprint).RunsRemaining, 2);
    const FGuid Reservation = FEconomyLedger::Reserve(State, Owner, Input, Ore.Item, 1);
    TestFalse(TEXT("Reserved inputs cannot fund manufacturing"), UBlueprintCatalogSubsystem::QuotePlayerJob(State, Site, Recipe, Blueprint, 2, Quote, Reason));
    TestTrue(TEXT("Blocked preview explains the input shortage"), Reason.ToString().Contains(TEXT("INPUT")));
    FEconomyLedger::Release(State, Owner, Reservation);
    State.Inventories.FindChecked(Output).ReservedOutputTonnes = 99;
    TestFalse(TEXT("Already reserved output space cannot be promised twice"), UBlueprintCatalogSubsystem::QuotePlayerJob(State, Site, Recipe, Blueprint, 2, Quote, Reason));
    State.Inventories.FindChecked(Output).ReservedOutputTonnes = 0;
    TestFalse(TEXT("Copy run limit is enforced before installation"), UBlueprintCatalogSubsystem::QuotePlayerJob(State, Site, Recipe, Blueprint, 3, Quote, Reason));
    FCommodityAmount Product; Product.Item = TEXT("concentrate"); Product.Quantity = 4; Product.UnitTonnes = .5;
    const FGuid Job = FEconomyLedger::QueueBlueprintJob(State, Owner, FName(*Site.Facility), Input, Output, Blueprint, 2, {Ore}, {Product}, 10);
    TestTrue(TEXT("Licensed job installs"), Job.IsValid());
    TestFalse(TEXT("Other owners cannot cancel a job"), FEconomyLedger::CancelJob(State, TEXT("Intruder"), Job));
    FEconomyLedger::AdvanceJobs(State, 1, [](FName){ return true; });
    TestEqual(TEXT("Starting consumes materials once"), FEconomyLedger::Quantity(State, Input, Ore.Item), 0.0);
    TestEqual(TEXT("Starting consumes licensed runs"), State.Blueprints.FindChecked(Blueprint).RunsRemaining, 0);
    TestEqual(TEXT("Running job reserves destination mass"), State.Inventories.FindChecked(Output).ReservedOutputTonnes, 4.0);
    TestTrue(TEXT("Owner can cancel running job"), FEconomyLedger::CancelJob(State, Owner, Job));
    TestEqual(TEXT("Cancellation releases output reservation"), State.Inventories.FindChecked(Output).ReservedOutputTonnes, 0.0);
    TestEqual(TEXT("Cancellation creates no product"), FEconomyLedger::Quantity(State, Output, Product.Item), 0.0);
    TestEqual(TEXT("Cancellation does not refund ore"), FEconomyLedger::Quantity(State, Input, Ore.Item), 0.0);
    TestEqual(TEXT("Cancellation does not refund licensed runs"), State.Blueprints.FindChecked(Blueprint).RunsRemaining, 0);
    const FGuid Original = FEconomyLedger::GrantBlueprint(State, Owner, Site.Recipe, true, 0);
    State.BlueprintReceipts.Add(Original, Receipt);
    FEconomyLedger::Deposit(State, Input, Ore.Item, 10, 1);
    const FGuid Queued = FEconomyLedger::QueueBlueprintJob(State, Owner, FName(*Site.Facility), Input, Output, Original, 1, {Ore}, {Product}, 10);
    TestTrue(TEXT("Queued job can be cancelled"), FEconomyLedger::CancelJob(State, Owner, Queued));
    TestEqual(TEXT("Queued cancellation leaves inputs untouched"), FEconomyLedger::Quantity(State, Input, Ore.Item), 10.0);
    FEconomyLedger::QueueBlueprintJob(State, Owner, FName(*Site.Facility), Input, Output, Original, 1, {Ore}, {Product}, 10);
    TestEqual(TEXT("Normal completion still runs automatically"), FEconomyLedger::AdvanceJobs(State, 10, [](FName){return true;}), 1);
    TestEqual(TEXT("Completed output is deposited without a claim step"), FEconomyLedger::Quantity(State, Output, Product.Item), 4.0);
    return true;
}
#endif
