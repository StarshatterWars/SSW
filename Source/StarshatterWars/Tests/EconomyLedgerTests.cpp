#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include <limits>
#include "GameStructs_economy.h"
#include "ManufacturingAI.h"
#include "PlayerSaveGame.h"
#include "Kismet/GameplayStatics.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FEconomyLedgerTest, "Starshatter.Economy.InventoryIndustryAndTransactions",
    EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::EngineFilter)
bool FEconomyLedgerTest::RunTest(const FString&)
{
    FEconomyState State;
    const FName Owner(TEXT("Industry")), Ore(TEXT("magnetite")), Product(TEXT("concentrate"));
    for (FName Location : {FName(TEXT("Mine")), FName(TEXT("Hauler")), FName(TEXT("Input")), FName(TEXT("Output"))})
        TestNotNull(TEXT("Create inventory"), FEconomyLedger::CreateInventory(State, Location, Owner, TEXT("Janus"), 10));
    TestTrue(TEXT("Extracted ore enters mining depot"), FEconomyLedger::Deposit(State, TEXT("Mine"), Ore, 6, 1));
    TestFalse(TEXT("Reject capacity overflow"), FEconomyLedger::Deposit(State, TEXT("Mine"), Ore, 5, 1));
    TestFalse(TEXT("Reject NaN"), FEconomyLedger::Deposit(State, TEXT("Mine"), Ore, std::numeric_limits<double>::infinity(), 1));
    const FGuid StackId = State.Inventories.FindChecked(TEXT("Mine")).Items[0].Id;
    const FGuid Claim = FEconomyLedger::Reserve(State, Owner, TEXT("Mine"), Ore, 4);
    TestTrue(TEXT("Reserve pickup"), Claim.IsValid());
    TestEqual(TEXT("Reservation keeps physical stock at pickup"), FEconomyLedger::Quantity(State, TEXT("Mine"), Ore), 6.0);
    TestFalse(TEXT("Another hauler cannot reserve the same ore"), FEconomyLedger::Reserve(State, Owner, TEXT("Mine"), Ore, 4).IsValid());
    TestFalse(TEXT("Other owner cannot take ore"), FEconomyLedger::Transfer(State, TEXT("Other"), TEXT("Mine"), TEXT("Hauler"), Ore, 1, Claim));
    TestTrue(TEXT("Partial loading commits reserved stock"), FEconomyLedger::Transfer(State, Owner, TEXT("Mine"), TEXT("Hauler"), Ore, 2, Claim));
    TestEqual(TEXT("Destination not credited in flight"), FEconomyLedger::Quantity(State, TEXT("Input"), Ore), 0.0);
    TestTrue(TEXT("Remaining reserved stock loads"), FEconomyLedger::Transfer(State, Owner, TEXT("Mine"), TEXT("Hauler"), Ore, 2, Claim));
    TestFalse(TEXT("Consumed reservation cannot replay"), FEconomyLedger::Transfer(State, Owner, TEXT("Mine"), TEXT("Hauler"), Ore, 1, Claim));
    TestTrue(TEXT("Unloading credits refinery"), FEconomyLedger::Transfer(State, Owner, TEXT("Hauler"), TEXT("Input"), Ore, 4));
    TestEqual(TEXT("Original stack identity stable"), State.Inventories.FindChecked(TEXT("Mine")).Items[0].Id, StackId);
    FCommodityAmount Material; Material.Item = Ore; Material.Quantity = 4;
    FCommodityAmount Finished; Finished.Item = Product; Finished.Quantity = 2;
    const FGuid Job = FEconomyLedger::QueueJob(State, Owner, TEXT("Refinery"), TEXT("Input"), TEXT("Output"), TEXT("Concentrate"), {Material}, {Finished}, 10);
    TestTrue(TEXT("Queue refinery job"), Job.IsValid());
    TestEqual(TEXT("Offline facility pauses"), FEconomyLedger::AdvanceJobs(State, 10, [](FName) { return false; }), 0);
    TestEqual(TEXT("Input is not consumed while offline"), FEconomyLedger::Quantity(State, TEXT("Input"), Ore), 4.0);
    FEconomyLedger::AdvanceJobs(State, 4, [](FName) { return true; });
    TestEqual(TEXT("Materials consumed once at start"), FEconomyLedger::Quantity(State, TEXT("Input"), Ore), 0.0);
    TestEqual(TEXT("Unfinished goods not tradable"), FEconomyLedger::Quantity(State, TEXT("Output"), Product), 0.0);
    auto* Save = NewObject<UPlayerSaveGame>(); Save->PlayerInfo.Economy = State;
    TArray<uint8> Bytes;
    if (!TestTrue(TEXT("Save running job and stock"), UGameplayStatics::SaveGameToMemory(Save, Bytes))) return false;
    const auto* Loaded = Cast<UPlayerSaveGame>(UGameplayStatics::LoadGameFromMemory(Bytes));
    if (!TestNotNull(TEXT("Load economy"), Loaded)) return false;
    State = Loaded->PlayerInfo.Economy;
    TestEqual(TEXT("Running job retains identity"), State.Jobs[0].Id, Job);
    TestEqual(TEXT("Running job retains remaining time"), State.Jobs[0].Remaining, 6.0);
    TestEqual(TEXT("Complete exactly once"), FEconomyLedger::AdvanceJobs(State, 6, [](FName) { return true; }), 1);
    TestEqual(TEXT("Real concentrate output"), FEconomyLedger::Quantity(State, TEXT("Output"), Product), 2.0);
    TestEqual(TEXT("No duplicate completion"), FEconomyLedger::AdvanceJobs(State, 60, [](FName) { return true; }), 0);
    FEconomyLedger::CreateInventory(State, TEXT("BuyerHold"), TEXT("Pilot"), TEXT("Janus"), 1);
    State.Wallets.Add(TEXT("Pilot"), 1000);
    const FGuid Trade = FGuid::NewGuid();
    TestFalse(TEXT("Full hold leaves payment untouched"), FEconomyLedger::Trade(State, TEXT("Pilot"), Owner, TEXT("Output"), TEXT("BuyerHold"), Product, 2, 200, Trade));
    TestEqual(TEXT("Failed sale preserves wallet"), State.Wallets.FindRef(TEXT("Pilot")), 1000LL);
    State.Inventories.FindChecked(TEXT("BuyerHold")).CapacityTonnes = 10;
    TestTrue(TEXT("Atomic purchase transfers money and stock"), FEconomyLedger::Trade(State, TEXT("Pilot"), Owner, TEXT("Output"), TEXT("BuyerHold"), Product, 2, 200, Trade));
    TestEqual(TEXT("Buyer charged"), State.Wallets.FindRef(TEXT("Pilot")), 800LL);
    TestEqual(TEXT("Seller paid"), State.Wallets.FindRef(Owner), 200LL);
    TestFalse(TEXT("Duplicate payment rejected"), FEconomyLedger::Pay(State, TEXT("Pilot"), Owner, 200, Trade, TEXT("Retry")));
    const FGuid Blocked = FEconomyLedger::QueueJob(State, Owner, TEXT("Refinery"), TEXT("Input"), TEXT("Output"), TEXT("Concentrate"), {Material}, {Finished}, 10);
    FEconomyLedger::AdvanceJobs(State, 1, [](FName) { return true; });
    TestEqual(TEXT("Missing input blocks job"), State.Jobs.Last().State, EIndustryJobState::Blocked);
    TestTrue(TEXT("Owner can cancel blocked job"), FEconomyLedger::CancelJob(State, Owner, Blocked));
    TestFalse(TEXT("Cancel cannot replay"), FEconomyLedger::CancelJob(State, Owner, Blocked));
    return true;
}
#endif
