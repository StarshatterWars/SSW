/*  Project Starshatter Wars
	Fractal Dev Studios
	Copyright © 2025-2026. All Rights Reserved.

	ORIGINAL AUTHOR AND STUDIO: John DiCamillo / Destroyer Studios LLC

	SUBSYSTEM:    Stars.exe
	FILE:         Asteroid.cpp
	AUTHOR:       Carlos Bott


	OVERVIEW
	========
	Asteroid Sprite animation class
*/

#include "Asteroid.h"

#include "Math/Vector.h"
#include "Math/UnrealMathUtility.h"
#include "Logging/LogMacros.h"

#include "SimShot.h"
#include "Explosion.h"
#include "Sim.h"

#include "Solid.h"
#include "Game.h"
#include "NativeAsteroidActor.h"
#include "Engine/StaticMesh.h"

// +--------------------------------------------------------------------+

static FVector asteroid_velocity = FVector::ZeroVector;
static double library_radii[6]{};

FText FSSWAsteroidResource::GetMineralName() const
{
    if (Mineral == ESSWMineral::Quantium40)
        return NSLOCTEXT("Asteroid", "Quantium40", "Quantium-40");

    const FString Name = StaticEnum<ESSWMineral>()->GetNameStringByValue(static_cast<int64>(Mineral));
    return FText::FromString(FName::NameToDisplayString(Name, false));
}

FText FSSWAsteroidResource::GetRarityName() const
{
    return StaticEnum<ESSWAsteroidRarity>()->GetDisplayNameTextByValue(static_cast<int64>(Rarity));
}

int32 FSSWAsteroidResource::GetValuePerTonne() const
{
    switch (Rarity)
    {
    case ESSWAsteroidRarity::Uncommon: return 75;
    case ESSWAsteroidRarity::Rare: return 100;
    case ESSWAsteroidRarity::Exotic: return Mineral == ESSWMineral::Quantium40 ? 300 : 150;
    default: return 50;
    }
}

FText FSSWAsteroidResource::GetTypeName() const
{
    switch (Type)
    {
    case ESSWAsteroidType::Stony: return NSLOCTEXT("Asteroid", "SType", "S-type stony");
    case ESSWAsteroidType::Metallic: return NSLOCTEXT("Asteroid", "MType", "M-type metallic");
    default: return NSLOCTEXT("Asteroid", "CType", "C-type carbonaceous");
    }
}

// +--------------------------------------------------------------------+

