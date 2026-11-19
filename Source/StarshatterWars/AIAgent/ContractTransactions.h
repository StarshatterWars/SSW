#pragma once

#include "GameStructs_contractstate.h"

struct FContractWalletOperation
{
    FGuid Id = FGuid::NewGuid(), Contract;
    int64 Delta = 0;
    FName Reason;
};

struct FContractAccount
{
    int64 Credits = 0;
    TArray<FContractWalletOperation> PendingWalletOperations;
    TMap<FName, TMap<FName, double>> StationStock;
};

using FContractAccounts = TMap<FName, FContractAccount>;

// State and balances are owned by the simulation or authoritative economy caller.
namespace ContractTransactions
{
    inline void RecordWallet(FContractAccount& Account, int64 Delta, FName Reason, FGuid Contract)
    {
        if (!Delta) return;
        auto& Operation = Account.PendingWalletOperations.AddDefaulted_GetRef();
        Operation.Delta = Delta; Operation.Reason = Reason; Operation.Contract = Contract;
    }
    inline bool CanReceiveCredits(const FContractAccount& Account, int64 Amount)
    {
        return Amount >= 0 && Account.Credits >= 0 && Account.Credits <= MAX_int64 - Amount;
    }

    inline bool TakeItems(FContractAccount& Account, FName Station, const TArray<FSSWContractItem>& Items)
    {
        auto Stock = Account.StationStock.FindRef(Station);
        for (const auto& Item : Items)
        {
            double* Quantity = Stock.Find(Item.ItemId);
            if (Item.ItemId.IsNone() || !FMath::IsFinite(Item.Quantity) || Item.Quantity <= 0 ||
                !Quantity || !FMath::IsFinite(*Quantity) || *Quantity < Item.Quantity)
                return false;
            *Quantity -= Item.Quantity;
        }
        if (!Items.IsEmpty()) Account.StationStock.Add(Station, MoveTemp(Stock));
        return true;
    }

    inline bool GiveItems(FContractAccount& Account, FName Station, const TArray<FSSWContractItem>& Items)
    {
        auto Stock = Account.StationStock.FindRef(Station);
        for (const auto& Item : Items)
        {
            double& Quantity = Stock.FindOrAdd(Item.ItemId);
            Quantity += Item.Quantity;
            if (!FMath::IsFinite(Quantity)) return false;
        }
        if (!Items.IsEmpty()) Account.StationStock.Add(Station, MoveTemp(Stock));
        return true;
    }

    inline bool MayTake(const FContractTransaction& Contract, FName Actor, FDateTime Now)
    {
        return Contract.State == EContractState::Open && Now < Contract.ExpiresAt &&
            !Actor.IsNone() && Actor != Contract.Issuer &&
            (!Contract.Terms.bPrivate || Actor == Contract.Terms.AssignedToId);
    }
}
