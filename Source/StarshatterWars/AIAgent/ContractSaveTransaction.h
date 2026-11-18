#pragma once
#include "ContractLedger.h"

// Save-boundary snapshot for contract accounts, wallets, reservations and receipts.
// Cargo adapters are restored before this snapshot so their synchronization cannot overwrite it.
struct FContractSaveSnapshot
{
    FEconomyState Economy;
    TMap<FName, FContractTransaction> Contracts;
    FContractAccounts Accounts;
    FContractSaveSnapshot(const FEconomyState& InEconomy, const TMap<FName, FContractTransaction>& InContracts, const FContractAccounts& InAccounts)
        : Economy(InEconomy), Contracts(InContracts), Accounts(InAccounts) {}
};

namespace ContractSaveTransaction
{
    inline bool Commit(FEconomyState& Economy, TMap<FName, FContractTransaction>& Contracts, FContractAccounts& Accounts,
        const FContractSaveSnapshot& Before, TFunctionRef<bool()> Save, TFunctionRef<void()> RestoreCargo)
    {
        if (FContractLedger::CanCapture(Economy, Accounts))
        {
            FContractLedger::Capture(Economy, Contracts, Accounts);
            if (Save()) return true;
        }
        RestoreCargo();
        Economy = Before.Economy; Contracts = Before.Contracts; Accounts = Before.Accounts;
        return false;
    }
}
