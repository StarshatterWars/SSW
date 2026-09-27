/*  Project Starshatter Wars
    Fractal Dev Studios
    Copyright © 2025-2026. All Rights Reserved.

    ORIGINAL AUTHOR AND STUDIO:
    John DiCamillo, Destroyer Studios LLC
    Copyright © 1997-2004. All Rights Reserved.

    SUBSYSTEM:    Stars.exe
    FILE:         Galaxy.cpp
    AUTHOR:       Carlos Bott

    OVERVIEW
    ========
    Galaxy (list of star systems) for a single campaign.
*/

#include "Galaxy.h"
#include "StarSystem.h"
#include "Starshatter.h"

#include "Game.h"
#include "List.h"

#include "StarshatterEnvironmentSubsystem.h"
#include "GameStructs.h"
#include "GameStructs_System.h"
#include "StarSystemRegistry.h"

#include "Math/Vector.h"
#include "Logging/LogMacros.h"

DEFINE_LOG_CATEGORY_STATIC(LogTempGalaxy, Log, All);

static Galaxy* galaxy = nullptr;

static int ConvertSpectralClass(ESPECTRAL_CLASS InClass)
{
    switch (InClass)
    {
    case ESPECTRAL_CLASS::O:           return Star::O;
    case ESPECTRAL_CLASS::B:           return Star::B;
    case ESPECTRAL_CLASS::A:           return Star::A;
    case ESPECTRAL_CLASS::F:           return Star::F;
    case ESPECTRAL_CLASS::G:           return Star::G;
    case ESPECTRAL_CLASS::K:           return Star::K;
    case ESPECTRAL_CLASS::M:           return Star::M;
    case ESPECTRAL_CLASS::RED_GIANT:   return Star::RED_GIANT;
    case ESPECTRAL_CLASS::WHITE_DWARF: return Star::WHITE_DWARF;
    case ESPECTRAL_CLASS::BLACK_HOLE:  return Star::BLACK_HOLE;
    default:                           return Star::G;
    }
}

static bool RegionAlreadyExists(StarSystem* System, const FString& RegionName)
{
    if (!System || RegionName.IsEmpty())
    {
        return false;
    }

    return System->FindRegion(TCHAR_TO_ANSI(*RegionName)) != nullptr;
}

static OrbitalRegion* BuildRegionFromTable(
    StarSystem* System,
    const FRegion& RegionRow)
{
    if (!System || RegionRow.Name.IsEmpty())
    {
        return nullptr;
    }

    Orbital* Parent = nullptr;

    if (!RegionRow.Parent.IsEmpty())
    {
        Parent = System->FindOrbital(TCHAR_TO_ANSI(*RegionRow.Parent));
    }

    OrbitalRegion* Region = new OrbitalRegion(
        System,
        TCHAR_TO_ANSI(*RegionRow.Name),
        0.0,
        RegionRow.Size,
        RegionRow.Orbit,
        Parent);

    if (!Region)
    {
        return nullptr;
    }

    Region->SetGrid(RegionRow.Grid);
    Region->SetInclination(RegionRow.Inclination);
    Region->SetAsteroids(RegionRow.Asteroids);

    for (const FString& LinkName : RegionRow.Link)
    {
        Region->AddLink(TCHAR_TO_ANSI(*LinkName));
    }

    if (Parent)
    {
        Parent->AddRegion(Region);
    }
    else
    {
        System->GetRegions().append(Region);
    }

    System->GetAllRegions().append(Region);

    return Region;
}

static OrbitalRegion* BuildRegionFromMap(
    StarSystem* System,
    const FRegion& RegionRow,
    Orbital* Parent)
{
    if (!System)
    {
        return nullptr;
    }

    OrbitalRegion* Region = new OrbitalRegion(
        System,
        TCHAR_TO_ANSI(*RegionRow.Name),
        0.0,
        RegionRow.Size,
        RegionRow.Orbit,
        Parent);

    if (!Region)
    {
        return nullptr;
    }

    Region->SetGrid(RegionRow.Grid);
    Region->SetInclination(RegionRow.Inclination);
    Region->SetAsteroids(RegionRow.Asteroids);

    for (const FString& LinkName : RegionRow.Link)
    {
        Region->AddLink(TCHAR_TO_ANSI(*LinkName));
    }

    if (Parent)
    {
        Parent->AddRegion(Region);
    }
    else
    {
        System->AddRegion(Region);
    }

    return Region;
}

