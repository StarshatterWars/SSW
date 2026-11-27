#pragma once
#include "GameStructs_economy.h"
namespace NpcTradeAI
{
    // Persist must save this exact live player state; failure rolls back the entire local change.
    bool Stage(FEconomyState&,const FNpcTradeReceipt&,TFunctionRef<bool()> Persist,FText&);
    bool Resolve(FEconomyState&,FGuid Receipt,bool bServerCommitted,TFunctionRef<bool()> Persist,FText&);
    bool HasPending(const FEconomyState&);
    FName Escrow(FGuid);
}
