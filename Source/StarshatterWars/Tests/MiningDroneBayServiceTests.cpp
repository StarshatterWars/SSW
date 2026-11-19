#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "MiningDroneBayService.h"
#include "MiningDroneSaveTransaction.h"

namespace
{
void SpareFixture(FEconomyState& State, FName Hold, const FMiningDroneBayDefinition& Bay, int32 Count)
{
    FEconomyLedger::CreateInventory(State, Hold, FEconomyLedger::Pilot(), TEXT("Janus"), 20);
    FEconomyLedger::Deposit(State, Hold, DataTableUtils::MakeValidName(TEXT("mining drone")), 2, .5);
    FMiningDroneFleetState Fleet; Fleet.ShipDesign = Bay.ShipDesign; Fleet.Region = TEXT("Janek");
    for (int32 Index = 0; Index < Count; ++Index)
    { FMiningDroneState Drone; Drone.Id = FGuid::NewGuid(); Fleet.Drones.Add(Drone); }
    State.MiningDrones.Add(Hold, Fleet);
}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMiningDroneSpareTest, "Starshatter.Mining.Drones.CargoBackedSpareLoading",
    EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::EngineFilter)
bool FMiningDroneSpareTest::RunTest(const FString&)
{
    FEconomyState State; const FName Hold(TEXT("Ship/OwnedDroneBay"));
    const FName Item = DataTableUtils::MakeValidName(TEXT("mining drone"));
    FMiningDroneBayDefinition Bay; Bay.ShipDesign = TEXT("Prospector"); Bay.BayCount = 3;
    SpareFixture(State, Hold, Bay, 2); FText Reason;
    const auto First = State.MiningDrones.FindChecked(Hold).Drones[0].Id;
    TestFalse(TEXT("Spare loading cannot occur in flight"), MiningDroneBayService::Load(State, Hold, Bay, false, true, Reason));
    TestFalse(TEXT("A client cannot provision a bay"), MiningDroneBayService::Load(State, Hold, Bay, true, false, Reason));
    const FGuid Reservation = FEconomyLedger::Reserve(State, FEconomyLedger::Pilot(), Hold, Item, 2);
    TestFalse(TEXT("Reserved cargo cannot be installed as a spare"), MiningDroneBayService::Load(State, Hold, Bay, true, true, Reason));
    FEconomyLedger::Release(State, FEconomyLedger::Pilot(), Reservation);
    TestTrue(TEXT("Older two-drone fleet can fill the new third slot using actual cargo"), MiningDroneBayService::Load(State, Hold, Bay, true, true, Reason));
    const auto& Fleet = State.MiningDrones.FindChecked(Hold);
    TestEqual(TEXT("Exactly one new bay slot is filled"), Fleet.Drones.Num(), 3);
    TestEqual(TEXT("Existing drone identities are retained"), Fleet.Drones[0].Id, First);
    TestTrue(TEXT("New drone receives a distinct persistent identity"), Fleet.Drones[2].Id.IsValid() && Fleet.Drones[2].Id != First);
    TestEqual(TEXT("Installed drone begins docked"), Fleet.Drones[2].State, EMiningDroneState::Docked);
    TestEqual(TEXT("Exactly one cargo unit was consumed"), FEconomyLedger::Quantity(State, Hold, Item), 1.0);
    TestFalse(TEXT("A full bay cannot consume another spare"), MiningDroneBayService::Load(State, Hold, Bay, true, true, Reason));
    TestEqual(TEXT("Failed full-bay attempt leaves remaining cargo"), FEconomyLedger::Quantity(State, Hold, Item), 1.0);

    auto& Lost = State.MiningDrones.FindChecked(Hold).Drones[1]; Lost.State = EMiningDroneState::Lost;
    const FGuid LostIdentity = Lost.Id;
    TestTrue(TEXT("Lost slot is replaced before extending the array"), MiningDroneBayService::Load(State, Hold, Bay, true, true, Reason));
    TestEqual(TEXT("Replacement does not grow bay capacity"), State.MiningDrones.FindChecked(Hold).Drones.Num(), 3);
    TestTrue(TEXT("Replacement receives a fresh identity"), State.MiningDrones.FindChecked(Hold).Drones[1].Id != LostIdentity);
    TestEqual(TEXT("Replacement consumes the second cargo drone"), FEconomyLedger::Quantity(State, Hold, Item), 0.0);
    TestTrue(TEXT("No credits or unrelated transaction receipts were created"), State.Wallets.IsEmpty() && State.Journal.IsEmpty());

    FEconomyState Foreign; SpareFixture(Foreign, Hold, Bay, 2);
    Foreign.Inventories.FindChecked(Hold).Owner = TEXT("Other pilot");
    TestFalse(TEXT("Another pilot's cargo cannot supply a spare"), MiningDroneBayService::Load(Foreign, Hold, Bay, true, true, Reason));
    FEconomyState WrongUnits; SpareFixture(WrongUnits, Hold, Bay, 2);
    WrongUnits.Inventories.FindChecked(Hold).Items[0].UnitTonnes = .1;
    TestFalse(TEXT("A different cargo mass cannot masquerade as a complete drone"), MiningDroneBayService::Load(WrongUnits, Hold, Bay, true, true, Reason));
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMiningDroneSpareRollbackTest, "Starshatter.Mining.Drones.SpareSaveFailureRestoresCargoAndBay",
    EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::EngineFilter)
bool FMiningDroneSpareRollbackTest::RunTest(const FString&)
{
    FEconomyState State; const FName Hold(TEXT("Ship/SpareRollback"));
    const FName Item = DataTableUtils::MakeValidName(TEXT("mining drone"));
    FMiningDroneBayDefinition Bay; Bay.ShipDesign = TEXT("Rockhopper"); Bay.BayCount = 10;
    SpareFixture(State, Hold, Bay, 10);
    auto& Lost = State.MiningDrones.FindChecked(Hold).Drones[3]; Lost.State = EMiningDroneState::Lost;
    const FGuid OldIdentity = Lost.Id; const auto Before = State; FText Reason;
    TestTrue(TEXT("Replacement stages against real cargo"), MiningDroneBayService::Load(State, Hold, Bay, true, true, Reason));
    int32 Saves = 0, CargoUpdates = 0; const TArray<TPair<Asteroid*, double>> NoExtraction;
    TestFalse(TEXT("Failed persistence rolls back the complete mutation"), MiningDroneSaveTransaction::Commit(State, Before, NoExtraction,
        [&]() { ++Saves; return false; }, [&]() { ++CargoUpdates; }));
    TestEqual(TEXT("One save attempt only"), Saves, 1);
    TestEqual(TEXT("Cargo adapter is updated then restored"), CargoUpdates, 2);
    TestEqual(TEXT("Both unconsumed cargo units return"), FEconomyLedger::Quantity(State, Hold, Item), 2.0);
    TestEqual(TEXT("Lost slot retains its old identity on failed save"), State.MiningDrones.FindChecked(Hold).Drones[3].Id, OldIdentity);
    TestEqual(TEXT("Failed installation leaves the old lost state"), State.MiningDrones.FindChecked(Hold).Drones[3].State, EMiningDroneState::Lost);
    TestTrue(TEXT("Retry can install the same remaining spare once"), MiningDroneBayService::Load(State, Hold, Bay, true, true, Reason));
    TestTrue(TEXT("Successful retry gets a new identity"), State.MiningDrones.FindChecked(Hold).Drones[3].Id != OldIdentity);
    TestEqual(TEXT("Retry consumes exactly one unit"), FEconomyLedger::Quantity(State, Hold, Item), 1.0);
    return true;
}
#endif
