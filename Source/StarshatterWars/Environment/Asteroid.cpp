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

Asteroid::Asteroid(int t, const FVector& pos, double m)
	: Debris(nullptr, pos, asteroid_velocity, m)
{
    MeshVariant=((t%6)+6)%6;
    // Seed from the authored spawn, independently of visual rotation and mesh choice.
    FRandomStream Deposit(HashCombine(GetTypeHash(pos), GetTypeHash(t)));
    const double TypeRoll = Deposit.GetFraction();
    using Mineral = ESSWMineral;
    if (TypeRoll < 0.70)
    {
        constexpr Mineral Minerals[] = {Mineral::Serpentine, Mineral::Saponite, Mineral::Magnetite, Mineral::Troilite};
        Resource.Type = ESSWAsteroidType::Carbonaceous;
        Resource.Mineral = Minerals[Deposit.RandHelper(UE_ARRAY_COUNT(Minerals))];
    }
    else if (TypeRoll < 0.93)
    {
        constexpr Mineral Minerals[] = {Mineral::Olivine, Mineral::Pyroxene, Mineral::Plagioclase};
        Resource.Type = ESSWAsteroidType::Stony;
        Resource.Rarity = ESSWAsteroidRarity::Uncommon;
        Resource.Mineral = Minerals[Deposit.RandHelper(UE_ARRAY_COUNT(Minerals))];
    }
    else
    {
        constexpr Mineral Minerals[] = {Mineral::Kamacite, Mineral::Taenite, Mineral::Schreibersite};
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
        Resource.Mineral = TypeRoll >= 0.995 ? Mineral::Quantium40 : Mineral::Dilithium;
    }

    // Heavy debris spins at a barely visible 0.14 degrees per second.
    // Seed a sustained, distinct angular velocity for each rock in the field.
    FRandomStream Spin(t);
    const double Speed = Spin.FRandRange(0.14, 0.28);
    SetAngularVelocity(Spin.VRand() * Speed);
    // Only the imported library supplies asteroid geometry and raw bounds.
    radius=library_radii[MeshVariant];
	life = -1;
}

// +--------------------------------------------------------------------+

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
