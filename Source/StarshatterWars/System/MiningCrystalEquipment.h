#pragma once
#include "MiningCrystalCatalog.h"
struct FEconomyState;

// Authoritative auto-focus magazines explicitly provisioned by native JSON.
// A pilot hull uses its existing persistent cargo-inventory identity.
class FMiningCrystalService
{
public:
    static FName FitConfigured(FEconomyState& State,FName Hull,FName ShipDesign,ESSWMineral Mineral);
    static double Multiplier(const FEconomyState& State,FName Hull,FName Crystal,ESSWMineral Mineral);
    static void RecordExtraction(FEconomyState& State,FName Hull,FName Crystal,double AcceptedTonnes);
    static FText Summary(const FEconomyState& State,FName Hull);
    static FName EquipmentKey(FName Hull,FName Crystal);
};