static OrbitalBody* BuildMoonFromMap(
    StarSystem* System,
    const FMoon& MoonRow,
    OrbitalBody* ParentPlanet)
{
    if (!System || !ParentPlanet)
    {
        return nullptr;
    }

    OrbitalBody* Moon = new OrbitalBody(
        System,
        TCHAR_TO_ANSI(*MoonRow.Name),
        Orbital::MOON,
        MoonRow.Mass,
        MoonRow.Radius,
        MoonRow.Orbit,
        ParentPlanet);

    if (!Moon)
    {
        return nullptr;
    }

    Moon->SetMapName(TCHAR_TO_ANSI(*MoonRow.Icon));
    Moon->SetTexture(TCHAR_TO_ANSI(*MoonRow.Texture));
    Moon->SetRotation(MoonRow.Rot * 3600.0);
    Moon->SetRetro(MoonRow.Retro);
    Moon->SetTimeScale(MoonRow.Tscale);
    Moon->SetTilt(MoonRow.Tilt);
    Moon->SetAtmosphere(MoonRow.Atmos);

    ParentPlanet->AddSatellite(Moon);

    for (const FRegion& RegionRow : MoonRow.Region)
    {
        BuildRegionFromMap(System, RegionRow, Moon);
    }

    return Moon;
}

static OrbitalBody* BuildPlanetFromMap(
    StarSystem* System,
    const FPlanet& PlanetRow,
    OrbitalBody* ParentStar)
{
    if (!System || !ParentStar)
    {
        return nullptr;
    }

    OrbitalBody* Planet = new OrbitalBody(
        System,
        TCHAR_TO_ANSI(*PlanetRow.Name),
        Orbital::PLANET,
        PlanetRow.Mass,
        PlanetRow.Radius,
        PlanetRow.Orbit,
        ParentStar);

    if (!Planet)
    {
        return nullptr;
    }

    Planet->SetMapName(TCHAR_TO_ANSI(*PlanetRow.Icon));
    Planet->SetTexture(TCHAR_TO_ANSI(*PlanetRow.Texture));
    Planet->SetRingTexture(TCHAR_TO_ANSI(*PlanetRow.Ring));
    Planet->SetGlossTexture(TCHAR_TO_ANSI(*PlanetRow.Gloss));
    Planet->SetRingRange(PlanetRow.Minrad, PlanetRow.Maxrad);
    Planet->SetRotation(PlanetRow.Rot * 3600.0);
    Planet->SetRetro(PlanetRow.Retro);
    Planet->SetTimeScale(PlanetRow.Tscale);
    Planet->SetTilt(PlanetRow.Tilt);
    Planet->SetAtmosphere(PlanetRow.Atmos);

    ParentStar->AddSatellite(Planet);

    for (const FRegion& RegionRow : PlanetRow.Region)
    {
        BuildRegionFromMap(System, RegionRow, Planet);
    }

    for (const FMoon& MoonRow : PlanetRow.Moon)
    {
        BuildMoonFromMap(System, MoonRow, Planet);
    }

    return Planet;
}

static OrbitalBody* BuildStarFromMap(
    StarSystem* System,
    const FStarSystem& StarRow)
{
    if (!System)
    {
        return nullptr;
    }

    OrbitalBody* StarBody = new OrbitalBody(
        System,
        TCHAR_TO_ANSI(*StarRow.Name),
        Orbital::STAR,
        StarRow.Mass,
        StarRow.Radius,
        StarRow.Orbit,
        System->GetCenter());

    if (!StarBody)
    {
        return nullptr;
    }

    StarBody->SetMapName(TCHAR_TO_ANSI(*StarRow.Map));
    StarBody->SetTexture(TCHAR_TO_ANSI(*StarRow.Image));
    StarBody->SetLighting(StarRow.Light, StarRow.Color);
    StarBody->SetBackColor(StarRow.Back);
    StarBody->SetRotation(StarRow.Rot * 3600.0);
    StarBody->SetRetro(StarRow.Retro);
    StarBody->SetTimeScale(StarRow.Tscale);
    StarBody->SetSubtype(ConvertSpectralClass(StarRow.Class));

    System->AddBody(StarBody);

    for (const FRegion& RegionRow : StarRow.Region)
    {
        BuildRegionFromMap(System, RegionRow, StarBody);
    }

    for (const FPlanet& PlanetRow : StarRow.Planet)
    {
        BuildPlanetFromMap(System, PlanetRow, StarBody);
    }

    return StarBody;
}

