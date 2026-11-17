#pragma once

#include "CoreMinimal.h"

// Owns the pickup reservation until it is loaded or released.
class ContractAI
{
public:
    bool Reserve(const FString& Item, double& Available, double Requested);
    void CommitPickup(double LoadedTonnes);
    void Release(double& Available);

    const FGuid& GetId() const { return ContractId; }
    const FString& GetItem() const { return CargoItem; }
    double GetReservedTonnes() const { return ReservedTonnes; }

private:
    FGuid ContractId;
    FString CargoItem;
    double ReservedTonnes = 0.0;
};

