/*=============================================================================
    Project:        Starshatter Wars
    Studio:         Fractal Dev Studios
    Copyright:      (C) 2024-2026. All Rights Reserved.

    SUBSYSTEM:      StarshatterWars (Unreal Engine)
    FILE:           StarSystemRegistry.cpp
    AUTHOR:         Carlos Bott

    OVERVIEW
    ========
    Implementation of the StarSystem runtime registry.

    Builds StarSystem objects from FS_Galaxy data and manages
    global registration, lookup, and lifecycle.

=============================================================================*/

#include "StarSystemRegistry.h"

#include "StarSystem.h"
#include "Orbital.h"
#include "OrbitalBody.h"
#include "OrbitalRegion.h"
#include "StarshatterEnvironmentSubsystem.h"

TMap<FName, StarSystem*> StarSystemRegistry::SystemsByName;

// -----------------------------------------------------------------------------
// Lifecycle
// -----------------------------------------------------------------------------

void StarSystemRegistry::Clear(bool bDeleteSystems)
{
    if (bDeleteSystems)
    {
        for (TPair<FName, StarSystem*>& Pair : SystemsByName)
        {
            delete Pair.Value;
        }
    }

    SystemsByName.Empty();
}

// -----------------------------------------------------------------------------
// Registration
// -----------------------------------------------------------------------------

void StarSystemRegistry::RegisterSystem(const FName& RowName, StarSystem* System)
{
    if (!RowName.IsNone() && System)
    {
        SystemsByName.Add(RowName, System);
    }
}

void StarSystemRegistry::RegisterSystem(const FString& Name, StarSystem* System)
{
    if (!Name.IsEmpty() && System)
    {
        SystemsByName.Add(FName(*Name), System);
    }
}

// -----------------------------------------------------------------------------
// Lookup
// -----------------------------------------------------------------------------

StarSystem* StarSystemRegistry::Find(const FName& Name)
{
    if (Name.IsNone())
    {
        return nullptr;
    }

    if (StarSystem* const* Found = SystemsByName.Find(Name))
    {
        return *Found;
    }

    return nullptr;
}

StarSystem* StarSystemRegistry::Find(const FString& Name)
{
    return Name.IsEmpty() ? nullptr : Find(FName(*Name));
}

StarSystem* StarSystemRegistry::Find(const char* Name)
{
    if (!Name || !Name[0])
    {
        return nullptr;
    }

    return Find(FName(ANSI_TO_TCHAR(Name)));
}

// -----------------------------------------------------------------------------
// Utility
// -----------------------------------------------------------------------------

bool StarSystemRegistry::Has(const FName& Name)
{
    return !Name.IsNone() && SystemsByName.Contains(Name);
}

bool StarSystemRegistry::Has(const FString& Name)
{
    return !Name.IsEmpty() && SystemsByName.Contains(FName(*Name));
}

bool StarSystemRegistry::Has(const char* Name)
{
    return Name && Name[0] && SystemsByName.Contains(FName(ANSI_TO_TCHAR(Name)));
}

int32 StarSystemRegistry::Num()
{
    return SystemsByName.Num();
}

const TMap<FName, StarSystem*>& StarSystemRegistry::GetAll()
{
    return SystemsByName;
}

// -----------------------------------------------------------------------------
// Build entry point
// -----------------------------------------------------------------------------

StarSystem* StarSystemRegistry::BuildAndRegister(
    const FS_Galaxy& Row,
    UStarshatterEnvironmentSubsystem* Env)
{
    if (Row.Name.IsEmpty() || !Env)
    {
        return nullptr;
    }

    StarSystem* System = BuildStarSystem(Row, Env);
    if (!System)
    {
        return nullptr;
    }

    RegisterSystem(Row.Name, System);
    return System;
}

// -----------------------------------------------------------------------------
// Internal build helpers
// -----------------------------------------------------------------------------

int32 StarSystemRegistry::ToLegacyStarClass(ESPECTRAL_CLASS InClass)
{
    switch (InClass)
    {
    case ESPECTRAL_CLASS::O:            return Star::O;
    case ESPECTRAL_CLASS::B:            return Star::B;
    case ESPECTRAL_CLASS::A:            return Star::A;
    case ESPECTRAL_CLASS::F:            return Star::F;
    case ESPECTRAL_CLASS::G:            return Star::G;
    case ESPECTRAL_CLASS::K:            return Star::K;
    case ESPECTRAL_CLASS::M:            return Star::M;
    case ESPECTRAL_CLASS::RED_GIANT:    return Star::RED_GIANT;
    case ESPECTRAL_CLASS::WHITE_DWARF:  return Star::WHITE_DWARF;
    case ESPECTRAL_CLASS::BLACK_HOLE:   return Star::BLACK_HOLE;
    default:                            return Star::G;
    }
}

StarSystem* StarSystemRegistry::BuildStarSystem(
    const FS_Galaxy& Row,
    UStarshatterEnvironmentSubsystem* Env)
{
    StarSystem* System = new StarSystem(
        TCHAR_TO_ANSI(*Row.Name),
        Row.Location,
        Row.Iff,
        ToLegacyStarClass(Row.Class));

    if (!System)
    {
        return nullptr;
    }

    // Register ONCE here:
    Env->RegisterStarSystem(System);

    System->SetAffiliation(Row.Iff);
    System->SetLocation(Row.Location);
    System->SetSequence(ToLegacyStarClass(Row.Class));

    for (const FS_StarMap& StarRow : Row.Stellar)
    {
        OrbitalBody* StarBody = BuildStar(System, StarRow, Env);
        if (!StarBody)
        {
            continue;
        }

        for (const FS_RegionMap& RegionRow : StarRow.Region)
        {
            BuildRegion(System, StarBody, RegionRow, Env);
        }

        for (const FS_PlanetMap& PlanetRow : StarRow.Planet)
        {
            OrbitalBody* PlanetBody = BuildPlanet(System, StarBody, PlanetRow, Env);
            if (!PlanetBody)
            {
                continue;
            }

            for (const FS_RegionMap& RegionRow : PlanetRow.Region)
            {
                BuildRegion(System, PlanetBody, RegionRow, Env);
            }

            for (const FS_MoonMap& MoonRow : PlanetRow.Moon)
            {
                OrbitalBody* MoonBody = BuildMoon(System, PlanetBody, MoonRow, Env);
                if (!MoonBody)
                {
                    continue;
                }

                for (const FS_RegionMap& RegionRow : MoonRow.Region)
                {
                    BuildRegion(System, MoonBody, RegionRow, Env);
                }
            }
        }
    }

    System->RecalculateRadius();
    return System;
}

