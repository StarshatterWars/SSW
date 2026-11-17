#include "ContractAI.h"
#include "EconomyRuntime.h"
#include "Engine/DataTable.h"

bool ContractAI::Reserve(const FString& Item, double& Available, double Requested)
{
    if (ContractId.IsValid() || Item.IsEmpty() || !FMath::IsFinite(Available) ||
        !FMath::IsFinite(Requested) || Available <= 0 || Requested <= 0)
        return false;

    ContractId = FGuid::NewGuid();
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


bool ContractAI::Reserve(FEconomyState& State, FName Source, const FString& Item, double Requested)
{
    if (ContractId.IsValid() || Item.IsEmpty() || !FMath::IsFinite(Requested) || Requested <= 0) return false;
    const FName Type = DataTableUtils::MakeValidName(Item);
    const double Amount = FMath::Min(Requested, FEconomyLedger::Available(State, Source, Type));
    const FGuid Id = FEconomyLedger::Reserve(State, FEconomyRuntime::IndustryOwner(), Source, Type, Amount);
    if (!Id.IsValid()) return false;
    ContractId = Id; SourceInventory = Source; CargoItem = Item; ReservedTonnes = Amount;
    return true;
}
void ContractAI::Release(FEconomyState& State)
{
    if (ContractId.IsValid()) FEconomyLedger::Release(State, FEconomyRuntime::IndustryOwner(), ContractId);
    *this = {};
}
