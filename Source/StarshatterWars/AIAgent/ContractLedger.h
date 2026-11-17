#pragma once
#include "ContractTransactions.h"
#include "GameStructs_economy.h"
// Persistence adapter for the existing atomic Acceptance/Bidding/Settlement handlers.
class FContractLedger
{
public:
    static void Capture(FEconomyState&, const TMap<FName, FContractTransaction>&, const FContractAccounts&);
    static void Restore(const FEconomyState&, TMap<FName, FContractTransaction>&, FContractAccounts&);
};
