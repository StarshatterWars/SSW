#pragma once
#include "CoreMinimal.h"
#include "AsteroidResource.h"

// SSW-authored focusing optics, not imported EVE items or server formulas.
struct FMiningCrystalDefinition
{
    FName Id;
    FString DisplayName, Description;
    TArray<ESSWMineral> Minerals;
    bool bUniversal = false;
    double YieldMultiplier = 1;
    FLinearColor Color = FLinearColor(.25f,2.5f,1.8f,1);
    float PulseHz = 1.2f, WidthScale = 1;
    bool Supports(ESSWMineral Mineral) const { return Mineral!=ESSWMineral::None && (bUniversal || Minerals.Contains(Mineral)); }
};

class FMiningCrystalCatalog
{
public:
    static const FMiningCrystalCatalog& Get();
    bool LoadJson(const FString& Json,FText& Error);
    const FMiningCrystalDefinition* Find(FName Id) const { return Crystals.Find(Id); }
    const TMap<FName,FMiningCrystalDefinition>& GetDefinitions() const { return Crystals; }
    double ExtractionMultiplier(FName Id,ESSWMineral Mineral) const;
    // A tint can follow the ore even without an equipped crystal; it never grants yield.
    const FMiningCrystalDefinition* Appearance(FName Id,ESSWMineral Mineral) const;
    static float Pulse(float Elapsed,float Hz);
    static ESSWMineral MineralFromName(FName Name);
private:
    TMap<FName,FMiningCrystalDefinition> Crystals;
};
