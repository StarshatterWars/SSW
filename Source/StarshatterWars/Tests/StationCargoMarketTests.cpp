#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "StationServices.h"
#include <limits>

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FStationCargoQuantityTest,"Starshatter.Economy.StationServices.PartialCargoCapacityAndReservations",
    EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::EngineFilter)
bool FStationCargoQuantityTest::RunTest(const FString&)
{
    FEconomyState State;
    const FName Pilot=FEconomyLedger::Pilot(),Ore(TEXT("magnetite"));
    FEconomyLedger::CreateInventory(State,TEXT("Hold"),Pilot,TEXT("Janus"),20);
    FEconomyLedger::CreateInventory(State,TEXT("Storage"),Pilot,TEXT("Janus"),10);
    FEconomyLedger::Deposit(State,TEXT("Hold"),Ore,12,1);
    FEconomyLedger::Deposit(State,TEXT("Storage"),Ore,2,1);
    const FGuid Reserved=FEconomyLedger::Reserve(State,Pilot,TEXT("Hold"),Ore,9);
    TestTrue(TEXT("Pickup reservation exists"),Reserved.IsValid());
    TestEqual(TEXT("Transfer quote excludes reserved cargo"),FStationServices::CargoTransferLimit(State,TEXT("Hold"),TEXT("Storage"),Ore,0,false),3.0);
    State.Inventories.FindChecked(TEXT("Storage")).ReservedOutputTonnes=6.5;
    TestEqual(TEXT("Future production output reserves destination space"),FStationServices::CargoTransferLimit(State,TEXT("Hold"),TEXT("Storage"),Ore,0,false),1.5);
    TestEqual(TEXT("Packaged goods round down to whole units"),FStationServices::CargoTransferLimit(State,TEXT("Hold"),TEXT("Storage"),Ore,0,true),1.0);
    TestTrue(TEXT("Requested fractional ore transfer succeeds"),FEconomyLedger::Transfer(State,Pilot,TEXT("Hold"),TEXT("Storage"),Ore,1.25));
    TestEqual(TEXT("Only requested cargo leaves hold"),FEconomyLedger::Quantity(State,TEXT("Hold"),Ore),10.75);
    TestEqual(TEXT("Only requested cargo reaches storage"),FEconomyLedger::Quantity(State,TEXT("Storage"),Ore),3.25);
    TestEqual(TEXT("Existing reserved cargo remains reserved"),State.Reservations.FindChecked(Reserved).Quantity,9.0);
    TestEqual(TEXT("Courier package cannot use ordinary transfer"),FStationServices::CargoTransferLimit(State,TEXT("Hold"),TEXT("Storage"),TEXT("Courier/Test"),0,false),0.0);
    TestEqual(TEXT("Same inventory is not a transfer"),FStationServices::CargoTransferLimit(State,TEXT("Hold"),TEXT("Hold"),Ore,0,false),0.0);
    TestEqual(TEXT("New personal storage respects its capacity"),FStationServices::CargoTransferLimit(State,TEXT("Hold"),TEXT("NewStore"),Ore,.5,false),.5);
    State.Inventories.FindChecked(TEXT("Storage")).Owner=TEXT("Other pilot");
    TestEqual(TEXT("Cannot transfer into another owner's store"),FStationServices::CargoTransferLimit(State,TEXT("Hold"),TEXT("Storage"),Ore,0,false),0.0);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FStationMarketPersistenceTest,"Starshatter.Economy.StationServices.MarketPersistenceReceiptsAndFunding",
    EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::EngineFilter)
