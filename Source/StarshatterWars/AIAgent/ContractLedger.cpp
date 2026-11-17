#include "ContractLedger.h"

void FContractLedger::Capture(FEconomyState& State, const TMap<FName, FContractTransaction>& Contracts, const FContractAccounts& Accounts)
{
    State.Contracts = Contracts;
    for (const auto& Account : Accounts)
    {
        State.Wallets.Add(Account.Key, Account.Value.Credits);
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
