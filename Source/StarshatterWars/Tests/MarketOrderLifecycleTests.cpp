#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "GameStructs_economy.h"
#include "PlayerSaveGame.h"
#include "Kismet/GameplayStatics.h"
namespace
{
void MarketFixture(FEconomyState& State)
{
    FEconomyLedger::CreateInventory(State,TEXT("Market"),TEXT("NPC"),TEXT("Janus"),1000)->bMarket=true;
    FEconomyLedger::CreateInventory(State,TEXT("BuyerHold"),TEXT("Buyer"),TEXT("Janus"),100);
    FEconomyLedger::CreateInventory(State,TEXT("SellerHold"),TEXT("Seller"),TEXT("Janus"),100);
    State.Wallets.Add(TEXT("Buyer"),1000);
    FEconomyLedger::Deposit(State,TEXT("SellerHold"),TEXT("Goods"),10,1);
}
FGuid Place(FEconomyState& State,FName Owner,FName Inventory,EMarketOrderSide Side,double Count,int64 Price,FGuid Receipt=FGuid())
{
    FText Reason;
    return FEconomyLedger::PlaceMarketOrder(State,Owner,TEXT("Market"),Inventory,TEXT("Goods"),Side,Count,1,Price,1000,4600,Receipt.IsValid()?Receipt:FGuid::NewGuid(),Reason);
}
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMarketOrderSettlementTest,"Starshatter.Economy.MarketOrders.EscrowPartialSettlementAndHistory",
    EAutomationTestFlags_ApplicationContextMask|EAutomationTestFlags::EngineFilter)
bool FMarketOrderSettlementTest::RunTest(const FString&)
{
    FEconomyState State;MarketFixture(State);FText Reason;
    const FGuid Sell=Place(State,TEXT("Seller"),TEXT("SellerHold"),EMarketOrderSide::Sell,8,80);
    const FGuid Receipt=FGuid::NewGuid();
    const FGuid Buy=Place(State,TEXT("Buyer"),TEXT("BuyerHold"),EMarketOrderSide::Buy,5,100,Receipt);
    if(!TestTrue(TEXT("Both backed orders placed"),Sell.IsValid() && Buy.IsValid()))return false;
    TestEqual(TEXT("Sell order reserves stock without moving it"),FEconomyLedger::Available(State,TEXT("SellerHold"),TEXT("Goods")),2.0);
    TestEqual(TEXT("Buy limit funds really leave spendable wallet"),State.Wallets.FindRef(TEXT("Buyer")),500LL);
    TestEqual(TEXT("Buy destination is reserved"),State.Inventories.FindChecked(TEXT("BuyerHold")).ReservedOutputTonnes,5.0);
    TestFalse(TEXT("Duplicate placement receipt cannot escrow twice"),Place(State,TEXT("Buyer"),TEXT("BuyerHold"),EMarketOrderSide::Buy,5,100,Receipt).IsValid());
    TestEqual(TEXT("One crossing pair settles"),FEconomyLedger::AdvanceMarketOrders(State,1001),1);
    TestEqual(TEXT("Older sell order supplies execution price"),State.Wallets.FindRef(TEXT("Seller")),400LL);
    TestEqual(TEXT("Unused buy-price collateral refunded"),State.Wallets.FindRef(TEXT("Buyer")),600LL);
    TestEqual(TEXT("Buyer receives actual units"),FEconomyLedger::Quantity(State,TEXT("BuyerHold"),TEXT("Goods")),5.0);
    TestEqual(TEXT("Seller stock actually leaves"),FEconomyLedger::Quantity(State,TEXT("SellerHold"),TEXT("Goods")),5.0);
    TestEqual(TEXT("Part-filled sell retains unsold amount"),State.MarketOrders.FindChecked(Sell).Remaining,3.0);
    TestEqual(TEXT("Sold buyer order is filled"),State.MarketOrders.FindChecked(Buy).State,EMarketOrderState::Filled);
    TestEqual(TEXT("Filled buy releases only its capacity reservation"),State.Inventories.FindChecked(TEXT("BuyerHold")).ReservedOutputTonnes,0.0);
    const auto& Fill=State.Journal.Last();
    TestEqual(TEXT("Receipt identifies bought item"),Fill.Item,FName(TEXT("Goods")));
    TestEqual(TEXT("Receipt identifies actual filled quantity"),Fill.Quantity,5.0);
    TestEqual(TEXT("Receipt identifies actual unit price"),Fill.UnitPrice,80.0);
    TestEqual(TEXT("Receipt links buy order"),Fill.BuyOrder,Buy);TestEqual(TEXT("Receipt links sell order"),Fill.SellOrder,Sell);
    TestEqual(TEXT("Receipt retains settlement time"),Fill.TimestampUtc,1001LL);
    TestEqual(TEXT("No repeated settlement after reload/tick"),FEconomyLedger::AdvanceMarketOrders(State,1002),0);
    TestFalse(TEXT("Another owner cannot cancel sell"),FEconomyLedger::CancelMarketOrder(State,TEXT("Buyer"),Sell,FGuid::NewGuid(),Reason));
    const FGuid Cancel=FGuid::NewGuid();
    TestTrue(TEXT("Unsold remainder can be cancelled"),FEconomyLedger::CancelMarketOrder(State,TEXT("Seller"),Sell,Cancel,Reason));
    TestEqual(TEXT("Cancellation unlocks unsold cargo"),FEconomyLedger::Available(State,TEXT("SellerHold"),TEXT("Goods")),5.0);
    TestFalse(TEXT("Cancellation cannot replay"),FEconomyLedger::CancelMarketOrder(State,TEXT("Seller"),Sell,Cancel,Reason));
    return true;
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMarketOrderPersistenceTest,"Starshatter.Economy.MarketOrders.RepriceExpiryAndPersistence",
    EAutomationTestFlags_ApplicationContextMask|EAutomationTestFlags::EngineFilter)
