#include "TransactionAI.h"
#include "SimDataEvents.h"

#include "Misc/DateTime.h"

int64 TransactionAI::Balance(const FEconomyState& State, FName Owner)
{
    return State.Wallets.FindRef(Owner);
}

int64 TransactionAI::BalanceIncludingLegacy(const FEconomyState& State, int64 LegacyCredits)
{
    const int64 Current = FMath::Max(0LL, Balance(State));
    const int64 Pending = State.bLegacyWalletMigrated ? 0 : FMath::Max(0LL, LegacyCredits);
    return Current + FMath::Min(MAX_int64 - Current, Pending);
}

TArray<FTransactionEntry> TransactionAI::Query(const FEconomyState& State, const FTransactionQuery& Filter)
{
    TArray<FTransactionEntry> Result;
    const FString Search = Filter.Search.TrimStartAndEnd();
    int64 Sequence = 0;
    for (const auto& Receipt : State.Journal)
    {
        ++Sequence;
        if (Receipt.Credits < 0) continue; // Invalid legacy/corrupt receipts cannot produce signed-overflow or invented income.
        const bool Explicit = Receipt.bHasWalletDeltas;
        const bool InvolvesOwner = Explicit ? Receipt.WalletDeltas.Contains(Filter.Owner) || Receipt.From == Filter.Owner || Receipt.To == Filter.Owner
            : Receipt.From == Filter.Owner || Receipt.To == Filter.Owner;
        if (!InvolvesOwner) continue;
        const bool Market = Receipt.Category == EWalletTransactionCategory::Market || Receipt.Reason == TEXT("Limit order fill") || Receipt.Reason == TEXT("Market purchase");
        if (Filter.bMarketOnly && !Market) continue;
        // Old receipts predate exact wallet deltas. Escrow fills debit escrow, not the buyer wallet again.
        const bool LegacyEscrowFill = Receipt.Reason == TEXT("Limit order fill");
        const bool DeltaKnown = Explicit || !LegacyEscrowFill || Receipt.From != Filter.Owner;
        const int64 Delta = Explicit ? Receipt.WalletDeltas.FindRef(Filter.Owner)
            : (Receipt.To == Filter.Owner ? Receipt.Credits : 0) - (Receipt.From == Filter.Owner && !LegacyEscrowFill ? Receipt.Credits : 0);
        // Market direction means goods sold/bought; wallet direction means actual spendable-credit movement.
        const bool Incoming = Filter.bMarketOnly ? Receipt.To == Filter.Owner : DeltaKnown && Delta > 0;
        const bool Outgoing = Filter.bMarketOnly ? Receipt.From == Filter.Owner : DeltaKnown && Delta < 0;
        if ((Filter.Direction == ETransactionDirection::Incoming && !Incoming) ||
            (Filter.Direction == ETransactionDirection::Outgoing && !Outgoing)) continue;
        const FString Haystack = Receipt.Reason.ToString() + TEXT(" ") + Receipt.Item.ToString() + TEXT(" ") + Receipt.From.ToString() + TEXT(" ") + Receipt.To.ToString()
            + TEXT(" ") + Receipt.Source.ToString() + TEXT(" ") + Receipt.Destination.ToString()
            + TEXT(" ") + Receipt.Id.ToString() + TEXT(" ") + Receipt.Reference.ToString();
        if (!Search.IsEmpty() && !Haystack.Contains(Search, ESearchCase::IgnoreCase)) continue;
        FTransactionEntry Entry; Entry.Transaction = Receipt; Entry.WalletDelta = Delta; Entry.bWalletDeltaKnown = DeltaKnown; Entry.Sequence = Sequence;
        Result.Add(MoveTemp(Entry));
    }
    Result.StableSort([&](const FTransactionEntry& Left, const FTransactionEntry& Right)
    {
        const bool Earlier = Left.Transaction.TimestampUtc != Right.Transaction.TimestampUtc
            ? Left.Transaction.TimestampUtc < Right.Transaction.TimestampUtc : Left.Sequence < Right.Sequence;
        const bool Later = Left.Transaction.TimestampUtc != Right.Transaction.TimestampUtc
            ? Left.Transaction.TimestampUtc > Right.Transaction.TimestampUtc : Left.Sequence > Right.Sequence;
        return Filter.bNewestFirst ? Later : Earlier;
    });
    return Result;
}

bool TransactionAI::ReconcileExternalBalance(FEconomyState& State, FName Owner, int64 NewBalance, FName Reason, FName Counterparty, EWalletTransactionCategory Category, FGuid Reference, FGuid Receipt)
{
    if (Owner.IsNone() || NewBalance < 0 || State.Wallets.FindRef(Owner) < 0) return false;
    if (Receipt.IsValid() && State.CommittedTransactions.Contains(Receipt)) return true;
    const int64 Previous = State.Wallets.FindRef(Owner);
    if (Previous == NewBalance) return true;
    FEconomyTransaction Entry; Entry.Id = Receipt.IsValid() ? Receipt : FGuid::NewGuid(); Entry.Reason = Reason;
    Entry.TimestampUtc = FDateTime::UtcNow().ToUnixTimestamp(); Entry.bHasWalletDeltas = true;
    Entry.WalletDeltas.Add(Owner, NewBalance - Previous);
    Entry.WalletBalancesAfter.Add(Owner, NewBalance); Entry.Category = Category;
    Entry.Reference = Reference;
    Entry.Credits = NewBalance > Previous ? NewBalance - Previous : Previous - NewBalance;
    Entry.From = NewBalance > Previous ? Counterparty : Owner;
    Entry.To = NewBalance > Previous ? Owner : Counterparty;
    State.Wallets.Add(Owner, NewBalance); State.CommittedTransactions.Add(Entry.Id); State.Journal.Add(MoveTemp(Entry)); FSimDataEvents::Wallet(State,Owner,Counterparty);
    if (State.Journal.Num() > 512) State.Journal.RemoveAt(0, State.Journal.Num() - 512);
    return true;
}