// +--------------------------------------------------------------------+

Galaxy::Galaxy(const char* n)
    : name(n), radius(10)
{
    filename[0] = 0;
}

// +--------------------------------------------------------------------+

Galaxy::~Galaxy()
{
    UE_LOG(LogTempGalaxy, Log, TEXT("DESTROYING GALAXY %s"), ANSI_TO_TCHAR((const char*)name));
    systems.clear();
    stars.clear();
}

// +--------------------------------------------------------------------+

void Galaxy::InitializeFromEnvironment(UStarshatterEnvironmentSubsystem* Env)
{
    if (galaxy) {
        delete galaxy;
        galaxy = nullptr;
    }

    galaxy = new Galaxy("Galaxy");

    if (!galaxy)
    {
        UE_LOG(LogTempGalaxy, Error,
            TEXT("[Galaxy] InitializeFromEnvironment: failed to allocate galaxy"));
        return;
    }

    galaxy->LoadFromEnvironmentSubsystem(Env);
}

void
Galaxy::Close()
{
    delete galaxy;
    galaxy = nullptr;
}

Galaxy*
Galaxy::GetInstance()
{
    return galaxy;
}

// +--------------------------------------------------------------------+

void
Galaxy::ClearSystems()
{
    systems.clear();
    stars.clear();
    radius = 10;
}

// +--------------------------------------------------------------------+

void Galaxy::LoadFromEnvironmentSubsystem(UStarshatterEnvironmentSubsystem* Env)
{
    ClearSystems();

    if (!Env)
    {
        UE_LOG(LogTempGalaxy, Warning,
            TEXT("[Galaxy] LoadFromEnvironmentSubsystem: Env is null"));
        return;
    }

    radius = 10.0;

    const TArray<StarSystem*>& RuntimeSystems = Env->GetRuntimeStarSystems();

    for (StarSystem* StarSys : RuntimeSystems)
    {
        if (!StarSys)
        {
            continue;
        }

        // Reuse existing runtime StarSystem built by StarSystemRegistry
        systems.append(StarSys);

        const FString SystemName = ANSI_TO_TCHAR(StarSys->GetName());
        const FVector SystemLoc = StarSys->GetLocation();
        const int32 StarClass = StarSys->GetSequence();

        // Build lightweight galaxy/theater display star only
        Star* NewStar = new Star(
            TCHAR_TO_ANSI(*SystemName),
            SystemLoc,
            StarClass);

        if (NewStar)
        {
            stars.append(NewStar);
        }

        const double Dist = FVector(SystemLoc.X, SystemLoc.Y, SystemLoc.Z).Size();
        if (Dist > radius)
        {
            radius = Dist;
        }

        UE_LOG(LogTemp, Warning,
            TEXT("[Galaxy] Using existing runtime StarSystem: %s  BodiesRadius=%.0f Regions=%d"),
            *SystemName,
            StarSys->GetRadius(),
            StarSys->GetAllRegions().size());
    }

    UE_LOG(LogTempGalaxy, Log,
        TEXT("[Galaxy] loaded from EnvironmentSubsystem: systems=%d stars=%d radius=%.0f"),
        systems.size(),
        stars.size(),
        radius);
}

// +--------------------------------------------------------------------+

void
Galaxy::ExecFrame()
{
    ListIter<StarSystem> sys = systems;
    while (++sys) {
        sys->ExecFrame();
    }
}

// +--------------------------------------------------------------------+

StarSystem*
Galaxy::GetSystem(const char* in_name)
{
    ListIter<StarSystem> sys = systems;
    while (++sys) {
        if (!strcmp(sys->GetName(), in_name))
            return sys.value();
    }

    return nullptr;
}

// +--------------------------------------------------------------------+

StarSystem*
Galaxy::FindSystemByRegion(const char* rgn_name)
{
    ListIter<StarSystem> iter = systems;
    while (++iter) {
        StarSystem* sys = iter.value();
        if (sys && sys->FindRegion(rgn_name))
            return sys;
    }

    return nullptr;
}

