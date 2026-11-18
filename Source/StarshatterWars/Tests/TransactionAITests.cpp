#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "TransactionAI.h"
#include "ContractLedger.h"
#include "ContractSaveTransaction.h"
#include "AcceptanceAI.h"
#include "SettlementAI.h"
#include "PlayerSaveGame.h"
#include "Kismet/GameplayStatics.h"
#include "Ship.h"
#include "ShipDesign.h"
#include "DataLoader.h"
#include "Misc/ScopeExit.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FWalletMarketDeltasTest,"Starshatter.Economy.Transactions.EscrowWalletDeltasAndPersistence",
    EAutomationTestFlags_ApplicationContextMask|EAutomationTestFlags::EngineFilter)
bool FWalletMarketDeltasTest::RunTest(const FString&)
{
    FEconomyState State;FText Reason;
    FEconomyLedger::CreateInventory(State,TEXT("Market"),TEXT("NPC"),TEXT("Janus"),1000)->bMarket=true;
    FEconomyLedger::CreateInventory(State,TEXT("BuyerHold"),TEXT("Buyer"),TEXT("Janus"),100);
    FEconomyLedger::CreateInventory(State,TEXT("SellerHold"),TEXT("Seller"),TEXT("Janus"),100);
    FEconomyLedger::Deposit(State,TEXT("SellerHold"),TEXT("Goods"),10,1);
    State.Wallets.Add(TEXT("Buyer"),1000);
    const FGuid Sell=FEconomyLedger::PlaceMarketOrder(State,TEXT("Seller"),TEXT("Market"),TEXT("SellerHold"),TEXT("Goods"),EMarketOrderSide::Sell,5,1,80,1000,4000,FGuid::NewGuid(),Reason);
    const FGuid Buy=FEconomyLedger::PlaceMarketOrder(State,TEXT("Buyer"),TEXT("Market"),TEXT("BuyerHold"),TEXT("Goods"),EMarketOrderSide::Buy,5,1,100,1000,4000,FGuid::NewGuid(),Reason);
    if(!TestTrue(TEXT("Backed orders created"),Sell.IsValid() && Buy.IsValid()))return false;
    FEconomyLedger::AdvanceMarketOrders(State,1001);
    FTransactionQuery Query;Query.Owner=TEXT("Buyer");Query.bNewestFirst=false;
    const auto Rows=TransactionAI::Query(State,Query);
    if(!TestEqual(TEXT("Escrow and fill each visible once"),Rows.Num(),2))return false;
    TestEqual(TEXT("Escrow removes spendable credits"),Rows[0].WalletDelta,-500LL);
    TestEqual(TEXT("Fill returns price improvement without second debit"),Rows[1].WalletDelta,100LL);
    TestEqual(TEXT("Historical escrow balance"),Rows[0].Transaction.WalletBalancesAfter.FindRef(TEXT("Buyer")),500LL);
    TestEqual(TEXT("Historical fill balance"),Rows[1].Transaction.WalletBalancesAfter.FindRef(TEXT("Buyer")),600LL);
    TestEqual(TEXT("Trade value remains available separately"),Rows[1].Transaction.Credits,400LL);
    TestEqual(TEXT("Wallet matches opening plus actual changes"),TransactionAI::Balance(State,TEXT("Buyer")),1000LL+Rows[0].WalletDelta+Rows[1].WalletDelta);
    Query.Direction=ETransactionDirection::Incoming;TestEqual(TEXT("Income filter retains only refund"),TransactionAI::Query(State,Query).Num(),1);
    Query.Direction=ETransactionDirection::Outgoing;TestEqual(TEXT("Outgoings filter retains only escrow debit"),TransactionAI::Query(State,Query).Num(),1);
    Query.bMarketOnly=true;
    auto Trades=TransactionAI::Query(State,Query);
    if(!TestEqual(TEXT("Market buy filter shows fill, excludes escrow placement"),Trades.Num(),1))return false;
    TestEqual(TEXT("Market buyer sees actual filled trade"),Trades[0].Transaction.Reason,FName(TEXT("Limit order fill")));
    Query.Direction=ETransactionDirection::Incoming;TestEqual(TEXT("Price improvement does not misclassify a buy as a market sale"),TransactionAI::Query(State,Query).Num(),0);
    Query.Owner=TEXT("Seller");TestEqual(TEXT("Seller sees the same trade as a sale"),TransactionAI::Query(State,Query).Num(),1);
    Query.Owner=TEXT("Buyer");Query.bMarketOnly=false;
    auto* Save=NewObject<UPlayerSaveGame>();Save->PlayerInfo.Economy=State;TArray<uint8> Bytes;
    if(!TestTrue(TEXT("Wallet receipts serialize"),UGameplayStatics::SaveGameToMemory(Save,Bytes)))return false;
    const auto* Loaded=Cast<UPlayerSaveGame>(UGameplayStatics::LoadGameFromMemory(Bytes));
    if(!TestNotNull(TEXT("Wallet state reloads"),Loaded))return false;
    Query.Direction=ETransactionDirection::All;
    const auto LoadedRows=TransactionAI::Query(Loaded->PlayerInfo.Economy,Query);
    if(!TestEqual(TEXT("All persisted receipts retained"),LoadedRows.Num(),2))return false;
    TestEqual(TEXT("Exact wallet delta survives reload"),LoadedRows[1].WalletDelta,100LL);
    TestEqual(TEXT("Historical balance survives reload"),LoadedRows[0].Transaction.WalletBalancesAfter.FindRef(TEXT("Buyer")),500LL);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FWalletContractBridgeTest,"Starshatter.Economy.Transactions.ContractCaptureAndLegacyReceipts",
    EAutomationTestFlags_ApplicationContextMask|EAutomationTestFlags::EngineFilter)
