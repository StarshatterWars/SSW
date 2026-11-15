#include "ContractAI.h"

bool ContractAI::Reserve(const FString& Item, double& Available, double Requested)
{
    if (ReservedTonnes > 0 || Item.IsEmpty() || !FMath::IsFinite(Available) ||
        !FMath::IsFinite(Requested) || Available <= 0 || Requested <= 0)
        return false;

    CargoItem = Item;
    ReservedTonnes = FMath::Min(Available, Requested);
    Available -= ReservedTonnes;
    return true;
}

void ContractAI::CommitPickup(double LoadedTonnes)
{
    if (FMath::IsFinite(LoadedTonnes) && LoadedTonnes > 0)
        ReservedTonnes = FMath::Max(0.0, ReservedTonnes - LoadedTonnes);
}

void ContractAI::Release(double& Available)
{
    Available += ReservedTonnes;
    *this = {};
}

