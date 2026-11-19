#include "BiddingAI.h"

using namespace ContractTransactions;

bool BiddingAI::PlaceBid(FContractTransaction& Contract, FContractAccounts& Accounts, FName Bidder,
    int64 Amount, FDateTime Now)
{
    if (Contract.Terms.Type != ESSWContractType::Auction || !MayTake(Contract, Bidder, Now) ||
        Amount < Contract.Terms.PriceCredits || Amount <= Contract.HighBid ||
        (Contract.Buyout > 0 && Amount > Contract.Buyout))
        return false;

    const bool bBuyout = Contract.Buyout > 0 && Amount == Contract.Buyout;
    const int64 Increment = FMath::Max<int64>(Contract.Terms.PriceCredits / 10, 1000);
    if (Contract.HighBid > 0 && !bBuyout && Amount - Contract.HighBid < Increment)
        return false;

    FContractAccount* Buyer = Accounts.Find(Bidder);
    FContractAccount* Previous = Accounts.Find(Contract.HighBidder);
    const bool bSameBidder = Bidder == Contract.HighBidder;
    const int64 Debit = bSameBidder ? Amount - Contract.HighBid : Amount;
    if (!Buyer || Buyer->Credits < Debit ||
        (!bSameBidder && Contract.HighBid > 0 && (!Previous || !CanReceiveCredits(*Previous, Contract.HighBid))))
        return false;

    if (bBuyout && !GiveItems(*Buyer, Contract.Terms.PickupStationId, Contract.Terms.OfferedItems))
        return false;

    Buyer->Credits -= Debit;
    RecordWallet(*Buyer,-Debit,TEXT("Contract auction bid"),Contract.Id);
    if (!bSameBidder && Previous)
    {
        Previous->Credits += Contract.HighBid;
        RecordWallet(*Previous,Contract.HighBid,TEXT("Contract bid refund"),Contract.Id);
    }
    Contract.HighBidder = Bidder;
    Contract.HighBid = Amount;
    if (bBuyout)
    {
        Contract.bBuyerClaimed = true;
        Contract.State = EContractState::AwaitingClaims;
    }
    else
    {
        const FTimespan Extension = FMath::Min(FTimespan::FromMinutes(5), FDateTime::MaxValue() - Now);
        Contract.ExpiresAt = FMath::Max(Contract.ExpiresAt, Now + Extension);
    }
    return true;
}
