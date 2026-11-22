#pragma once

#include "ContractTransactions.h"

class SettlementAI
{
public:
    static bool DeliverCourier(FContractTransaction& Contract, FContractAccounts& Accounts,
        FName Actor, FName Station);
    static bool FailCourier(FContractTransaction& Contract, FContractAccounts& Accounts,
        FName Actor, FDateTime Now, bool bPackageLost);
    static bool CloseExpired(FContractTransaction& Contract, FContractAccounts& Accounts, FDateTime Now);
    static bool ClaimAuction(FContractTransaction& Contract, FContractAccounts& Accounts, FName Actor, bool bApplyGst=false);
};
