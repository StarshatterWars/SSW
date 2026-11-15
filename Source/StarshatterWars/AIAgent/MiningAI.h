#pragma once

#include "AsteroidResource.h"
#include "NavAI.h"

class Asteroid;
class SimRegion;

class MiningAI : public NavAI
{
public:
    enum class ScanTier : uint8 { Survey, Assay };
    enum class BeltStatus : uint8 { Unknown, HasOre, Depleted };

    struct AsteroidReading
    {
        int ContactId;
        double Distance;
        ESSWMineral Mineral;
        TOptional<double> RemainingTonnes;
    };

    struct ScanResult
    {
        const SimRegion* Region = nullptr;
        TArray<AsteroidReading> Readings;
        BeltStatus Status = BeltStatus::Unknown;
        double Range = 0.0;
    };

    explicit MiningAI(Ship* InShip);

    static ScanResult ScanAsteroids(SimRegion& Region, const FVector& Origin, double Range, ScanTier Tier);
    static Asteroid* SelectDeposit(SimRegion& Region, const FVector& Origin, const ScanResult& Scan,
        const TArray<ESSWMineral>& AcceptedMinerals, const FString& LoadedMineral, TOptional<int> PreferredContactId);
};