bool FWalletContractBridgeTest::RunTest(const FString&)
{
    FEconomyState State;State.Wallets.Add(TEXT("Pilot"),1000);
    FContractAccounts Accounts;Accounts.FindOrAdd(TEXT("Pilot")).Credits=750;
    TMap<FName,FContractTransaction> Contracts;
    FContractLedger::Capture(State,Contracts,Accounts);FContractLedger::Capture(State,Contracts,Accounts);
    TestEqual(TEXT("Repeated contract capture never pays or records twice"),State.Journal.Num(),1);
    Accounts.FindChecked(TEXT("Pilot")).Credits=1200;FContractLedger::Capture(State,Contracts,Accounts);
    FTransactionQuery Query;
    auto Rows=TransactionAI::Query(State,Query);
    if(!TestEqual(TEXT("Changed contract balances journaled"),Rows.Num(),2))return false;
    TestEqual(TEXT("Contract settlement preserves exact net payout"),Rows[0].WalletDelta,450LL);
    TestEqual(TEXT("Contract balance remains authoritative"),TransactionAI::Balance(State),1200LL);
    TestEqual(TEXT("Contract category persists"),Rows[0].Transaction.Category,EWalletTransactionCategory::Contract);
    FEconomyTransaction Legacy;Legacy.Id=FGuid::NewGuid();Legacy.From=TEXT("Pilot");Legacy.To=TEXT("Shipyard");Legacy.Credits=50;Legacy.Reason=TEXT("Component refit");State.Journal.Add(Legacy);
    Query.Search=TEXT("refit");Rows=TransactionAI::Query(State,Query);
    if(!TestEqual(TEXT("Case-insensitive search finds old receipts"),Rows.Num(),1))return false;
    TestEqual(TEXT("Legacy ordinary transfer direction remains readable"),Rows[0].WalletDelta,-50LL);
    TestFalse(TEXT("Old historical balances remain explicitly unknown"),Rows[0].Transaction.WalletBalancesAfter.Contains(TEXT("Pilot")));
    Query.bMarketOnly=true;TestEqual(TEXT("Market detail view excludes unrelated contract/refit entries"),TransactionAI::Query(State,Query).Num(),0);
    return true;
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FWalletBoundaryTest,"Starshatter.Economy.Transactions.ArithmeticAndInvalidContractCapture",
    EAutomationTestFlags_ApplicationContextMask|EAutomationTestFlags::EngineFilter)
