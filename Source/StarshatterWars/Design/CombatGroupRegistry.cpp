/*=============================================================================
    Project:        Starshatter Wars
    Studio:         Fractal Dev Studios
    Copyright:      (C) 2024-2026. All Rights Reserved.

    SUBSYSTEM:      StarshatterWars (Unreal Engine)
    FILE:           CombatGroupRegistry.cpp
    AUTHOR:         Carlos Bott

    OVERVIEW
    ========
    Implementation of CombatGroupRegistry.

=============================================================================*/

#include "CombatGroupRegistry.h"

TMap<FName, FS_CombatGroup> CombatGroupRegistry::GroupsByName;

// -------------------------------------------------------------
// Lifecycle
// -------------------------------------------------------------

void CombatGroupRegistry::Clear()
{
    GroupsByName.Empty();
}

// -------------------------------------------------------------
// Registration
// -------------------------------------------------------------

void CombatGroupRegistry::RegisterGroup(const FName& RowName, const FS_CombatGroup& Row)
{
    if (!RowName.IsNone())
    {
        GroupsByName.Add(RowName, Row);
    }
}

void CombatGroupRegistry::RegisterGroup(const FString& Name, const FS_CombatGroup& Row)
{
    if (!Name.IsEmpty())
    {
        GroupsByName.Add(FName(*Name), Row);
    }
}

// -------------------------------------------------------------
// Lookup
// -------------------------------------------------------------

const FS_CombatGroup* CombatGroupRegistry::Find(const FName& Name)
{
    return Name.IsNone() ? nullptr : GroupsByName.Find(Name);
}

const FS_CombatGroup* CombatGroupRegistry::Find(const FString& Name)
{
    return Name.IsEmpty() ? nullptr : GroupsByName.Find(FName(*Name));
}

const FS_CombatGroup* CombatGroupRegistry::FindByTypeAndId(
    ECOMBATGROUP_TYPE Type,
    int32 Id)
{
    for (const TPair<FName, FS_CombatGroup>& Pair : GroupsByName)
    {
        const FS_CombatGroup& Group = Pair.Value;

        if (Group.Type == Type && Group.Id == Id)
        {
            return &Group;
        }
    }

    return nullptr;
}

// -------------------------------------------------------------
// Queries
// -------------------------------------------------------------

TArray<const FS_CombatGroup*> CombatGroupRegistry::FindByRegion(const FString& Region)
{
    TArray<const FS_CombatGroup*> Results;

    const FString Target = Region.TrimStartAndEnd().ToUpper();

    for (const TPair<FName, FS_CombatGroup>& Pair : GroupsByName)
    {
        const FS_CombatGroup& Group = Pair.Value;

        const FString GroupRegion = Group.Region.TrimStartAndEnd().ToUpper();

        if (GroupRegion == Target)
        {
            Results.Add(&Group);
        }
    }

    return Results;
}

TArray<const FS_CombatGroup*> CombatGroupRegistry::FindByParent(
    ECOMBATGROUP_TYPE ParentType,
    int32 ParentId)
{
    TArray<const FS_CombatGroup*> Results;

    for (const TPair<FName, FS_CombatGroup>& Pair : GroupsByName)
    {
        const FS_CombatGroup& Group = Pair.Value;

        if (Group.ParentType == ParentType && Group.ParentId == ParentId)
        {
            Results.Add(&Group);
        }
    }

    return Results;
}

// -------------------------------------------------------------
// Utility
// -------------------------------------------------------------

bool CombatGroupRegistry::Has(const FName& Name)
{
    return !Name.IsNone() && GroupsByName.Contains(Name);
}

bool CombatGroupRegistry::Has(const FString& Name)
{
    return !Name.IsEmpty() && GroupsByName.Contains(FName(*Name));
}

int32 CombatGroupRegistry::Num()
{
    return GroupsByName.Num();
}

const TMap<FName, FS_CombatGroup>& CombatGroupRegistry::GetAll()
{
    return GroupsByName;
}