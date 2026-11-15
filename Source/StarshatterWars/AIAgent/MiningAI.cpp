#include "MiningAI.h"

#include "Asteroid.h"
#include "SimRegion.h"

MiningAI::MiningAI(Ship* InShip)
    : NavAI(InShip)
{
}

MiningAI::ScanResult MiningAI::ScanAsteroids(SimRegion& Region, const FVector& Origin, double Range, ScanTier Tier)
{
    ScanResult Scan;
    if (!FMath::IsFinite(Range) || Range <= 0.0) return Scan;

    Scan.Region = &Region;
    Scan.Range = Range;
    bool bCompleteCoverage = true;
    for (ListIter<Asteroid> Rocks = Region.GetRoids(); ++Rocks;)
    {
        const Asteroid* Rock = Rocks.value();
        if (!Rock || Rock->GetIntegrity() <= 0.0) continue;

        const double Distance = FVector::Dist(Origin, Rock->GetLocation());
        if (Distance > Range)
        {
            bCompleteCoverage = false;
            continue;
        }

        const auto& Resource = Rock->GetResource();
        const TOptional<double> Quantity = Tier == ScanTier::Assay
            ? TOptional<double>(Resource.AmountTonnes) : TOptional<double>();
        Scan.Readings.Add({Rock->GetContactId(), Distance, Resource.Mineral, Quantity});
        if (Quantity && *Quantity > UE_SMALL_NUMBER) Scan.Status = BeltStatus::HasOre;
    }

    if (Tier == ScanTier::Assay && bCompleteCoverage && !Scan.Readings.IsEmpty() &&
        Scan.Status == BeltStatus::Unknown)
        Scan.Status = BeltStatus::Depleted;

    Scan.Readings.Sort([](const AsteroidReading& A, const AsteroidReading& B)
    {
        return A.Distance == B.Distance ? A.ContactId < B.ContactId : A.Distance < B.Distance;
    });
    return Scan;
}

Asteroid* MiningAI::SelectDeposit(SimRegion& Region, const FVector& Origin, const ScanResult& Scan,
    const TArray<ESSWMineral>& AcceptedMinerals, const FString& LoadedMineral, TOptional<int> PreferredContactId)
{
    if (Scan.Region != &Region || Scan.Readings.IsEmpty()) return nullptr;

    const auto IsCandidate = [&AcceptedMinerals, &LoadedMineral](const AsteroidReading& Reading)
    {
        if (!Reading.RemainingTonnes || *Reading.RemainingTonnes <= UE_SMALL_NUMBER ||
            !AcceptedMinerals.Contains(Reading.Mineral))
            return false;

        if (LoadedMineral.IsEmpty()) return true;
        FSSWAsteroidResource Resource;
        Resource.Mineral = Reading.Mineral;
        return LoadedMineral == Resource.GetMineralName().ToString();
    };

    const auto Resolve = [&Region, &Origin, &Scan](int ContactId) -> Asteroid*
    {
        for (ListIter<Asteroid> Rocks = Region.GetRoids(); ++Rocks;)
        {
            Asteroid* Rock = Rocks.value();
            if (Rock && Rock->GetContactId() == ContactId && Rock->GetIntegrity() > 0 &&
                Rock->GetResource().AmountTonnes > UE_SMALL_NUMBER &&
                FVector::DistSquared(Origin, Rock->GetLocation()) <= FMath::Square(Scan.Range))
                return Rock;
        }
        return nullptr;
    };

    if (PreferredContactId)
    {
        const auto* Preferred = Scan.Readings.FindByPredicate([&PreferredContactId](const AsteroidReading& Reading)
        {
            return Reading.ContactId == *PreferredContactId;
        });
        if (Preferred && IsCandidate(*Preferred))
            if (Asteroid* Rock = Resolve(Preferred->ContactId)) return Rock;
    }
    for (const AsteroidReading& Reading : Scan.Readings)
        if (IsCandidate(Reading))
            if (Asteroid* Rock = Resolve(Reading.ContactId)) return Rock;
    return nullptr;
}