bool FMarketOrderPersistenceTest::RunTest(const FString&)
{
    FEconomyState State;MarketFixture(State);FText Reason;
    const FGuid Buy=Place(State,TEXT("Buyer"),TEXT("BuyerHold"),EMarketOrderSide::Buy,5,100);
    TestTrue(TEXT("Price decrease refunds excess escrow"),FEconomyLedger::ModifyMarketOrder(State,TEXT("Buyer"),Buy,80,FGuid::NewGuid(),Reason));
    TestEqual(TEXT("Reprice credit refund"),State.Wallets.FindRef(TEXT("Buyer")),600LL);
    TestFalse(TEXT("Unaffordable raise rejected"),FEconomyLedger::ModifyMarketOrder(State,TEXT("Buyer"),Buy,300,FGuid::NewGuid(),Reason));
    TestEqual(TEXT("Failed raise retains limit"),State.MarketOrders.FindChecked(Buy).UnitPrice,80LL);
    TestFalse(TEXT("Fractional queued units rejected without rounding a payment"),Place(State,TEXT("Buyer"),TEXT("BuyerHold"),EMarketOrderSide::Buy,1.5,10).IsValid());
    auto* Save=NewObject<UPlayerSaveGame>();Save->PlayerInfo.Economy=State;TArray<uint8> Bytes;
    if(!TestTrue(TEXT("Orders and escrow serialize"),UGameplayStatics::SaveGameToMemory(Save,Bytes)))return false;
    const auto* Loaded=Cast<UPlayerSaveGame>(UGameplayStatics::LoadGameFromMemory(Bytes));
    if(!TestNotNull(TEXT("Saved economy reloads"),Loaded))return false;
    State=Loaded->PlayerInfo.Economy;
    TestEqual(TEXT("Escrow survives load"),State.MarketOrders.FindChecked(Buy).EscrowCredits,400LL);
    TestEqual(TEXT("Capacity reservation survives load"),State.Inventories.FindChecked(TEXT("BuyerHold")).ReservedOutputTonnes,5.0);
    FEconomyLedger::AdvanceMarketOrders(State,4600);
    TestEqual(TEXT("Expired order marked once"),State.MarketOrders.FindChecked(Buy).State,EMarketOrderState::Expired);
    TestEqual(TEXT("Expiry refunds real unspent credits"),State.Wallets.FindRef(TEXT("Buyer")),1000LL);
    TestEqual(TEXT("Expiry releases destination capacity"),State.Inventories.FindChecked(TEXT("BuyerHold")).ReservedOutputTonnes,0.0);
    FEconomyLedger::AdvanceMarketOrders(State,5000);
    TestEqual(TEXT("Expiry cannot mint another refund"),State.Wallets.FindRef(TEXT("Buyer")),1000LL);
    return true;
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMarketOrderFailedFillTest,"Starshatter.Economy.MarketOrders.FailedFillPreservesBackingAndTimePriority",
    EAutomationTestFlags_ApplicationContextMask|EAutomationTestFlags::EngineFilter)
bool FMarketOrderFailedFillTest::RunTest(const FString&)
{
    FEconomyState State;MarketFixture(State);
    const FGuid Buy=Place(State,TEXT("Buyer"),TEXT("BuyerHold"),EMarketOrderSide::Buy,4,100);
    const FGuid Sell=Place(State,TEXT("Seller"),TEXT("SellerHold"),EMarketOrderSide::Sell,4,80);
    State.Inventories.FindChecked(TEXT("BuyerHold")).CapacityTonnes=0;
    TestEqual(TEXT("Destination failure prevents complete fill"),FEconomyLedger::AdvanceMarketOrders(State,1001),0);
    TestEqual(TEXT("Failed fill retains buyer escrow"),State.MarketOrders.FindChecked(Buy).EscrowCredits,400LL);
    TestEqual(TEXT("Failed fill leaves seller cargo"),FEconomyLedger::Quantity(State,TEXT("SellerHold"),TEXT("Goods")),10.0);
    TestEqual(TEXT("Failed fill retains sell reservation"),FEconomyLedger::Available(State,TEXT("SellerHold"),TEXT("Goods")),6.0);
    TestEqual(TEXT("Failed fill retains reserved capacity"),State.Inventories.FindChecked(TEXT("BuyerHold")).ReservedOutputTonnes,4.0);
    State.Inventories.FindChecked(TEXT("BuyerHold")).CapacityTonnes=100;
    TestEqual(TEXT("Backed order can settle once storage is restored"),FEconomyLedger::AdvanceMarketOrders(State,1002),1);
    TestEqual(TEXT("Older buy establishes maker price"),State.Wallets.FindRef(TEXT("Seller")),400LL);
    return true;
}
#endif
