#pragma once

#include "GameStructs_economy.h"

enum class ETransactionDirection : uint8 { All, Incoming, Outgoing };

struct FTransactionQuery
{
    FName Owner = FEconomyLedger::Pilot();
    ETransactionDirection Direction = ETransactionDirection::All;
    FString Search;
    bool bMarketOnly = false;
    bool bNewestFirst = true;
};

struct FTransactionEntry
{
    FEconomyTransaction Transaction;
    int64 WalletDelta = 0;
    bool bWalletDeltaKnown = true;
    int64 Sequence = 0;
};

// Read-only wallet views; simulation services remain responsible for committing transactions.
class STARSHATTERWARS_API TransactionAI
{
public:
    static TArray<FTransactionEntry> Query(const FEconomyState&, const FTransactionQuery&);
    static int64 Balance(const FEconomyState&, FName Owner = FEconomyLedger::Pilot());
    static int64 BalanceIncludingLegacy(const FEconomyState&, int64 LegacyCredits);
    // Adapter for legacy contract accounts: log an already-calculated balance once, never pay twice.
    static bool ReconcileExternalBalance(FEconomyState&, FName Owner, int64 NewBalance, FName Reason,
        FName Counterparty = FName(TEXT("Contract escrow")), EWalletTransactionCategory Category = EWalletTransactionCategory::Contract, FGuid Reference = FGuid(), FGuid Receipt = FGuid());
};
