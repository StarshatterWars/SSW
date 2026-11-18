#include "ContractLedger.h"
#include "TransactionAI.h"

bool FContractLedger::CanCapture(const FEconomyState& State, const FContractAccounts& Accounts)
{
    for (const auto& Account : Accounts)
    {
        if (Account.Key.IsNone() || Account.Value.Credits < 0 || State.Wallets.FindRef(Account.Key) < 0) return false;
        for (const auto& Station : Account.Value.StationStock)
            for (const auto& Item : Station.Value)
                if (Station.Key.IsNone() || Item.Key.IsNone() || !FMath::IsFinite(Item.Value) || Item.Value < 0 || Item.Value > 1.e15) return false;
    }
    return true;
}

void FContractLedger::Capture(FEconomyState& State, const TMap<FName, FContractTransaction>& Contracts, const FContractAccounts& Accounts)
{
    // Capture is an all-account boundary: malformed legacy state must not partially overwrite live wallets.
    if (!CanCapture(State, Accounts)) return;
    FGuid Reference;
    FName Reason(TEXT("Contract settlement and escrow"));
    int32 Changed = 0;
    for (const auto& Pair : Contracts)
    {
        const auto* Old = State.Contracts.Find(Pair.Key); const auto& Current = Pair.Value;
        if (Old && Old->State == Current.State && Old->HighBid == Current.HighBid && Old->HighBidder == Current.HighBidder &&
            Old->Acceptor == Current.Acceptor && Old->bSellerClaimed == Current.bSellerClaimed && Old->bBuyerClaimed == Current.bBuyerClaimed) continue;
        ++Changed; Reference = Current.Id;
        Reason = Current.State == EContractState::Completed ? FName(TEXT("Contract completed"))
            : Current.State == EContractState::Failed ? FName(TEXT("Contract failed"))
            : Current.State == EContractState::Expired ? FName(TEXT("Contract expired"))
            : Current.State == EContractState::InProgress ? FName(TEXT("Contract acceptance and collateral"))
            : Current.HighBid > 0 ? FName(TEXT("Contract auction bid and settlement")) : FName(TEXT("Contract publication and escrow"));
    }
    if (Changed != 1) { Reference.Invalidate(); Reason = TEXT("Contract settlement and escrow"); }
    State.Contracts = Contracts;
    for (const auto& Account : Accounts)
    {
        TransactionAI::ReconcileExternalBalance(State, Account.Key, Account.Value.Credits, Reason, TEXT("Contract escrow"), EWalletTransactionCategory::Contract, Reference);
        for (const auto& Station : Account.Value.StationStock)
        {
            const FName Key(*(TEXT("ContractInventory/") + Account.Key.ToString() + TEXT("/") + Station.Key.ToString()));
            auto* Inventory = FEconomyLedger::CreateInventory(State, Key, Account.Key, NAME_None, 1.e15);
            if (!Inventory) continue;
            for (auto& Item : Inventory->Items) Item.Quantity = 0;
            for (const auto& Item : Station.Value)
            {
                auto* Stack = Inventory->Items.FindByPredicate([&](const auto& Entry) { return Entry.Type == Item.Key; });
                if (!Stack) { Stack = &Inventory->Items.AddDefaulted_GetRef(); Stack->Id = FGuid::NewGuid(); Stack->Type = Item.Key; }
                Stack->Quantity = Item.Value;
            }
        }
    }
}
void FContractLedger::Restore(const FEconomyState& State, TMap<FName, FContractTransaction>& Contracts, FContractAccounts& Accounts)
{
    Contracts = State.Contracts; Accounts.Reset();
    for (const auto& Pair : State.Wallets) Accounts.FindOrAdd(Pair.Key).Credits = Pair.Value;
    for (const auto& Pair : State.Inventories)
    {
        FString Suffix = Pair.Key.ToString();
        if (!Suffix.RemoveFromStart(TEXT("ContractInventory/"))) continue;
        FString Owner, Station;
        if (!Suffix.Split(TEXT("/"), &Owner, &Station)) continue;
        auto& Stock = Accounts.FindOrAdd(FName(*Owner)).StationStock.FindOrAdd(FName(*Station));
        for (const auto& Item : Pair.Value.Items) Stock.Add(Item.Type, Item.Quantity);
    }
}
