#include "ContractLedger.h"
#include "TransactionAI.h"

bool FContractLedger::RepairIdentities(TMap<FName,FContractTransaction>& Contracts)
{
    TArray<FName> Keys;Contracts.GetKeys(Keys);Keys.Sort(FNameLexicalLess());
    TSet<FGuid> Seen;bool Changed=false;
    for(FName Key:Keys)
    {
        auto& Id=Contracts.FindChecked(Key).Id;
        if(!Id.IsValid() || Seen.Contains(Id)){Id=FGuid::NewGuid();Changed=true;}
        Seen.Add(Id);
    }
    return Changed;
}

bool FContractLedger::CanCapture(const FEconomyState& State, const FContractAccounts& Accounts)
{
    TSet<FGuid> Seen;
    for (const auto& Account : Accounts)
    {
        if (Account.Key.IsNone() || Account.Value.Credits < 0 || State.Wallets.FindRef(Account.Key) < 0) return false;
        int64 BeforeOperations=Account.Value.Credits;
        for (int32 Index=Account.Value.PendingWalletOperations.Num()-1;Index>=0;--Index)
        {
            const auto& Op=Account.Value.PendingWalletOperations[Index];
            if (!Op.Id.IsValid() || !Op.Contract.IsValid() || Op.Reason.IsNone() || Seen.Contains(Op.Id) || Op.Delta==MIN_int64) return false;
            Seen.Add(Op.Id);
            if (State.CommittedTransactions.Contains(Op.Id)) continue;
            if ((Op.Delta>0 && BeforeOperations<Op.Delta) || (Op.Delta<0 && BeforeOperations>MAX_int64+Op.Delta)) return false;
            BeforeOperations-=Op.Delta;
        }
        for (const auto& Station : Account.Value.StationStock)
        {
            const FName Key(*(TEXT("ContractInventory/")+Account.Key.ToString()+TEXT("/")+Station.Key.ToString()));
            const auto* Existing=State.Inventories.Find(Key);
            if(Existing && (Existing->Owner!=Account.Key || !Existing->System.IsNone()))return false;
            for (const auto& Item : Station.Value)
                if (Station.Key.IsNone() || Item.Key.IsNone() || !FMath::IsFinite(Item.Value) || Item.Value < 0 || Item.Value > 1.e15) return false;
        }
    }
    return true;
}

void FContractLedger::Capture(FEconomyState& State, const TMap<FName, FContractTransaction>& Contracts, FContractAccounts& Accounts)
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
    for (auto& Account : Accounts)
    {
        int64 Baseline=Account.Value.Credits;
        for (int32 Index=Account.Value.PendingWalletOperations.Num()-1;Index>=0;--Index)
        {
            const auto& Op=Account.Value.PendingWalletOperations[Index];
            if (!State.CommittedTransactions.Contains(Op.Id)) Baseline-=Op.Delta;
        }
        // Legacy grants/recovery reconcile once, then every native operation gets
        // its own immutable receipt even when a batch nets to zero.
        if (!Account.Value.PendingWalletOperations.IsEmpty()) TransactionAI::ReconcileExternalBalance(State,Account.Key,Baseline,TEXT("Contract account reconciliation"),
            TEXT("Contract escrow"),EWalletTransactionCategory::Contract);
        for (const auto& Op:Account.Value.PendingWalletOperations)
            if (!State.CommittedTransactions.Contains(Op.Id))
                TransactionAI::ReconcileExternalBalance(State,Account.Key,State.Wallets.FindRef(Account.Key)+Op.Delta,
                    Op.Reason,TEXT("Contract escrow"),EWalletTransactionCategory::Contract,Op.Contract,Op.Id);
        Account.Value.PendingWalletOperations.Reset();
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
