#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "NativeEconomyWorker.h"
#include "EconomyCommandQueue.h"
#include "SimDataEvents.h"
#include "EconomyRuntime.h"
#include "PlayerSaveGame.h"
#include "Kismet/GameplayStatics.h"
#include "HAL/PlatformProcess.h"
#include "HAL/Event.h"
#include "Misc/ScopeExit.h"

namespace
{
struct FMarketWorkerFixture
{
    FEconomyState Live;
    TUniquePtr<FNativeEconomyWorker> Worker=MakeUnique<FNativeEconomyWorker>();
    FEvent* Done=FPlatformProcess::GetSynchEventFromPool(false);
    const FName NPC=FEconomyRuntime::IndustryOwner(),Market=TEXT("Market"),Hold=TEXT("Hold"),Item=TEXT("Goods");
    FText Why;
    FMarketWorkerFixture()
    {
        auto* Store=FEconomyLedger::CreateInventory(Live,Market,NPC,TEXT("Janus"),100);Store->bMarket=true;
        FEconomyLedger::CreateInventory(Live,Hold,FEconomyLedger::Pilot(),TEXT("Janus"),100);
        FEconomyLedger::Deposit(Live,Market,Item,20,1);Live.Wallets.Add(FEconomyLedger::Pilot(),1000);Live.Wallets.Add(NPC,1000);
    }
    ~FMarketWorkerFixture(){Worker.Reset();FPlatformProcess::ReturnSynchEventToPool(Done);}
    FNativeMarketCommand Trade(bool Buy=true)
    {
        FNativeMarketCommand C;C.Action=EMarketCommand::Trade;C.Receipt=FGuid::NewGuid();C.NowUtc=1000;
        auto& Q=C.Quote;Q.bAllowed=true;Q.bBuy=Buy;Q.Item=Item;Q.Quantity=3;Q.UnitPrice=Buy?10:8;Q.TotalCredits=Buy?30:24;
        Q.Source=Buy?Market:Hold;Q.Destination=Buy?Hold:Market;Q.Buyer=Buy?FEconomyLedger::Pilot():NPC;Q.Seller=Buy?NPC:FEconomyLedger::Pilot();return C;
    }
    FNativeMarketCommand Order(EMarketCommand Action,double Quantity=5,int64 Price=10)
    {
        FNativeMarketCommand C;C.Action=Action;C.Receipt=FGuid::NewGuid();C.NowUtc=1000;C.Market=Market;C.Inventory=Hold;C.Item=Item;
        C.Quantity=Quantity;C.UnitTonnes=1;C.LimitPrice=Price;C.Ask=10;C.Bid=8;return C;
    }
    TUniquePtr<FNativeEconomyResult> Calculate(const FNativeMarketCommand& C,uint64 Revision=1)
    {
        FNativeEconomyInput In;In.State=Live;In.Revision=Revision;In.Profile=TEXT("Pilot/session1");In.Market=MakeUnique<FNativeMarketCommand>(C);
        if(!Worker->Submit(MoveTemp(In),[this](){Done->Trigger();}) || !Done->Wait(10000))return nullptr;return Worker->TakeResult();
    }
    bool Commit(FNativeEconomyResult& Out,uint64 Revision=1)
    {return FNativeEconomyWorker::CommitMarket(MoveTemp(Out),Live,Revision,TEXT("Pilot/session1"),[]{return true;},Why);}
};
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMarketWorkerTradeTest,"Starshatter.Domains.Market.BuyIsolationRollbackAndReplay",EAutomationTestFlags_ApplicationContextMask|EAutomationTestFlags::EngineFilter)
bool FMarketWorkerTradeTest::RunTest(const FString&)
{
    FMarketWorkerFixture F;auto C=F.Trade();int32 Notifications=0;bool OnMain=true;
    const auto H=FSimDataEvents::Changed().AddLambda([&](const FEconomyState& S){if(&S==&F.Live){++Notifications;OnMain&=IsInGameThread();}});ON_SCOPE_EXIT{FSimDataEvents::Changed().Remove(H);};
    auto Out=F.Calculate(C);if(!TestNotNull(TEXT("Native market reply"),Out.Get()))return false;
    TestTrue(TEXT("Market arithmetic executes on worker"),Out->bOffGameThread);TestTrue(TEXT("Native buy calculated"),Out->Market && Out->Market->bSucceeded);
    TestEqual(TEXT("No early cargo"),FEconomyLedger::Quantity(F.Live,F.Hold,F.Item),0.);TestEqual(TEXT("No early wallet charge"),F.Live.Wallets.FindRef(FEconomyLedger::Pilot()),1000LL);TestEqual(TEXT("Worker cannot notify UI"),Notifications,0);
    TestFalse(TEXT("Generic commit refuses a typed financial result"),FNativeEconomyWorker::Commit(MoveTemp(*Out),F.Live,1,TEXT("Pilot/session1")));
    TestFalse(TEXT("Stale inventory revision rejected"),FNativeEconomyWorker::CommitMarket(MoveTemp(*Out),F.Live,2,TEXT("Pilot/session1"),[]{return true;},F.Why));
    TestFalse(TEXT("Loaded profile/session cannot receive old work"),FNativeEconomyWorker::CommitMarket(MoveTemp(*Out),F.Live,1,TEXT("Pilot/session2"),[]{return true;},F.Why));
    FMarketOrder Concurrent;Concurrent.Id=FGuid::NewGuid();F.Live.MarketOrders.Add(Concurrent.Id,Concurrent);
    TestFalse(TEXT("Direct book changes cannot be overwritten without a ledger revision"),F.Commit(*Out));F.Live.MarketOrders.Reset();
    bool SawCandidate=false;
    TestFalse(TEXT("Failed persistence rolls back"),FNativeEconomyWorker::CommitMarket(MoveTemp(*Out),F.Live,1,TEXT("Pilot/session1"),[&](){SawCandidate=FEconomyLedger::Quantity(F.Live,F.Hold,F.Item)==3;return false;},F.Why));
    TestTrue(TEXT("Save sees cargo and wallet candidate together"),SawCandidate);TestEqual(TEXT("Cargo restored on save failure"),FEconomyLedger::Quantity(F.Live,F.Hold,F.Item),0.);TestEqual(TEXT("Wallet restored on failure"),F.Live.Wallets.FindRef(FEconomyLedger::Pilot()),1000LL);
    Out=F.Calculate(C);if(!Out)return false;TestTrue(TEXT("Retry after rollback saves once"),F.Commit(*Out));
    TestEqual(TEXT("Exactly three goods delivered"),FEconomyLedger::Quantity(F.Live,F.Hold,F.Item),3.);TestEqual(TEXT("Finite NPC stock reduced"),FEconomyLedger::Quantity(F.Live,F.Market,F.Item),17.);TestEqual(TEXT("Exact quoted debit"),F.Live.Wallets.FindRef(FEconomyLedger::Pilot()),970LL);
    TestTrue(TEXT("UI notifications stay on main thread"),OnMain);TestFalse(TEXT("Consumed result cannot commit twice"),F.Commit(*Out));
    Out=F.Calculate(C);TestTrue(TEXT("Replay receipt rejected on worker"),Out && Out->Market && !Out->Market->bSucceeded);
    return true;
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMarketWorkerSellTest,"Starshatter.Domains.Market.SellFundingCapacityAndFees",EAutomationTestFlags_ApplicationContextMask|EAutomationTestFlags::EngineFilter)
bool FMarketWorkerSellTest::RunTest(const FString&)
{
    FMarketWorkerFixture F;FEconomyLedger::Deposit(F.Live,F.Hold,F.Item,5,1);auto C=F.Trade(false);
    F.Live.Wallets[F.NPC]=0;auto Out=F.Calculate(C);TestTrue(TEXT("Unfunded NPC cannot buy cargo"),Out && !Out->Market->bSucceeded);TestEqual(TEXT("Rejected sale retains cargo"),FEconomyLedger::Quantity(F.Live,F.Hold,F.Item),5.);
    F.Live.Wallets[F.NPC]=1000;F.Live.Inventories.FindChecked(F.Market).ReservedOutputTonnes=80;Out=F.Calculate(C);TestTrue(TEXT("Reserved factory output space blocks sale"),Out && !Out->Market->bSucceeded);F.Live.Inventories.FindChecked(F.Market).ReservedOutputTonnes=0;
    const FGuid Reservation=FEconomyLedger::Reserve(F.Live,FEconomyLedger::Pilot(),F.Hold,F.Item,4);Out=F.Calculate(C);TestTrue(TEXT("Reserved cargo cannot be sold"),Out && !Out->Market->bSucceeded);FEconomyLedger::Release(F.Live,FEconomyLedger::Pilot(),Reservation);
    Out=F.Calculate(C);if(!Out)return false;TestTrue(TEXT("Funded sale commits"),F.Commit(*Out));
    const auto Charges=CommerceCharges::Quote(24,ECommercePurpose::MarketSale);
    TestEqual(TEXT("Seller receives disclosed net, not gross"),F.Live.Wallets.FindRef(FEconomyLedger::Pilot()),1000+Charges.Base);TestEqual(TEXT("NPC pays exact gross"),F.Live.Wallets.FindRef(F.NPC),976LL);
    TestEqual(TEXT("Cargo leaves hold"),FEconomyLedger::Quantity(F.Live,F.Hold,F.Item),2.);TestEqual(TEXT("Actual cargo arrives in NPC stock"),FEconomyLedger::Quantity(F.Live,F.Market,F.Item),23.);
    int64 Sum=0;for(const auto& W:F.Live.Wallets)Sum+=W.Value;TestEqual(TEXT("Credits conserved including tax and fees"),Sum,2000LL);
    return true;
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMarketWorkerOrderTest,"Starshatter.Domains.Market.PartialFillRepriceCancelPersistence",EAutomationTestFlags_ApplicationContextMask|EAutomationTestFlags::EngineFilter)
bool FMarketWorkerOrderTest::RunTest(const FString&)
{
    FMarketWorkerFixture F;FEconomyLedger::Withdraw(F.Live,F.NPC,F.Market,F.Item,18);auto C=F.Order(EMarketCommand::PlaceBuy);
    auto Out=F.Calculate(C);if(!TestNotNull(TEXT("Limit-order reply"),Out.Get()))return false;const FGuid Order=Out->Market->Order;
    TestTrue(TEXT("Worker generated order"),Out->Market->bSucceeded && Order.IsValid());TestTrue(TEXT("Order and escrow commit together"),F.Commit(*Out));
    auto* Book=F.Live.MarketOrders.Find(Order);if(!TestNotNull(TEXT("New order book is not discarded by snapshot merge"),Book))return false;
    TestEqual(TEXT("Only real two NPC units fill"),FEconomyLedger::Quantity(F.Live,F.Hold,F.Item),2.);TestEqual(TEXT("Three units remain open"),Book->Remaining,3.);TestEqual(TEXT("Remaining escrow is funded"),Book->EscrowCredits,30LL);TestEqual(TEXT("Future output space remains reserved"),F.Live.Inventories.FindChecked(F.Hold).ReservedOutputTonnes,3.);
    C=F.Order(EMarketCommand::Modify,3,11);C.Order=Order;Out=F.Calculate(C);if(!Out)return false;
    TestFalse(TEXT("Failed repricing save restores escrow and price"),FNativeEconomyWorker::CommitMarket(MoveTemp(*Out),F.Live,1,TEXT("Pilot/session1"),[]{return false;},F.Why));TestEqual(TEXT("Old limit retained"),F.Live.MarketOrders.FindChecked(Order).UnitPrice,10LL);
    Out=F.Calculate(C);if(!Out)return false;TestTrue(TEXT("Reprice saves"),F.Commit(*Out));TestEqual(TEXT("Repriced remaining escrow"),F.Live.MarketOrders.FindChecked(Order).EscrowCredits,33LL);
    C=F.Order(EMarketCommand::Cancel);C.Order=Order;Out=F.Calculate(C);if(!Out)return false;TestTrue(TEXT("Cancellation saves"),F.Commit(*Out));
    TestEqual(TEXT("Cancelled status preserved"),F.Live.MarketOrders.FindChecked(Order).State,EMarketOrderState::Cancelled);TestEqual(TEXT("Cancellation releases reserved space"),F.Live.Inventories.FindChecked(F.Hold).ReservedOutputTonnes,0.);
    TestEqual(TEXT("Only goods plus disclosed brokerage remain spent"),F.Live.Wallets.FindRef(FEconomyLedger::Pilot()),980-CommerceCharges::BrokerFee(50)-CommerceCharges::BrokerFee(33));
    auto* Save=NewObject<UPlayerSaveGame>();Save->PlayerInfo.Economy=F.Live;TArray<uint8> Bytes;TestTrue(TEXT("Market state serializes"),UGameplayStatics::SaveGameToMemory(Save,Bytes));
    const auto* Loaded=Cast<UPlayerSaveGame>(UGameplayStatics::LoadGameFromMemory(Bytes));if(!TestNotNull(TEXT("Market state reloads"),Loaded))return false;
    TestEqual(TEXT("Cancelled order retained on reload"),Loaded->PlayerInfo.Economy.MarketOrders.FindChecked(Order).State,EMarketOrderState::Cancelled);
    return true;
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMarketWorkerReservationTest,"Starshatter.Domains.Market.SellOrderReservationsAndSealedCargo",EAutomationTestFlags_ApplicationContextMask|EAutomationTestFlags::EngineFilter)
bool FMarketWorkerReservationTest::RunTest(const FString&)
{
    FMarketWorkerFixture F;FEconomyLedger::Deposit(F.Live,F.Hold,F.Item,5,1);auto C=F.Order(EMarketCommand::PlaceSell,4,20);
    auto Out=F.Calculate(C);if(!Out)return false;const FGuid Order=Out->Market->Order;TestTrue(TEXT("Sell order saves"),F.Commit(*Out));
    TestEqual(TEXT("Actual cargo held, not prematurely delivered"),FEconomyLedger::Quantity(F.Live,F.Hold,F.Item),5.);TestEqual(TEXT("Only unreserved unit is available"),FEconomyLedger::Available(F.Live,F.Hold,F.Item),1.);
    C=F.Order(EMarketCommand::Cancel);C.Order=Order;Out=F.Calculate(C);if(!Out)return false;TestTrue(TEXT("Sell cancellation saves"),F.Commit(*Out));TestEqual(TEXT("Cancellation releases goods without duplicating them"),FEconomyLedger::Available(F.Live,F.Hold,F.Item),5.);
    C=F.Trade();C.Quote.Item=TEXT("Courier/Sealed");Out=F.Calculate(C);TestTrue(TEXT("Sealed contract package cannot be traded"),Out && !Out->Market->bSucceeded);
    C=F.Order(EMarketCommand::PlaceBuy);C.Item=TEXT("Courier/Sealed");Out=F.Calculate(C);TestTrue(TEXT("Sealed cargo cannot enter order book"),Out && !Out->Market->bSucceeded);
    return true;
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMarketQueuePolicyTest,"Starshatter.Domains.Market.QueueBoundsOrderingAndCancellation",EAutomationTestFlags_ApplicationContextMask|EAutomationTestFlags::EngineFilter)
bool FMarketQueuePolicyTest::RunTest(const FString&)
{
    FEconomyCommandQueue Q;TArray<FGuid> Ids;const auto Label=INVTEXT("BUY GOODS");
    TestFalse(TEXT("Invalid receipt rejected"),Q.Enqueue({},Label));
    for(int32 I=0;I<Q.Capacity;++I){Ids.Add(FGuid::NewGuid());TestTrue(TEXT("Bounded slot admitted"),Q.Enqueue(Ids.Last(),Label));}
    TestFalse(TEXT("Overflow rejected"),Q.Enqueue(FGuid::NewGuid(),Label));
    TestFalse(TEXT("Receipt replay rejected"),Q.Enqueue(Ids[0],Label));
    TestFalse(TEXT("Cannot skip FIFO"),Q.Start(Ids[1]));TestTrue(TEXT("First request starts"),Q.Start(Ids[0]));
    TestFalse(TEXT("Single worker only"),Q.Start(Ids[1]));TestFalse(TEXT("Processing cannot be cancelled"),Q.Cancel(Ids[0],Label));
    TestTrue(TEXT("Unstarted request can be cancelled"),Q.Cancel(Ids[1],Label));
    TestTrue(TEXT("Failed request finishes"),Q.Finish(Ids[0],false,INVTEXT("Stock changed")));
    TestEqual(TEXT("Cancelled row skipped"),Q.Next(),Ids[2]);TestTrue(TEXT("Queue continues after failure"),Q.Start(Ids[2]));
    TestTrue(TEXT("Saved confirmation"),Q.Finish(Ids[2],true,Label));TestFalse(TEXT("Completion cannot replay"),Q.Finish(Ids[2],true,Label));
    TestEqual(TEXT("Pending count excludes terminal rows"),Q.Count(),Q.Capacity-3);
    TestEqual(TEXT("Failure reason retained"),Q.Snapshot()[0].Reason.ToString(),FString(TEXT("Stock changed")));
    return true;
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMarketQueueFinancialTest,"Starshatter.Domains.Market.QueueUsesLatestSavedState",EAutomationTestFlags_ApplicationContextMask|EAutomationTestFlags::EngineFilter)
bool FMarketQueueFinancialTest::RunTest(const FString&)
{
    FMarketWorkerFixture F;FEconomyCommandQueue Q;auto A=F.Trade(),B=F.Trade();
    Q.Enqueue(A.Receipt,INVTEXT("FIRST BUY"));Q.Enqueue(B.Receipt,INVTEXT("SECOND BUY"));
    for(const auto* C:{&A,&B})
    {
        TestEqual(TEXT("Next receipt follows submission order"),Q.Next(),C->Receipt);Q.Start(C->Receipt);
        auto Out=F.Calculate(*C);if(!TestNotNull(TEXT("Queued worker reply"),Out.Get()))return false;
        const bool Saved=F.Commit(*Out);TestTrue(TEXT("Each request saves separately"),Saved);Q.Finish(C->Receipt,Saved,F.Why);
    }
    TestEqual(TEXT("Second request sees prior saved debit"),F.Live.Wallets.FindRef(FEconomyLedger::Pilot()),940LL);
    TestEqual(TEXT("Both buys arrive exactly once"),FEconomyLedger::Quantity(F.Live,F.Hold,F.Item),6.);
    auto C=F.Trade();Q.Enqueue(C.Receipt,INVTEXT("CANCELLED BUY"));Q.Cancel(C.Receipt,INVTEXT("Cancelled"));
    TestFalse(TEXT("Cancelled request cannot execute"),Q.Start(C.Receipt));TestEqual(TEXT("Cancellation charges nothing"),F.Live.Wallets.FindRef(FEconomyLedger::Pilot()),940LL);
    return true;
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMarketQueueHistoryTest,"Starshatter.Domains.Market.QueueHistoryAndProfileReset",EAutomationTestFlags_ApplicationContextMask|EAutomationTestFlags::EngineFilter)
bool FMarketQueueHistoryTest::RunTest(const FString&)
{
    FEconomyCommandQueue Q;
    for(int32 I=0;I<100;++I){const FGuid Id=FGuid::NewGuid();Q.Enqueue(Id,INVTEXT("TRADE"));Q.Start(Id);Q.Finish(Id,true,INVTEXT("Saved"));}
    TestEqual(TEXT("History remains bounded"),Q.Snapshot().Num(),Q.HistoryLimit);TestEqual(TEXT("No phantom pending entries"),Q.Count(),0);
    const FGuid Old=FGuid::NewGuid();Q.Enqueue(Old,INVTEXT("OLD PROFILE"));Q.Reset();
    TestTrue(TEXT("Profile reset clears private history"),Q.Snapshot().IsEmpty());TestFalse(TEXT("Old receipt no longer dispatches"),Q.Start(Old));
    return true;
}
#endif
