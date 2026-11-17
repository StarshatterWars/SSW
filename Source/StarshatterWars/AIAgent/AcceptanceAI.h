#pragma once

#include "ContractTransactions.h"

class AcceptanceAI
{
public:
    static bool Publish(FContractTransaction& Contract, FContractAccounts& Accounts, FDateTime Now);
    static bool Accept(FContractTransaction& Contract, FContractAccounts& Accounts, FName Actor, FDateTime Now);
};
