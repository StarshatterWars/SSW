#pragma once

#include "CoreMinimal.h"
#include "GameStructs_economy.h"

// Owns the pickup reservation until it is loaded or released.
class ContractAI
{
public:
    bool Reserve(const FString& Item, double& Available, double Requested);
    bool Reserve(FEconomyState&, FName Source, const FString& Item, double Requested);
    void Release(FEconomyState&);
    FName GetSource() const { return SourceInventory; }
    void CommitPickup(double LoadedTonnes);
    void Release(double& Available);

    const FGuid& GetId() const { return ContractId; }
    const FString& GetItem() const { return CargoItem; }
    double GetReservedTonnes() const { return ReservedTonnes; }

private:
    FName SourceInventory;
    FGuid ContractId;
    FString CargoItem;
    double ReservedTonnes = 0.0;
};

