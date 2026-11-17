#include "SettlementAI.h"

using namespace ContractTransactions;

bool SettlementAI::DeliverCourier(FContractTransaction& Contract, FContractAccounts& Accounts,
    FName Actor, FName Station)
{
    if (Contract.State != EContractState::InProgress || Contract.Terms.Type != ESSWContractType::Courier ||
        Actor != Contract.Acceptor || Station != Contract.Terms.DestinationStationId)
        return false;
    FContractAccount* Courier = Accounts.Find(Actor);
    FContractAccount* Issuer = Accounts.Find(Contract.Issuer);
    const int64 Payout = Contract.Terms.RewardCredits + Contract.Terms.CollateralCredits;
    if (!Courier || !Issuer || !CanReceiveCredits(*Courier, Payout) ||
        !GiveItems(*Issuer, Station, Contract.Terms.OfferedItems))
        return false;
    Courier->Credits += Payout;
    Contract.State = EContractState::Completed;
    return true;
}

bool SettlementAI::FailCourier(FContractTransaction& Contract, FContractAccounts& Accounts,
    FName Actor, FDateTime Now, bool bPackageLost)
{
    if (Contract.State != EContractState::InProgress || Contract.Terms.Type != ESSWContractType::Courier ||
        (Actor != Contract.Acceptor && (Actor != Contract.Issuer || Now < Contract.DeliverBy)))
        return false;
    FContractAccount* Issuer = Accounts.Find(Contract.Issuer);
    const int64 Refund = Contract.Terms.RewardCredits + Contract.Terms.CollateralCredits;
    if (!Issuer || !CanReceiveCredits(*Issuer, Refund) ||
        (!bPackageLost && !GiveItems(*Issuer, Contract.Terms.PickupStationId, Contract.Terms.OfferedItems)))
        return false;
    Issuer->Credits += Refund;
    Contract.State = EContractState::Failed;
    return true;
}

bool SettlementAI::CloseExpired(FContractTransaction& Contract, FContractAccounts& Accounts, FDateTime Now)
{
    if (Contract.State != EContractState::Open || Now < Contract.ExpiresAt)
        return false;
    if (Contract.Terms.Type == ESSWContractType::Auction && Contract.HighBid > 0)
    {
        Contract.State = EContractState::AwaitingClaims;
        return true;
    }
    FContractAccount* Issuer = Accounts.Find(Contract.Issuer);
    if (!Issuer || !CanReceiveCredits(*Issuer, Contract.Terms.RewardCredits) ||
        !GiveItems(*Issuer, Contract.Terms.PickupStationId, Contract.Terms.OfferedItems))
        return false;
    Issuer->Credits += Contract.Terms.RewardCredits;
    Contract.State = EContractState::Expired;
    return true;
}

bool SettlementAI::ClaimAuction(FContractTransaction& Contract, FContractAccounts& Accounts, FName Actor)
{
    if (Contract.Terms.Type != ESSWContractType::Auction || Contract.State != EContractState::AwaitingClaims)
        return false;
    FContractAccount* Account = Accounts.Find(Actor);
    if (!Account)
        return false;
    if (Actor == Contract.Issuer && !Contract.bSellerClaimed && CanReceiveCredits(*Account, Contract.HighBid))
    {
        Account->Credits += Contract.HighBid;
        Contract.bSellerClaimed = true;
    }
    else if (Actor == Contract.HighBidder && !Contract.bBuyerClaimed &&
        GiveItems(*Account, Contract.Terms.PickupStationId, Contract.Terms.OfferedItems))
    {
        Contract.bBuyerClaimed = true;
    }
    else
    {
        return false;
    }
    if (Contract.bBuyerClaimed && Contract.bSellerClaimed) Contract.State = EContractState::Completed;
    return true;
}