bool FWalletBoundaryTest::RunTest(const FString&)
{
    FEconomyState State;State.Wallets.Add(TEXT("Pilot"),1000);State.Wallets.Add(TEXT("Full"),MAX_int64);State.Wallets.Add(TEXT("Invalid"),-1);
    TestFalse(TEXT("Same-party transfer cannot create a false receipt"),FEconomyLedger::Pay(State,TEXT("Pilot"),TEXT("Pilot"),5,FGuid::NewGuid(),TEXT("Test")));
    TestFalse(TEXT("Recipient overflow rejected before sender debit"),FEconomyLedger::Pay(State,TEXT("Pilot"),TEXT("Full"),1,FGuid::NewGuid(),TEXT("Test")));
    TestFalse(TEXT("Negative recipient wallet rejected atomically"),FEconomyLedger::Pay(State,TEXT("Pilot"),TEXT("Invalid"),5,FGuid::NewGuid(),TEXT("Test")));
    TestEqual(TEXT("Failed transactions leave pilot funds untouched"),TransactionAI::Balance(State),1000LL);
    TestEqual(TEXT("Failed transactions leave no journal receipts"),State.Journal.Num(),0);
    FContractAccounts Accounts;Accounts.FindOrAdd(TEXT("Pilot")).Credits=1200;Accounts.FindOrAdd(TEXT("BadAccount")).Credits=-1;
    TMap<FName,FContractTransaction> Contracts;FContractTransaction Contract;Contract.Id=FGuid::NewGuid();Contracts.Add(TEXT("BadBatch"),Contract);
    FContractLedger::Capture(State,Contracts,Accounts);
    TestEqual(TEXT("Invalid contract batch does not partially replace a valid balance"),TransactionAI::Balance(State),1000LL);
    TestEqual(TEXT("Invalid contract batch does not create partial receipts"),State.Journal.Num(),0);
    TestTrue(TEXT("Invalid contract batch leaves previous contract state intact"),State.Contracts.IsEmpty());
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FWalletExpiryTest,"Starshatter.Economy.Transactions.RepriceExpiryAndLegacyUnknownDelta",
    EAutomationTestFlags_ApplicationContextMask|EAutomationTestFlags::EngineFilter)
bool FWalletExpiryTest::RunTest(const FString&)
{
    FEconomyState State;FText Reason;State.Wallets.Add(TEXT("Pilot"),1000);
    FEconomyLedger::CreateInventory(State,TEXT("Market"),TEXT("NPC"),TEXT("Janus"),1000)->bMarket=true;
    FEconomyLedger::CreateInventory(State,TEXT("Hold"),TEXT("Pilot"),TEXT("Janus"),100);
    const FGuid Buy=FEconomyLedger::PlaceMarketOrder(State,TEXT("Pilot"),TEXT("Market"),TEXT("Hold"),TEXT("Goods"),EMarketOrderSide::Buy,5,1,100,1000,2000,FGuid::NewGuid(),Reason);
    if(!TestTrue(TEXT("Buy escrow created"),Buy.IsValid()))return false;
    TestTrue(TEXT("Price reduction succeeds"),FEconomyLedger::ModifyMarketOrder(State,TEXT("Pilot"),Buy,80,FGuid::NewGuid(),Reason));
    TestEqual(TEXT("Reprice receipt records actual refund"),State.Journal.Last().WalletDeltas.FindRef(TEXT("Pilot")),100LL);
    TestEqual(TEXT("Reprice receipt captures correct balance"),State.Journal.Last().WalletBalancesAfter.FindRef(TEXT("Pilot")),600LL);
    FEconomyLedger::AdvanceMarketOrders(State,2000);
    TestEqual(TEXT("Expiry receipt refunds remaining collateral"),State.Journal.Last().WalletDeltas.FindRef(TEXT("Pilot")),400LL);
    TestEqual(TEXT("Expiry receipt captures restored balance"),State.Journal.Last().WalletBalancesAfter.FindRef(TEXT("Pilot")),1000LL);
    const int32 ReceiptCount=State.Journal.Num();FEconomyLedger::AdvanceMarketOrders(State,3000);
    TestEqual(TEXT("Repeated expiry never duplicates a credit or receipt"),State.Journal.Num(),ReceiptCount);
    FEconomyTransaction Legacy;Legacy.Id=FGuid::NewGuid();Legacy.From=TEXT("Pilot");Legacy.To=TEXT("Seller");Legacy.Credits=400;Legacy.Reason=TEXT("Limit order fill");State.Journal.Add(Legacy);
    FTransactionQuery Query;Query.Search=TEXT("fill");auto Rows=TransactionAI::Query(State,Query);
    if(!TestEqual(TEXT("Legacy fill remains visible without inventing balance data"),Rows.Num(),1))return false;
    TestFalse(TEXT("Old buyer refund is explicitly unknown"),Rows[0].bWalletDeltaKnown);
    Query.Direction=ETransactionDirection::Incoming;TestTrue(TEXT("Unknown old refund excluded from income filter"),TransactionAI::Query(State,Query).IsEmpty());
    Query.Direction=ETransactionDirection::Outgoing;TestTrue(TEXT("Unknown old escrow debit excluded from expenses filter"),TransactionAI::Query(State,Query).IsEmpty());
    Query.bMarketOnly=true;TestEqual(TEXT("Known old market direction still identifies a purchase"),TransactionAI::Query(State,Query).Num(),1);
    Query.bMarketOnly=false;Query.Direction=ETransactionDirection::All;
    Legacy.Credits=MIN_int64;State.Journal.Add(Legacy);
    TestEqual(TEXT("Invalid negative receipt skipped without signed overflow"),TransactionAI::Query(State,Query).Num(),1);
    return true;
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FContractSaveBoundaryTest,"Starshatter.Economy.Transactions.ContractSaveFailureRollsBackCargoAndReceipts",
    EAutomationTestFlags_ApplicationContextMask|EAutomationTestFlags::EngineFilter)
