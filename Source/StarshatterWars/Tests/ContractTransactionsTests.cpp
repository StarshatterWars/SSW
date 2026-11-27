#if WITH_DEV_AUTOMATION_TESTS

#include "AcceptanceAI.h"
#include "BiddingAI.h"
#include "SettlementAI.h"
#include "Misc/AutomationTest.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FContractTransactionsTest, "Starshatter.Industry.ContractTransactions",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FContractTransactionsTest::RunTest(const FString&)
{
    const FName IssuerAccount(TEXT("Issuer")), Buyer(TEXT("Buyer")), Rival(TEXT("Rival"));
    const FName Pickup(TEXT("Pickup")), Destination(TEXT("Destination")), Ore(TEXT("Ore"));
    const FDateTime Now(2228, 11, 17);
    FContractAccounts Accounts;
    Accounts.Add(IssuerAccount).StationStock.FindOrAdd(Pickup).Add(Ore, 10.0);
    Accounts.FindChecked(IssuerAccount).Credits = 10000;
    Accounts.Add(Buyer).Credits = 5000;
    Accounts.Add(Rival).Credits = 5000;

    FContractTransaction Draft;
    Draft.Issuer = IssuerAccount;
    Draft.Terms.PickupStationId = Pickup;
    Draft.Terms.OfferExpiryMinutes = 10;
    auto& Cargo = Draft.Terms.OfferedItems.AddDefaulted_GetRef();
    Cargo.ItemId = Ore;
    Cargo.Quantity = 2;

    auto Courier = Draft;
    Courier.Terms.DestinationStationId = Destination;
    Courier.Terms.CompletionDays = 1;
    Courier.Terms.CollateralCredits = 500;
    Courier.Terms.RewardCredits = 100;
    Courier.Terms.bPrivate = true;
    Courier.Terms.AssignedToId = Buyer;
    TestTrue(TEXT("Publish reserves cargo and reward"), AcceptanceAI::Publish(Courier, Accounts, Now));
    TestFalse(TEXT("Private courier denies the wrong pilot"), AcceptanceAI::Accept(Courier, Accounts, Rival, Now));
    TestTrue(TEXT("Assignee posts collateral"), AcceptanceAI::Accept(Courier, Accounts, Buyer, Now));
    TestFalse(TEXT("Wrong station cannot settle"), SettlementAI::DeliverCourier(Courier, Accounts, Buyer, Pickup));
    TestTrue(TEXT("Correct delivery settles"), SettlementAI::DeliverCourier(Courier, Accounts, Buyer, Destination));
    TestFalse(TEXT("Delivery cannot pay twice"), SettlementAI::DeliverCourier(Courier, Accounts, Buyer, Destination));
    TestEqual(TEXT("Reward and collateral paid"), Accounts.FindChecked(Buyer).Credits, int64(5100));
    TestEqual(TEXT("Cargo credited at the destination"), Accounts.FindChecked(IssuerAccount).StationStock.FindRef(Destination).FindRef(Ore), 2.0);

    auto Exchange = Draft;
    Exchange.Terms.Type = ESSWContractType::ItemExchange;
    Exchange.Terms.PriceCredits = 200;
    Exchange.Terms.RequestedItems.Add(Cargo);
    TestTrue(TEXT("Publish exchange"), AcceptanceAI::Publish(Exchange, Accounts, Now));
    TestFalse(TEXT("Missing requested cargo blocks exchange"), AcceptanceAI::Accept(Exchange, Accounts, Buyer, Now));
    TestEqual(TEXT("Rejected exchange preserves funds"), Accounts.FindChecked(Buyer).Credits, int64(5100));
    Accounts.FindChecked(Buyer).StationStock.FindOrAdd(Pickup).Add(Ore, 2);
    TestTrue(TEXT("Exchange transfers payment and items together"), AcceptanceAI::Accept(Exchange, Accounts, Buyer, Now));

    auto Auction = Draft;
    Auction.Terms.Type = ESSWContractType::Auction;
    Auction.Terms.PriceCredits = 1000;
    TestTrue(TEXT("Publish auction"), AcceptanceAI::Publish(Auction, Accounts, Now));
    TestTrue(TEXT("Opening bid held"), BiddingAI::PlaceBid(Auction, Accounts, Buyer, 1000, Now));
    const auto Late = Now + FTimespan::FromMinutes(9);
    TestFalse(TEXT("Reject bid below increment"), BiddingAI::PlaceBid(Auction, Accounts, Rival, 1100, Late));
    TestTrue(TEXT("Late bid extends auction"), BiddingAI::PlaceBid(Auction, Accounts, Rival, 2000, Late));
    TestEqual(TEXT("Outbid buyer refunded"), Accounts.FindChecked(Buyer).Credits, int64(4900));
    TestFalse(TEXT("Original expiration was extended"), SettlementAI::CloseExpired(Auction, Accounts, Now + FTimespan::FromMinutes(10)));
    TestTrue(TEXT("Auction closes at extended deadline"), SettlementAI::CloseExpired(Auction, Accounts, Auction.ExpiresAt));
    TestTrue(TEXT("Seller claims credits"), SettlementAI::ClaimAuction(Auction, Accounts, IssuerAccount));
    TestFalse(TEXT("Seller cannot claim twice"), SettlementAI::ClaimAuction(Auction, Accounts, IssuerAccount));
    TestTrue(TEXT("Winner claims cargo"), SettlementAI::ClaimAuction(Auction, Accounts, Rival));
    TestTrue(TEXT("Both claims complete auction"), Auction.State == EContractState::Completed);
    return true;
}

#endif