OrbitalBody* StarSystemRegistry::BuildStar(
    StarSystem* System,
    const FS_StarMap& Row,
    UStarshatterEnvironmentSubsystem* Env)
{
    if (!System || !Env)
    {
        return nullptr;
    }

    OrbitalBody* StarBody = new OrbitalBody(
        System,
        TCHAR_TO_ANSI(*Row.Name),
        Orbital::STAR,
        Row.Mass,
        Row.Radius,
        Row.Orbit,
        System->GetCenter());

    StarBody->SetMapName(TCHAR_TO_ANSI(*Row.Map));
    StarBody->SetTextureName(TCHAR_TO_ANSI(*Row.Image));
    StarBody->SetLight(Row.Light);
    StarBody->SetTimeScale(Row.Tscale);
    StarBody->SetRetrograde(Row.Retro);
    StarBody->SetRotation(Row.Rot * 3600.0);
    StarBody->SetColor(Row.Color);
    StarBody->SetBackColor(Row.Back);
    StarBody->SetSubtype(ToLegacyStarClass(Row.Class));

    System->AddBody(StarBody);
    Env->RegisterStar(StarBody);

    return StarBody;
}

OrbitalBody* StarSystemRegistry::BuildPlanet(
    StarSystem* System,
    OrbitalBody* ParentStar,
    const FS_PlanetMap& Row,
    UStarshatterEnvironmentSubsystem* Env)
{
    if (!System || !ParentStar || !Env)
    {
        return nullptr;
    }

    OrbitalBody* PlanetBody = new OrbitalBody(
        System,
        TCHAR_TO_ANSI(*Row.Name),
        Orbital::PLANET,
        Row.Mass,
        Row.Radius,
        Row.Orbit,
        ParentStar);

    PlanetBody->SetMapName(TCHAR_TO_ANSI(*Row.Icon));
    PlanetBody->SetTextureName(TCHAR_TO_ANSI(*Row.Texture));
    PlanetBody->SetGlossTexture(TCHAR_TO_ANSI(*Row.Gloss));
    PlanetBody->SetRingTexture(TCHAR_TO_ANSI(*Row.Ring));
    PlanetBody->SetRingRange(Row.Minrad, Row.Maxrad);
    PlanetBody->SetTimeScale(Row.Tscale);
    PlanetBody->SetTilt(Row.Tilt);
    PlanetBody->SetRetrograde(Row.Retro);
    PlanetBody->SetRotation(Row.Rot * 3600.0);
    PlanetBody->SetAtmosphere(Row.Atmos);

    ParentStar->AddSatellite(PlanetBody);
    Env->RegisterPlanet(PlanetBody);

    return PlanetBody;
}

OrbitalBody* StarSystemRegistry::BuildMoon(
    StarSystem* System,
    OrbitalBody* ParentPlanet,
    const FS_MoonMap& Row,
    UStarshatterEnvironmentSubsystem* Env)
{
    if (!System || !ParentPlanet || !Env)
    {
        return nullptr;
    }

    OrbitalBody* MoonBody = new OrbitalBody(
        System,
        TCHAR_TO_ANSI(*Row.Name),
        Orbital::MOON,
        Row.Mass,
        Row.Radius,
        Row.Orbit,
        ParentPlanet);

    MoonBody->SetMapName(TCHAR_TO_ANSI(*Row.Icon));
    MoonBody->SetTextureName(TCHAR_TO_ANSI(*Row.Texture));
    MoonBody->SetTimeScale(Row.Tscale);
    MoonBody->SetTilt(Row.Tilt);
    MoonBody->SetRetrograde(Row.Retro);
    MoonBody->SetRotation(Row.Rot * 3600.0);
    MoonBody->SetAtmosphere(Row.Atmos);

    ParentPlanet->AddSatellite(MoonBody);
    Env->RegisterMoon(MoonBody);

    return MoonBody;
}

OrbitalRegion* StarSystemRegistry::BuildRegion(
    StarSystem* System,
    Orbital* Parent,
    const FS_RegionMap& Row,
    UStarshatterEnvironmentSubsystem* Env)
{
    if (!System || !Parent || !Env)
    {
        return nullptr;
    }

    OrbitalRegion* Region = new OrbitalRegion(
        System,
        TCHAR_TO_ANSI(*Row.Name),
        0,
        Row.Size,
        Row.Orbit,
        Parent);

    Region->SetGrid(Row.Grid);
    Region->SetInclination(Row.Inclination);
    Region->SetAsteroids(Row.Asteroids);

    for (const FString& LinkName : Row.Link)
    {
        Region->AddLink(TCHAR_TO_ANSI(*LinkName));
    }

    Parent->AddRegion(Region);

    // If your AddRegion adds to root regions AND all_regions,
    // use AddAttachedRegion instead.
    System->AddRegion(Region);

    Env->RegisterRegion(Region);

    return Region;
}