bool FStationMarketPersistenceTest::RunTest(const FString&)
{
    FEconomyState State;
    const FName Pilot=FEconomyLedger::Pilot(),NPC(TEXT("Civilian industry")),Ore(TEXT("magnetite"));
    FEconomyLedger::CreateInventory(State,TEXT("NPC"),NPC,TEXT("Janus"),100);
    FEconomyLedger::CreateInventory(State,TEXT("Hold"),Pilot,TEXT("Janus"),10);
    FEconomyLedger::Deposit(State,TEXT("NPC"),Ore,20,1);
    State.Wallets.Add(Pilot,1000);
    FStationMarketQuote Quote;Quote.bAllowed=true;Quote.Item=Ore;Quote.Quantity=1.25;Quote.TotalCredits=125;
    Quote.Buyer=Pilot;Quote.Seller=NPC;Quote.Source=TEXT("NPC");Quote.Destination=TEXT("Hold");
    FText Reason;int Saves=0;
    const FGuid Receipt=FGuid::NewGuid();
    State.Inventories.FindChecked(TEXT("Hold")).ReservedOutputTonnes=9;
    TestFalse(TEXT("Reserved output space prevents a market purchase"),FStationServices::CommitMarketTrade(State,Quote,Receipt,[&](){++Saves;return true;},Reason));
    TestEqual(TEXT("Capacity failure never attempts save"),Saves,0);
    State.Inventories.FindChecked(TEXT("Hold")).ReservedOutputTonnes=0;
    const FGuid StockReservation=FEconomyLedger::Reserve(State,NPC,TEXT("NPC"),Ore,19);
    TestFalse(TEXT("Reserved NPC stock is unavailable for purchase"),FStationServices::CommitMarketTrade(State,Quote,Receipt,[&](){++Saves;return true;},Reason));
    TestEqual(TEXT("Stock failure never attempts save"),Saves,0);
    FEconomyLedger::Release(State,NPC,StockReservation);
    TestFalse(TEXT("Failed persistence rejects transaction"),FStationServices::CommitMarketTrade(State,Quote,Receipt,[&](){++Saves;return false;},Reason));
    TestEqual(TEXT("Exactly one save attempt, no compensatory save"),Saves,1);
    TestEqual(TEXT("Failed save restores pilot credits"),State.Wallets.FindRef(Pilot),1000LL);
    TestEqual(TEXT("Failed save restores NPC stock"),FEconomyLedger::Quantity(State,TEXT("NPC"),Ore),20.0);
    TestEqual(TEXT("Failed save removes purchased cargo"),FEconomyLedger::Quantity(State,TEXT("Hold"),Ore),0.0);
    TestFalse(TEXT("Failed save does not commit receipt"),State.CommittedTransactions.Contains(Receipt));
    TestEqual(TEXT("Failed save restores journal"),State.Journal.Num(),0);
    TestTrue(TEXT("Retry after failed save transfers and saves exactly once"),FStationServices::CommitMarketTrade(State,Quote,Receipt,[&](){++Saves;return true;},Reason));
    TestEqual(TEXT("Fractional ore retained exactly"),FEconomyLedger::Quantity(State,TEXT("Hold"),Ore),1.25);
    TestEqual(TEXT("Purchase funds NPC wallet"),State.Wallets.FindRef(NPC),125LL);
    TestEqual(TEXT("Purchase debits pilot wallet"),State.Wallets.FindRef(Pilot),875LL);
    TestTrue(TEXT("Saved trade commits receipt"),State.CommittedTransactions.Contains(Receipt));
    const int JournalCount=State.Journal.Num();
    TestFalse(TEXT("Duplicate receipt cannot charge twice"),FStationServices::CommitMarketTrade(State,Quote,Receipt,[&](){++Saves;return true;},Reason));
    TestEqual(TEXT("Duplicate does not save"),Saves,2);
    TestEqual(TEXT("Duplicate does not add journal entry"),State.Journal.Num(),JournalCount);
    Quote.Buyer=NPC;Quote.Seller=Pilot;Quote.Source=TEXT("Hold");Quote.Destination=TEXT("NPC");Quote.TotalCredits=126;
    TestFalse(TEXT("Unfunded NPC cannot create credits to buy cargo"),FStationServices::CommitMarketTrade(State,Quote,FGuid::NewGuid(),[](){return true;},Reason));
    TestEqual(TEXT("Rejected sale leaves cargo in hold"),FEconomyLedger::Quantity(State,TEXT("Hold"),Ore),1.25);
    Quote.TotalCredits=100;
    TestTrue(TEXT("Funded NPC sale transfers real cargo and credits"),FStationServices::CommitMarketTrade(State,Quote,FGuid::NewGuid(),[](){return true;},Reason));
    TestEqual(TEXT("Sold cargo removed"),FEconomyLedger::Quantity(State,TEXT("Hold"),Ore),0.0);
    TestEqual(TEXT("NPC funded sale costs credits"),State.Wallets.FindRef(NPC),25LL);
    TestEqual(TEXT("Pilot receives sale proceeds"),State.Wallets.FindRef(Pilot),975LL);
    Quote.Item=TEXT("Courier/Test");
    TestFalse(TEXT("Sealed contract cargo cannot be traded"),FStationServices::CommitMarketTrade(State,Quote,FGuid::NewGuid(),[](){return true;},Reason));
    Quote.Item=Ore;Quote.Quantity=std::numeric_limits<double>::infinity();
    TestFalse(TEXT("Nonfinite trade rejected before persistence"),FStationServices::CommitMarketTrade(State,Quote,FGuid::NewGuid(),[](){return true;},Reason));
    return true;
}
#endif
