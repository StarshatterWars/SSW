/*=============================================================================
    Project:        Starshatter Wars
    Studio:         Fractal Dev Studios
    Copyright:      (C) 2024-2026. All Rights Reserved.

    SUBSYSTEM:      StarshatterWars (Unreal Engine)
    FILE:           CombatGroupRegistry.h
    AUTHOR:         Carlos Bott

    OVERVIEW
    ========
    Pure C++ global registry for FS_CombatGroup rows.

    - No UObject dependency
    - Populated by UStarshatterGameDataSubsystem
    - Provides fast lookup and region-based queries
    - Authoritative source for mission nav / sector objects

=============================================================================*/

#pragma once

#include "CoreMinimal.h"
#include "GameStructs.h"

class STARSHATTERWARS_API CombatGroupRegistry
{
public:

    // -------------------------------------------------------------
    // Lifecycle
    // -------------------------------------------------------------
    static void Clear();

    // -------------------------------------------------------------
    // Registration (called by subsystem)
    // -------------------------------------------------------------
    static void RegisterGroup(const FName& RowName, const FS_CombatGroup& Row);
    static void RegisterGroup(const FString& Name, const FS_CombatGroup& Row);

    // -------------------------------------------------------------
    // Lookup
    // -------------------------------------------------------------
    static const FS_CombatGroup* Find(const FName& Name);
    static const FS_CombatGroup* Find(const FString& Name);

    static const FS_CombatGroup* FindByTypeAndId(
        ECOMBATGROUP_TYPE Type,
        int32 Id);

    // -------------------------------------------------------------
    // Queries
    // -------------------------------------------------------------
    static TArray<const FS_CombatGroup*> FindByRegion(const FString& Region);

    static TArray<const FS_CombatGroup*> FindByParent(
        ECOMBATGROUP_TYPE ParentType,
        int32 ParentId);

    // -------------------------------------------------------------
    // Utility
    // -------------------------------------------------------------
    static bool Has(const FName& Name);
    static bool Has(const FString& Name);

    static int32 Num();

    static const TMap<FName, FS_CombatGroup>& GetAll();

private:
    static TMap<FName, FS_CombatGroup> GroupsByName;
};