bool FContractSaveBoundaryTest::RunTest(const FString&)
{
    FEconomyState State;State.Wallets.Add(TEXT("Pilot"),1000);
    FEconomyLedger::CreateInventory(State,TEXT("Hold"),TEXT("Pilot"),TEXT("Janus"),1);
    FContractAccounts Accounts;Accounts.FindOrAdd(TEXT("Pilot")).Credits=1000;
    TMap<FName,FContractTransaction> Contracts;FContractTransaction Offer;Offer.Id=FGuid::NewGuid();Offer.State=EContractState::Open;Contracts.Add(TEXT("Courier"),Offer);
    const FContractSaveSnapshot Before(State,Contracts,Accounts);
    Accounts.FindChecked(TEXT("Pilot")).Credits=750;Contracts.FindChecked(TEXT("Courier")).State=EContractState::InProgress;
    FEconomyLedger::Deposit(State,TEXT("Hold"),TEXT("Courier/Courier"),1,.1);
    bool NativePackage=true;int32 Saves=0;
    TestFalse(TEXT("Failed persistence refuses contract success"),ContractSaveTransaction::Commit(State,Contracts,Accounts,Before,[&]()
    {
        ++Saves;TestEqual(TEXT("Wallet captured before save callback"),State.Wallets.FindRef(TEXT("Pilot")),750LL);
        TestEqual(TEXT("Receipt captured before save callback"),State.Journal.Num(),1);return false;
    },[&]()
    {
        NativePackage=false;
        // A real Ship cargo adapter may synchronize during restoration; final snapshot must win.
        FEconomyLedger::Deposit(State,TEXT("Hold"),TEXT("AdapterDuringRollback"),1,.1);
    }));
    TestEqual(TEXT("Only one save attempt, no compensatory resave"),Saves,1);
    TestFalse(TEXT("Native package restored"),NativePackage);
    TestEqual(TEXT("Spendable wallet restored"),State.Wallets.FindRef(TEXT("Pilot")),1000LL);
    TestEqual(TEXT("Legacy contract account restored"),Accounts.FindChecked(TEXT("Pilot")).Credits,1000LL);
    TestEqual(TEXT("Offer restored"),Contracts.FindChecked(TEXT("Courier")).State,EContractState::Open);
    TestTrue(TEXT("Uncommitted receipts removed"),State.Journal.IsEmpty());
    TestEqual(TEXT("Uncommitted package removed"),FEconomyLedger::Quantity(State,TEXT("Hold"),TEXT("Courier/Courier")),0.0);
    TestEqual(TEXT("Adapter rollback cannot overwrite authoritative snapshot"),FEconomyLedger::Quantity(State,TEXT("Hold"),TEXT("AdapterDuringRollback")),0.0);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FContractCourierIntegrationTest,"Starshatter.Economy.Transactions.CourierSettlementPersistsWithWalletJournal",
    EAutomationTestFlags_ApplicationContextMask|EAutomationTestFlags::EngineFilter)