Asteroid::Asteroid(int t, const FVector& pos, double m, TOptional<ESSWAsteroidType> DominantType)
	: Debris(nullptr, pos, asteroid_velocity, m), ContactId(t)
{
    MeshVariant=((t%6)+6)%6;
    // Seed from the authored spawn, independently of visual rotation and mesh choice.
    FRandomStream Deposit(HashCombine(GetTypeHash(pos), GetTypeHash(t)));
    double TypeRoll = Deposit.GetFraction();
    if (DominantType.IsSet() && TypeRoll < 0.985 && Deposit.GetFraction() < 0.65f)
    {
        switch (DominantType.GetValue())
        {
        case ESSWAsteroidType::Carbonaceous: TypeRoll = 0.35; break;
        case ESSWAsteroidType::Stony: TypeRoll = 0.81; break;
        case ESSWAsteroidType::Metallic: TypeRoll = 0.95; break;
        }
    }
    using Mineral = ESSWMineral;
    if (TypeRoll < 0.70)
    {
        constexpr Mineral Minerals[] = {Mineral::Serpentine, Mineral::Saponite, Mineral::Magnetite, Mineral::Troilite, Mineral::Graphite, Mineral::Pyrite, Mineral::WaterIce};
        Resource.Type = ESSWAsteroidType::Carbonaceous;
        Resource.Mineral = Minerals[Deposit.RandHelper(UE_ARRAY_COUNT(Minerals))];
    }
    else if (TypeRoll < 0.93)
    {
        constexpr Mineral Minerals[] = {Mineral::Olivine, Mineral::Pyroxene, Mineral::Plagioclase, Mineral::Chromite, Mineral::Spinel, Mineral::Apatite, Mineral::Merrillite, Mineral::Diamond, Mineral::Calcite, Mineral::Dolomite, Mineral::Quartz, Mineral::Rutile, Mineral::Zircon, Mineral::Monazite, Mineral::Bauxite, Mineral::Spodumene, Mineral::Fluorite};
        Resource.Type = ESSWAsteroidType::Stony;
        Resource.Rarity = ESSWAsteroidRarity::Uncommon;
        Resource.Mineral = Minerals[Deposit.RandHelper(UE_ARRAY_COUNT(Minerals))];
    }
    else
    {
        constexpr Mineral Minerals[] = {Mineral::Kamacite, Mineral::Taenite, Mineral::Schreibersite, Mineral::Pentlandite, Mineral::Hematite, Mineral::Ilmenite, Mineral::Cassiterite, Mineral::Chalcopyrite, Mineral::Sphalerite, Mineral::Galena, Mineral::Cobaltite, Mineral::Scheelite, Mineral::Wolframite, Mineral::Uraninite};
        Resource.Type = ESSWAsteroidType::Metallic;
        Resource.Rarity = ESSWAsteroidRarity::Rare;
        Resource.Mineral = Minerals[Deposit.RandHelper(UE_ARRAY_COUNT(Minerals))];
    }
    // The surveyed deposit is a small fraction of the simulation mass in kg.
    if (FMath::IsFinite(m) && m > 0.0)
        Resource.AmountTonnes = m * 0.001 * Deposit.FRandRange(0.002f, 0.025f);
    // Use the existing composition roll so other deposits retain their seeded yields.
    if (TypeRoll >= 0.985)
    {
        Resource.Rarity = ESSWAsteroidRarity::Exotic;
        Resource.Mineral = TypeRoll >= 0.999 ? Mineral::KhyberOre : TypeRoll >= 0.995 ? Mineral::Quantium40 : Mineral::Dilithium;
    }

    // Heavy debris spins at a barely visible 0.14 degrees per second.
    // Seed a sustained, distinct angular velocity for each rock in the field.
    FRandomStream Spin(t);
    const double Speed = Spin.FRandRange(0.14, 0.28);
    SetAngularVelocity(Spin.VRand() * Speed);
    // Only the imported library supplies asteroid geometry and raw bounds.
    radius=library_radii[MeshVariant];
    // Scale breakup resistance with rock size, independently of collision mass.
    InitialResourceTonnes = Resource.AmountTonnes;
    InitialIntegrity = integrity = static_cast<float>(FMath::Clamp(radius * 8.0, 500.0, 20000.0));
	life = -1;
}

// +--------------------------------------------------------------------+

double Asteroid::ExtractOre(double Tonnes)
{
    if (!FMath::IsFinite(Tonnes) || Tonnes <= 0) return 0;
    const double Extracted = FMath::Clamp(Tonnes, 0.0, Resource.AmountTonnes);
    Resource.AmountTonnes -= Extracted;
    return Extracted;
}

double Asteroid::ReturnExtractedOre(double Tonnes)
{
    if (!FMath::IsFinite(Tonnes) || Tonnes <= 0) return 0;
    const double Restored = FMath::Min(Tonnes, FMath::Max(0.0, InitialResourceTonnes - Resource.AmountTonnes));
    Resource.AmountTonnes += Restored;
    return Restored;
}

void
Asteroid::Initialize()
{
    FMemory::Memzero(library_radii,sizeof(library_radii));
    const auto* Library=GetDefault<UAsteroidLibrarySettings>();
    for(int I=0;I<6 && Library->Meshes.Num()>0;++I)
        if(auto* Mesh=Library->Meshes[I%Library->Meshes.Num()].LoadSynchronous())library_radii[I]=ANativeAsteroidActor::RawRadius(Mesh->GetBounds().SphereRadius);
    // No MAG model loading: ANativeAsteroidActor renders the configured pack.
    // Debris retains simulation damage and spherical collision using raw radii.
}

void
Asteroid::Close()
{
    FMemory::Memzero(library_radii,sizeof(library_radii));
}
