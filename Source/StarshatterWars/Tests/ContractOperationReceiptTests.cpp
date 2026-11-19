#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "ContractLedger.h"
#include "AcceptanceAI.h"
#include "BiddingAI.h"
#include "TransactionAI.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FContractOperationReceiptTest,"Starshatter.Economy.Transactions.ContractOperationReceipts",
    EAutomationTestFlags_ApplicationContextMask|EAutomationTestFlags::EngineFilter)
bool FContractOperationReceiptTest::RunTest(const FString&)
{
    FEconomyState State;FContractAccounts Accounts;
    State.Wallets.Add(TEXT("A"),1000);State.Wallets.Add(TEXT("B"),3000);
    Accounts.FindOrAdd(TEXT("A")).Credits=1000;Accounts.FindOrAdd(TEXT("B")).Credits=3000;
    Accounts.FindOrAdd(TEXT("Issuer")).StationStock.FindOrAdd(TEXT("Depot")).Add(TEXT("Parts"),1);
    FContractTransaction Auction;Auction.Issuer=TEXT("Issuer");Auction.Terms.Type=ESSWContractType::Auction;
    Auction.Terms.PickupStationId=TEXT("Depot");Auction.Terms.PriceCredits=1000;Auction.Terms.OfferExpiryMinutes=60;
    auto& Item=Auction.Terms.OfferedItems.AddDefaulted_GetRef();Item.ItemId=TEXT("Parts");Item.Quantity=1;
    const auto Now=FDateTime::UtcNow();
    if(!TestTrue(TEXT("Publish backed auction"),AcceptanceAI::Publish(Auction,Accounts,Now)) ||
       !TestTrue(TEXT("First bidder pays escrow"),BiddingAI::PlaceBid(Auction,Accounts,TEXT("A"),1000,Now)) ||
       !TestTrue(TEXT("Outbid refunds first bidder"),BiddingAI::PlaceBid(Auction,Accounts,TEXT("B"),2000,Now)))return false;
    TMap<FName,FContractTransaction> Contracts;Contracts.Add(TEXT("Auction"),Auction);
    FContractLedger::Capture(State,Contracts,Accounts);
    FTransactionQuery Query;Query.Owner=TEXT("A");Query.bNewestFirst=false;
    const auto Rows=TransactionAI::Query(State,Query);
    if(!TestEqual(TEXT("Net zero batch retains bid and refund separately"),Rows.Num(),2))return false;
    TestEqual(TEXT("Bid debit"),Rows[0].WalletDelta,-1000LL);TestEqual(TEXT("Refund credit"),Rows[1].WalletDelta,1000LL);
    TestEqual(TEXT("Actual balance unchanged"),State.Wallets.FindRef(TEXT("A")),1000LL);
    for(const auto& Row:Rows)TestTrue(TEXT("Every operation names its contract"),Row.Transaction.Reference==Auction.Id);
    TestTrue(TEXT("Distinct immutable operation identities"),Rows[0].Transaction.Id!=Rows[1].Transaction.Id);
    const int32 Receipts=State.Journal.Num();FContractLedger::Capture(State,Contracts,Accounts);
    TestEqual(TEXT("Repeated capture is idempotent"),State.Journal.Num(),Receipts);
    auto& Invalid=Accounts.FindChecked(TEXT("A")).PendingWalletOperations.AddDefaulted_GetRef();
    Invalid.Delta=MIN_int64;Invalid.Contract=Auction.Id;Invalid.Reason=TEXT("Invalid");
    TestFalse(TEXT("Invalid operation rejects entire save boundary"),FContractLedger::CanCapture(State,Accounts));
    Accounts.FindChecked(TEXT("A")).PendingWalletOperations.Reset();
    Accounts.FindChecked(TEXT("A")).StationStock.FindOrAdd(TEXT("Depot")).Add(TEXT("Parts"),1);
    FEconomyLedger::CreateInventory(State,TEXT("ContractInventory/A/Depot"),TEXT("Other"),NAME_None,100);
    TestFalse(TEXT("Foreign inventory prevents the whole contract commit"),FContractLedger::CanCapture(State,Accounts));
    FContractLedger::Capture(State,Contracts,Accounts);
    TestEqual(TEXT("Rejected inventory leaves receipts untouched"),State.Journal.Num(),Receipts);
    TestEqual(TEXT("Rejected inventory leaves credits untouched"),State.Wallets.FindRef(TEXT("A")),1000LL);
    Contracts.Add(TEXT("Copied draft"),Auction);
    TestTrue(TEXT("Legacy copied contract identity is repaired"),FContractLedger::RepairIdentities(Contracts));
    TestTrue(TEXT("Each listing has a different journal identity"),Contracts.FindChecked(TEXT("Auction")).Id!=Contracts.FindChecked(TEXT("Copied draft")).Id);
    const FGuid Repaired=Contracts.FindChecked(TEXT("Copied draft")).Id;
    TestFalse(TEXT("Identity migration is idempotent"),FContractLedger::RepairIdentities(Contracts));
    TestTrue(TEXT("New identity remains stable"),Contracts.FindChecked(TEXT("Copied draft")).Id==Repaired);
    return true;
}
#endif
