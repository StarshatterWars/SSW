#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "NativeEconomyWorker.h"
#include "EconomyRuntime.h"
#include "SimDataEvents.h"
#include "HAL/PlatformProcess.h"
#include "HAL/Event.h"
#include "Misc/ScopeExit.h"
#include "Kismet/GameplayStatics.h"
#include "PlayerSaveGame.h"
namespace
{
struct FLedgerWorkerFixture
{
    FEconomyState State;
    TUniquePtr<FNativeEconomyWorker> Worker=MakeUnique<FNativeEconomyWorker>();
    FEvent* Done=FPlatformProcess::GetSynchEventFromPool(false);
    FName Hold=TEXT("Hold"),Output=TEXT("Output"),Ore=TEXT("Ore");FText Why;
    FLedgerWorkerFixture()
    {
        State.Wallets.Add(FEconomyLedger::Pilot(),5000);State.InitializedMarkets.Add(TEXT("Career starting grant"));
        FEconomyLedger::CreateInventory(State,Hold,FEconomyLedger::Pilot(),TEXT("Janus"),100);FEconomyLedger::Deposit(State,Hold,Ore,10,1);
    }
    ~FLedgerWorkerFixture(){Worker.Reset();FPlatformProcess::ReturnSynchEventToPool(Done);}
    FNativeLedgerCommand Command(ELedgerCommand Action)
    {FNativeLedgerCommand C;C.Action=Action;C.Receipt=FGuid::NewGuid();C.NowUtc=1000;C.Source=Hold;C.Hold=Hold;C.Destination=Output;C.System=TEXT("Janus");C.Capacity=100;return C;}
    TUniquePtr<FNativeEconomyResult> Run(const FNativeLedgerCommand& C)
    {
        FNativeEconomyInput In;In.State=State;In.Revision=1;In.Profile=TEXT("Ledger/session1");In.Ledger=MakeUnique<FNativeLedgerCommand>(C);
        if(!Worker->Submit(MoveTemp(In),[this](){Done->Trigger();}) || !Done->Wait(10000))return nullptr;return Worker->TakeResult();
    }
    bool Apply(const FNativeLedgerCommand& C)
    {auto R=Run(C);return R && FNativeEconomyWorker::CommitLedger(MoveTemp(*R),State,1,TEXT("Ledger/session1"),[]{return true;},Why);}
};
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FLedgerTransferWorkerTest,"Starshatter.Domains.Ledger.TransferRollbackAndReceipts",EAutomationTestFlags_ApplicationContextMask|EAutomationTestFlags::EngineFilter)
bool FLedgerTransferWorkerTest::RunTest(const FString&)
{
    FLedgerWorkerFixture F;auto C=F.Command(ELedgerCommand::Transfer);C.Item=F.Ore;C.Quantity=3;
    auto R=F.Run(C);if(!TestNotNull(TEXT("Native transfer reply"),R.Get()))return false;
    TestTrue(TEXT("Off game thread"),R->bOffGameThread);TestEqual(TEXT("No early cargo transfer"),FEconomyLedger::Quantity(F.State,F.Hold,F.Ore),10.);
    TestFalse(TEXT("Typed result cannot bypass save gate"),FNativeEconomyWorker::Commit(MoveTemp(*R),F.State,1,TEXT("Ledger/session1")));
    TestFalse(TEXT("Stale revision cannot commit"),FNativeEconomyWorker::CommitLedger(MoveTemp(*R),F.State,2,TEXT("Ledger/session1"),[]{return true;},F.Why));
    TestFalse(TEXT("Failed save rolls back"),FNativeEconomyWorker::CommitLedger(MoveTemp(*R),F.State,1,TEXT("Ledger/session1"),[]{return false;},F.Why));
    TestEqual(TEXT("Source restored"),FEconomyLedger::Quantity(F.State,F.Hold,F.Ore),10.);TestFalse(TEXT("No unpaid destination inventory"),F.State.Inventories.Contains(F.Output));
    TestTrue(TEXT("Retry saved"),F.Apply(C));TestEqual(TEXT("Only requested cargo arrives"),FEconomyLedger::Quantity(F.State,F.Output,F.Ore),3.);
    TestFalse(TEXT("Replayed receipt cannot transfer twice"),F.Apply(C));TestEqual(TEXT("Source debited once"),FEconomyLedger::Quantity(F.State,F.Hold,F.Ore),7.);
    TestEqual(TEXT("Transfer never grants credits"),F.State.Wallets.FindRef(FEconomyLedger::Pilot()),5000LL);
    return true;
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FLedgerTransferLimitsTest,"Starshatter.Domains.Ledger.TransferCapacityReservationsAndSealedCargo",EAutomationTestFlags_ApplicationContextMask|EAutomationTestFlags::EngineFilter)
bool FLedgerTransferLimitsTest::RunTest(const FString&)
{
    FLedgerWorkerFixture F;auto C=F.Command(ELedgerCommand::Transfer);C.Item=F.Ore;C.Quantity=3;C.Capacity=2;
    TestFalse(TEXT("Destination capacity enforced"),F.Apply(C));C.Capacity=100;
    FEconomyReservation Reserved;Reserved.Id=FGuid::NewGuid();Reserved.Owner=FEconomyLedger::Pilot();Reserved.Inventory=F.Hold;Reserved.Item=F.Ore;Reserved.Quantity=9;F.State.Reservations.Add(Reserved.Id,Reserved);
    TestFalse(TEXT("Reserved stock cannot transfer"),F.Apply(C));F.State.Reservations.Reset();
    C.Item=TEXT("Courier/Test");FEconomyLedger::Deposit(F.State,F.Hold,C.Item,1,.1);C.Quantity=1;
    TestFalse(TEXT("Sealed packages require contract delivery"),F.Apply(C));return true;
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FLedgerIndustryWorkerTest,"Starshatter.Domains.Ledger.BlueprintInstallCancelAndSave",EAutomationTestFlags_ApplicationContextMask|EAutomationTestFlags::EngineFilter)
bool FLedgerIndustryWorkerTest::RunTest(const FString&)
{
    FLedgerWorkerFixture F;auto B=F.Command(ELedgerCommand::BlueprintPurchase);B.Recipe=TEXT("Refine");B.Credits=100;B.Runs=2;
    TestTrue(TEXT("License purchase saved"),F.Apply(B));TestEqual(TEXT("One license"),F.State.Blueprints.Num(),1);TestEqual(TEXT("Existing price charged"),F.State.Wallets.FindRef(FEconomyLedger::Pilot()),4900LL);
    TestFalse(TEXT("No duplicate purchase receipt"),F.Apply(B));const FGuid License=F.State.Blueprints.CreateConstIterator().Key();
    auto C=F.Command(ELedgerCommand::IndustryInstall);C.Object=License;C.Facility=TEXT("Works");C.Runs=1;C.Duration=10;C.Credits=25;
    FCommodityAmount Ore;Ore.Item=F.Ore;Ore.Quantity=2;C.Materials.Add(Ore);FCommodityAmount Product;Product.Item=TEXT("Ingots");Product.Quantity=1;C.Products.Add(Product);
    auto R=F.Run(C);if(!R)return false;
    TestFalse(TEXT("Failed installation save rolls back"),FNativeEconomyWorker::CommitLedger(MoveTemp(*R),F.State,1,TEXT("Ledger/session1"),[]{return false;},F.Why));TestTrue(TEXT("No unsaved jobs"),F.State.Jobs.IsEmpty());
    TestTrue(TEXT("Installation saved"),F.Apply(C));TestEqual(TEXT("Only one job"),F.State.Jobs.Num(),1);TestEqual(TEXT("Installation fee charged once"),F.State.Wallets.FindRef(FEconomyLedger::Pilot()),4875LL);
    auto Cancel=F.Command(ELedgerCommand::IndustryCancel);Cancel.Object=F.State.Jobs[0].Id;TestTrue(TEXT("Cancellation saved"),F.Apply(Cancel));
    TestEqual(TEXT("Queued cancellation retains input"),FEconomyLedger::Quantity(F.State,F.Hold,F.Ore),10.);TestEqual(TEXT("Cancelled state"),F.State.Jobs[0].State,EIndustryJobState::Cancelled);
    TestFalse(TEXT("Cancellation cannot replay"),F.Apply(Cancel));return true;
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FLedgerContractWorkerTest,"Starshatter.Domains.Ledger.ContractCargoEscrowAndClaims",EAutomationTestFlags_ApplicationContextMask|EAutomationTestFlags::EngineFilter)
bool FLedgerContractWorkerTest::RunTest(const FString&)
{
    FLedgerWorkerFixture F;auto Seed=F.Command(ELedgerCommand::ContractMaintain);Seed.bSeedTraining=true;
    TestTrue(TEXT("Training publication saved"),F.Apply(Seed));TestEqual(TEXT("Three existing training offers"),F.State.Contracts.Num(),3);
    const int64 Initial=F.State.Wallets.FindRef(FEconomyLedger::Pilot());
    auto Accept=F.Command(ELedgerCommand::ContractAccept);Accept.Contract=TEXT("Courier");auto R=F.Run(Accept);if(!R)return false;
    TestFalse(TEXT("Acceptance save rollback"),FNativeEconomyWorker::CommitLedger(MoveTemp(*R),F.State,1,TEXT("Ledger/session1"),[]{return false;},F.Why));
    TestEqual(TEXT("Failed acceptance leaves offer open"),F.State.Contracts.FindChecked(TEXT("Courier")).State,EContractState::Open);
    TestEqual(TEXT("Failed acceptance leaves no package"),FEconomyLedger::Quantity(F.State,F.Hold,TEXT("Courier/Courier")),0.);
    TestTrue(TEXT("Courier acceptance saved"),F.Apply(Accept));TestEqual(TEXT("Sealed cargo arrives with acceptance"),FEconomyLedger::Quantity(F.State,F.Hold,TEXT("Courier/Courier")),1.);
    TestTrue(TEXT("Collateral debited"),F.State.Wallets.FindRef(FEconomyLedger::Pilot())<Initial);
    auto Deliver=F.Command(ELedgerCommand::ContractDeliver);Deliver.Contract=TEXT("Courier");Deliver.Destination=TEXT("Janus Transfer Station");
    TestTrue(TEXT("Delivery saved"),F.Apply(Deliver));TestEqual(TEXT("Package removed on delivery"),FEconomyLedger::Quantity(F.State,F.Hold,TEXT("Courier/Courier")),0.);
    TestEqual(TEXT("Contract complete"),F.State.Contracts.FindChecked(TEXT("Courier")).State,EContractState::Completed);
    TestTrue(TEXT("Reward and collateral received"),F.State.Wallets.FindRef(FEconomyLedger::Pilot())>Initial);TestFalse(TEXT("Delivery replay rejected"),F.Apply(Deliver));
    auto Claim=F.Command(ELedgerCommand::ContractClaim);Claim.Contract=TEXT("Sale");const auto Before=F.State.Wallets.FindRef(FEconomyLedger::Pilot());
    TestTrue(TEXT("Auction payout saved"),F.Apply(Claim));TestTrue(TEXT("Payout received"),F.State.Wallets.FindRef(FEconomyLedger::Pilot())>Before);TestFalse(TEXT("Claim cannot repeat"),F.Apply(Claim));
    auto Save=NewObject<UPlayerSaveGame>();Save->PlayerInfo.Economy=F.State;TArray<uint8> Bytes;TestTrue(TEXT("Contract state serializes"),UGameplayStatics::SaveGameToMemory(Save,Bytes));
    const auto* Reload=Cast<UPlayerSaveGame>(UGameplayStatics::LoadGameFromMemory(Bytes));if(!TestNotNull(TEXT("Contract save reloads"),Reload))return false;
    TestEqual(TEXT("Completion survives reload"),Reload->PlayerInfo.Economy.Contracts.FindChecked(TEXT("Courier")).State,EContractState::Completed);return true;
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FLedgerMarketExpiryWorkerTest,"Starshatter.Domains.Ledger.OrderExpiryAndStaleContract",EAutomationTestFlags_ApplicationContextMask|EAutomationTestFlags::EngineFilter)
bool FLedgerMarketExpiryWorkerTest::RunTest(const FString&)
{
    FLedgerWorkerFixture F;FEconomyLedger::CreateInventory(F.State,TEXT("Market"),TEXT("NPC"),TEXT("Janus"),100)->bMarket=true;
    const FGuid Order=FEconomyLedger::PlaceMarketOrder(F.State,FEconomyLedger::Pilot(),TEXT("Market"),F.Hold,F.Ore,EMarketOrderSide::Buy,2,1,10,1000,1001,FGuid::NewGuid(),F.Why);
    TestTrue(TEXT("Expiry fixture order created"),Order.IsValid());auto C=F.Command(ELedgerCommand::MarketMaintain);C.NowUtc=1002;TestTrue(TEXT("Zero-fill expiry still saves"),F.Apply(C));
    TestEqual(TEXT("Expired status committed"),F.State.MarketOrders.FindChecked(Order).State,EMarketOrderState::Expired);TestEqual(TEXT("Unused escrow returned"),F.State.Wallets.FindRef(FEconomyLedger::Pilot()),5000LL);
    auto Seed=F.Command(ELedgerCommand::ContractMaintain);Seed.bSeedTraining=true;F.Apply(Seed);
    auto Accept=F.Command(ELedgerCommand::ContractAccept);Accept.Contract=TEXT("Courier");auto R=F.Run(Accept);if(!R)return false;
    F.State.Contracts.FindChecked(TEXT("Courier")).Terms.Description=TEXT("Concurrent amendment");
    TestFalse(TEXT("Unversioned contract edits cannot be overwritten"),FNativeEconomyWorker::CommitLedger(MoveTemp(*R),F.State,1,TEXT("Ledger/session1"),[]{return true;},F.Why));return true;
}
#endif
