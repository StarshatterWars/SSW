#pragma once

#include "ContractTransactions.h"

class BiddingAI
{
public:
    static bool PlaceBid(FContractTransaction& Contract, FContractAccounts& Accounts, FName Bidder,
        int64 Amount, FDateTime Now);
};
