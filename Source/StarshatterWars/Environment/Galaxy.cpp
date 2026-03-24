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

#include "StarshatterEnvironmentSubsystem.h"
#include "GameStructs.h"
#include "GameStructs_System.h"

#include "Math/Vector.h"
#include "Logging/LogMacros.h"

DEFINE_LOG_CATEGORY_STATIC(LogStarshatterWarsGalaxy, Log, All);

static Galaxy* galaxy = nullptr;

// +--------------------------------------------------------------------+

Galaxy::Galaxy(const char* n)
    : name(n), radius(10)
{
    filename[0] = 0;
}

// +--------------------------------------------------------------------+

Galaxy::~Galaxy()
{
    UE_LOG(LogStarshatterWarsGalaxy, Log, TEXT("DESTROYING GALAXY %s"), ANSI_TO_TCHAR((const char*)name));
    systems.destroy();
    stars.destroy();
}

// +--------------------------------------------------------------------+

void
Galaxy::InitializeFromEnvironment(UStarshatterEnvironmentSubsystem* Env)
{
    if (galaxy) {
        delete galaxy;
        galaxy = nullptr;
    }

    galaxy = new Galaxy("Galaxy");
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
    systems.destroy();
    stars.destroy();
    radius = 10;
}

// +--------------------------------------------------------------------+

void Galaxy::LoadFromEnvironmentSubsystem(UStarshatterEnvironmentSubsystem* Env)
{
    ClearSystems();

    if (!Env)
    {
        UE_LOG(LogStarshatterWarsGalaxy, Warning,
            TEXT("Galaxy::LoadFromEnvironmentSubsystem: Env is null"));
        return;
    }

    // DO NOT call Env->LoadAll() here.
    // That creates recursion because LoadAll() calls Galaxy::InitializeFromEnvironment().

    radius = 10;

    for (const FS_Galaxy& GalaxyRow : Env->GalaxyDataArray)
    {
        if (GalaxyRow.Name.IsEmpty())
        {
            continue;
        }

        const FString SystemName = GalaxyRow.Name;
        const FVector SystemLoc = GalaxyRow.Location;
        const int32 SystemIFF = GalaxyRow.Iff;

        int StarClass = Star::G;

        switch (GalaxyRow.Class)
        {
        case ESPECTRAL_CLASS::O:           StarClass = Star::O;           break;
        case ESPECTRAL_CLASS::B:           StarClass = Star::B;           break;
        case ESPECTRAL_CLASS::A:           StarClass = Star::A;           break;
        case ESPECTRAL_CLASS::F:           StarClass = Star::F;           break;
        case ESPECTRAL_CLASS::G:           StarClass = Star::G;           break;
        case ESPECTRAL_CLASS::K:           StarClass = Star::K;           break;
        case ESPECTRAL_CLASS::M:           StarClass = Star::M;           break;
        case ESPECTRAL_CLASS::RED_GIANT:   StarClass = Star::RED_GIANT;   break;
        case ESPECTRAL_CLASS::WHITE_DWARF: StarClass = Star::WHITE_DWARF; break;
        case ESPECTRAL_CLASS::BLACK_HOLE:  StarClass = Star::BLACK_HOLE;  break;
        default:                           StarClass = Star::G;           break;
        }

        StarSystem* StarSys = new StarSystem(
            TCHAR_TO_ANSI(*SystemName),
            SystemLoc,
            SystemIFF,
            StarClass);

        if (StarSys)
        {
            // Optional: only keep this if StarSystem::Load() is still valid in DT path
            // StarSys->Load();
            systems.append(StarSys);
        }

        Star* NewStar = new Star(
            TCHAR_TO_ANSI(*SystemName),
            SystemLoc,
            StarClass);

        if (NewStar)
        {
            stars.append(NewStar);
        }
    }

    UE_LOG(LogStarshatterWarsGalaxy, Log,
        TEXT("Galaxy loaded from EnvironmentSubsystem: systems=%d stars=%d"),
        systems.size(),
        stars.size());
}

// +--------------------------------------------------------------------+

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
        if (!strcmp(sys->Name(), in_name))
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