bool FContractCourierIntegrationTest::RunTest(const FString&)
{
    FEconomyState State;State.Wallets.Add(TEXT("Pilot"),1000);State.Wallets.Add(TEXT("Depot"),1000);
    FContractAccounts Accounts;Accounts.FindOrAdd(TEXT("Pilot")).Credits=1000;auto& Depot=Accounts.FindOrAdd(TEXT("Depot"));Depot.Credits=1000;
    Depot.StationStock.FindOrAdd(TEXT("Archon")).Add(TEXT("Supplies"),1);
    FContractTransaction Contract;Contract.Issuer=TEXT("Depot");Contract.Terms.PickupStationId=TEXT("Archon");Contract.Terms.DestinationStationId=TEXT("Transfer");
    Contract.Terms.OfferExpiryMinutes=60;Contract.Terms.CompletionDays=1;Contract.Terms.RewardCredits=200;Contract.Terms.CollateralCredits=100;
    auto& Item=Contract.Terms.OfferedItems.AddDefaulted_GetRef();Item.ItemId=TEXT("Supplies");Item.Quantity=1;
    const FDateTime Now(2228,11,18);TMap<FName,FContractTransaction> Contracts;
    if(!TestTrue(TEXT("Real courier offer published"),AcceptanceAI::Publish(Contract,Accounts,Now)))return false;
    Contracts.Add(TEXT("Courier"),Contract);FContractLedger::Capture(State,Contracts,Accounts);
    FContractSaveSnapshot Before(State,Contracts,Accounts);
    if(!TestTrue(TEXT("Real courier acceptance debits collateral"),AcceptanceAI::Accept(Contracts.FindChecked(TEXT("Courier")),Accounts,TEXT("Pilot"),Now)))return false;
    TArray<uint8> Bytes;
    const auto Save=[&]()
    {
        auto* Object=NewObject<UPlayerSaveGame>();Object->PlayerInfo.Economy=State;return UGameplayStatics::SaveGameToMemory(Object,Bytes);
    };
    TestTrue(TEXT("Accepted contract and collateral persist together"),ContractSaveTransaction::Commit(State,Contracts,Accounts,Before,Save,[](){}));
    const auto* Loaded=Cast<UPlayerSaveGame>(UGameplayStatics::LoadGameFromMemory(Bytes));
    if(!TestNotNull(TEXT("Accepted courier reloads"),Loaded))return false;
    State=Loaded->PlayerInfo.Economy;FContractLedger::Restore(State,Contracts,Accounts);
    TestEqual(TEXT("Saved collateral debit restored"),Accounts.FindChecked(TEXT("Pilot")).Credits,900LL);
    const int32 Count=State.Journal.Num();FContractLedger::Capture(State,Contracts,Accounts);
    TestEqual(TEXT("Restoring and reading offers cannot duplicate receipts"),State.Journal.Num(),Count);
    Before=FContractSaveSnapshot(State,Contracts,Accounts);
    if(!TestTrue(TEXT("Real courier settles"),SettlementAI::DeliverCourier(Contracts.FindChecked(TEXT("Courier")),Accounts,TEXT("Pilot"),TEXT("Transfer"))))return false;
    TestTrue(TEXT("Settlement and payout persist together"),ContractSaveTransaction::Commit(State,Contracts,Accounts,Before,Save,[](){}));
    TestEqual(TEXT("Reward and collateral credited exactly once"),State.Wallets.FindRef(TEXT("Pilot")),1200LL);
    TestFalse(TEXT("Completed courier cannot pay again"),SettlementAI::DeliverCourier(Contracts.FindChecked(TEXT("Courier")),Accounts,TEXT("Pilot"),TEXT("Transfer")));
    FTransactionQuery Query;int64 Net=0;for(const auto& Row:TransactionAI::Query(State,Query))Net+=Row.WalletDelta;
    TestEqual(TEXT("Journal reconciles actual pilot gain"),Net,200LL);
    return true;
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCourierNativeReloadTest,"Starshatter.Economy.Transactions.CourierNativeAdapterReloadAndCapacity",
    EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FCourierNativeReloadTest::RunTest(const FString&)
{
    const bool OwnLoader=!DataLoader::GetLoader();if(OwnLoader)DataLoader::Initialize();
    ON_SCOPE_EXIT {if(OwnLoader)DataLoader::Close();};
    class FPackageShip final : public Ship
    {
    public:
        explicit FPackageShip(ShipDesign* Design):Ship("Courier reload","",Design,1,0,nullptr,false){}
        bool Restore(const FEconomyState& State){return RestoreCourierPackageFromInventory(State);}
    };
    ShipDesign Design;Design.type=static_cast<int>(CLASSIFICATION::LCA);Design.integrity=100;Design.CargoCapacity.Tonnes=1;
    FPackageShip Craft(&Design);FEconomyState State;
    FEconomyLedger::CreateInventory(State,Craft.GetCargoInventoryKey(),FEconomyLedger::Pilot(),TEXT("Janus"),1);
    FEconomyLedger::Deposit(State,Craft.GetCargoInventoryKey(),TEXT("Courier/Courier"),1,.1);
    FContractTransaction Contract;Contract.Id=FGuid::NewGuid();Contract.State=EContractState::InProgress;Contract.Acceptor=FEconomyLedger::Pilot();State.Contracts.Add(TEXT("Courier"),Contract);
    const FGuid Stack=State.Inventories.FindChecked(Craft.GetCargoInventoryKey()).Items[0].Id;
    auto* Save=NewObject<UPlayerSaveGame>();Save->PlayerInfo.Economy=State;TArray<uint8> Bytes;
    if(!TestTrue(TEXT("Sealed package and accepted contract serialize"),UGameplayStatics::SaveGameToMemory(Save,Bytes)))return false;
    const auto* Loaded=Cast<UPlayerSaveGame>(UGameplayStatics::LoadGameFromMemory(Bytes));if(!TestNotNull(TEXT("Cargo save reloads"),Loaded))return false;
    State=Loaded->PlayerInfo.Economy;
    TestTrue(TEXT("Native Ship adapter restores accepted saved package"),Craft.Restore(State));
    TestTrue(TEXT("Native Ship recognizes its contract package"),Craft.HasCourierPackage(TEXT("Courier")));
    TestTrue(TEXT("Native cargo capacity counts the restored package"),FMath::IsNearlyEqual(Craft.GetCargoUsedTonnes(),.1));
    TestFalse(TEXT("Repeated adapter calls cannot create another package"),Craft.Restore(State));
    TestEqual(TEXT("Restoration deposits no new cargo"),FEconomyLedger::Quantity(State,Craft.GetCargoInventoryKey(),TEXT("Courier/Courier")),1.0);
    TestEqual(TEXT("Restoration preserves saved physical stack identity"),State.Inventories.FindChecked(Craft.GetCargoInventoryKey()).Items[0].Id,Stack);
    FPackageShip Respawn(&Design);
    TestFalse(TEXT("Different hull cannot clone another hull's courier package"),Respawn.Restore(State));
    auto* Foreign=FEconomyLedger::CreateInventory(State,Respawn.GetCargoInventoryKey(),TEXT("OtherPilot"),TEXT("Janus"),1);
    FEconomyLedger::Deposit(State,Respawn.GetCargoInventoryKey(),TEXT("Courier/Courier"),1,.1);
    TestFalse(TEXT("Wrong owner cannot restore sealed package"),Respawn.Restore(State));
    Foreign->Owner=FEconomyLedger::Pilot();State.Contracts.FindChecked(TEXT("Courier")).State=EContractState::Completed;
    TestFalse(TEXT("Completed contract cannot recreate a delivered package"),Respawn.Restore(State));
    State.Contracts.FindChecked(TEXT("Courier")).State=EContractState::InProgress;Design.CargoCapacity.Tonnes=.05;
    TestFalse(TEXT("Overcapacity saved package cannot restore"),Respawn.Restore(State));
    return true;
}
#endif
