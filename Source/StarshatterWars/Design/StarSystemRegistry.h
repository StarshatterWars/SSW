/*=============================================================================
    Project:        Starshatter Wars
    Studio:         Fractal Dev Studios
    Copyright:      (C) 2024-2026. All Rights Reserved.

    SUBSYSTEM:      StarshatterWars (Unreal Engine)
    FILE:           StarSystemRegistry.h
    AUTHOR:         Carlos Bott

    OVERVIEW
    ========
    Global runtime registry and builder for StarSystem objects.

    Converts subsystem-hydrated FS_* data into runtime StarSystem
    instances and provides fast lookup access.

=============================================================================*/

#pragma once

#include "CoreMinimal.h"
#include "GameStructs.h"

class StarSystem;
class Orbital;
class OrbitalBody;
class OrbitalRegion;
class UStarshatterEnvironmentSubsystem;

class STARSHATTERWARS_API StarSystemRegistry
{
public:

    // -------------------------------------------------------------
    // Lifecycle
    // -------------------------------------------------------------
    static void Clear(bool bDeleteSystems = true);

    // -------------------------------------------------------------
    // Registration
    // -------------------------------------------------------------
    static void RegisterSystem(const FName& RowName, StarSystem* System);
    static void RegisterSystem(const FString& Name, StarSystem* System);

    // -------------------------------------------------------------
    // Lookup
    // -------------------------------------------------------------
    static StarSystem* Find(const FName& Name);
    static StarSystem* Find(const FString& Name);
    static StarSystem* Find(const char* Name);

    // -------------------------------------------------------------
    // Build entry points
    // -------------------------------------------------------------
    static StarSystem* BuildAndRegister(
        const FS_Galaxy& Row,
        UStarshatterEnvironmentSubsystem* Env);

    // -------------------------------------------------------------
    // Utility
    // -------------------------------------------------------------
    static bool Has(const FName& Name);
    static bool Has(const FString& Name);
    static bool Has(const char* Name);

    static int32 Num();

    static const TMap<FName, StarSystem*>& GetAll();
    static const TArray<StarSystem*>& GetOwnedSystems();

private:

    // -------------------------------------------------------------
    // Internal build helpers
    // -------------------------------------------------------------
    static int32 ToLegacyStarClass(ESPECTRAL_CLASS InClass);

    static StarSystem* BuildStarSystem(
        const FS_Galaxy& Row,
        UStarshatterEnvironmentSubsystem* Env);

    static OrbitalBody* BuildStar(
        StarSystem* System,
        const FStarSystem& Row,
        UStarshatterEnvironmentSubsystem* Env);

    static OrbitalBody* BuildPlanet(
        StarSystem* System,
        OrbitalBody* ParentStar,
        const FPlanet& Row,
        UStarshatterEnvironmentSubsystem* Env);

    static OrbitalBody* BuildMoon(
        StarSystem* System,
        OrbitalBody* ParentPlanet,
        const FMoon& Row,
        UStarshatterEnvironmentSubsystem* Env);

    static OrbitalRegion* BuildRegion(
        StarSystem* System,
        Orbital* Parent,
        const FRegion& Row,
        UStarshatterEnvironmentSubsystem* Env);

private:
    static TMap<FName, StarSystem*> SystemsByName;
    static TArray<StarSystem*> OwnedSystems;
};