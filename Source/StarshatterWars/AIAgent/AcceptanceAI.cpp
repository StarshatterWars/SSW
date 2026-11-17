#include "AcceptanceAI.h"

using namespace ContractTransactions;

bool AcceptanceAI::Publish(FContractTransaction& Contract, FContractAccounts& Accounts, FDateTime Now)
{
    const auto& Terms = Contract.Terms;
    FContractAccount* Issuer = Accounts.Find(Contract.Issuer);
    if (Contract.State != EContractState::Draft || Contract.Issuer.IsNone() || !Issuer ||
        Terms.PickupStationId.IsNone() || Terms.OfferExpiryMinutes <= 0 ||
        Terms.OfferExpiryMinutes > (FDateTime::MaxValue() - Now).GetTotalMinutes() ||
        (Terms.bPrivate && Terms.AssignedToId.IsNone()) || Terms.bMultiContract ||
        Terms.PriceCredits < 0 || Terms.RewardCredits < 0 || Terms.CollateralCredits < 0 ||
        Issuer->Credits < Terms.RewardCredits)
        return false;

    switch (Terms.Type)
    {
    case ESSWContractType::Courier:
        if (Terms.DestinationStationId.IsNone() || Terms.CompletionDays <= 0 || Terms.OfferedItems.IsEmpty() ||
            !Terms.RequestedItems.IsEmpty() || Terms.PriceCredits != 0 ||
            Terms.RewardCredits > MAX_int64 - Terms.CollateralCredits)
            return false;
        break;
    case ESSWContractType::ItemExchange:
        if (Terms.CollateralCredits != 0 || (Terms.OfferedItems.IsEmpty() && Terms.RewardCredits == 0) ||
            (Terms.PriceCredits != 0 && Terms.RewardCredits != 0))
            return false;
        for (const auto& Item : Terms.RequestedItems)
            if (Item.ItemId.IsNone() || !FMath::IsFinite(Item.Quantity) || Item.Quantity <= 0)
                return false;
        break;
    case ESSWContractType::Auction:
        if (Terms.OfferedItems.IsEmpty() || !Terms.RequestedItems.IsEmpty() || Terms.RewardCredits != 0 ||
            Terms.CollateralCredits != 0 || Terms.PriceCredits <= 0 || Contract.Buyout < 0 ||
            (Contract.Buyout > 0 && Contract.Buyout < Terms.PriceCredits))
            return false;
        break;
    default:
        return false;
    }

    if (!TakeItems(*Issuer, Terms.PickupStationId, Terms.OfferedItems))
        return false;
    if (!Contract.Id.IsValid()) Contract.Id=FGuid::NewGuid();
    Issuer->Credits -= Terms.RewardCredits;
    Contract.ExpiresAt = Now + FTimespan::FromMinutes(Terms.OfferExpiryMinutes);
    Contract.State = EContractState::Open;
    return true;
}

bool AcceptanceAI::Accept(FContractTransaction& Contract, FContractAccounts& Accounts, FName Actor, FDateTime Now)
{
    if (!MayTake(Contract, Actor, Now))
        return false;
    FContractAccount* Buyer = Accounts.Find(Actor);
    FContractAccount* Issuer = Accounts.Find(Contract.Issuer);
    if (!Buyer || !Issuer)
        return false;

    const auto& Terms = Contract.Terms;
    if (Terms.Type == ESSWContractType::Courier)
    {
        if (Buyer->Credits < Terms.CollateralCredits ||
            Terms.CompletionDays > (FDateTime::MaxValue() - Now).GetTotalDays())
            return false;
        Buyer->Credits -= Terms.CollateralCredits;
        Contract.DeliverBy = Now + FTimespan::FromDays(Terms.CompletionDays);
        Contract.State = EContractState::InProgress;
    }
    else if (Terms.Type == ESSWContractType::ItemExchange)
    {
        if (Buyer->Credits < Terms.PriceCredits || !CanReceiveCredits(*Issuer, Terms.PriceCredits) ||
            !CanReceiveCredits(*Buyer, Terms.RewardCredits))
            return false;
        FContractAccount BuyerAfter = *Buyer;
        FContractAccount IssuerAfter = *Issuer;
        if (!TakeItems(BuyerAfter, Terms.PickupStationId, Terms.RequestedItems) ||
            !GiveItems(IssuerAfter, Terms.PickupStationId, Terms.RequestedItems) ||
            !GiveItems(BuyerAfter, Terms.PickupStationId, Terms.OfferedItems))
            return false;
        BuyerAfter.Credits += Terms.RewardCredits - Terms.PriceCredits;
        IssuerAfter.Credits += Terms.PriceCredits;
        *Buyer = MoveTemp(BuyerAfter);
        *Issuer = MoveTemp(IssuerAfter);
        Contract.State = EContractState::Completed;
    }
    else
    {
        return false;
    }
    Contract.Acceptor = Actor;
    return true;
}
