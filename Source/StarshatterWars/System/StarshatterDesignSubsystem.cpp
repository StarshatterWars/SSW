/*  Project Starshatter Wars
    Fractal Dev Studios
    Copyright (C) 2025-2026.
    All Rights Reserved.

    FILE:         StarshatterDesignSubsystem.cpp
    AUTHOR:       Carlos Bott

    OVERVIEW
    ========
    Implementation for UStarshatterDesignSubsystem.

    Fixes:
    - No GetRowMap() pointer casting (avoids C2440)
    - Uses DataTable->GetRowNames() returning TArray<FName> (avoids C2660)
    - Iterates row names and uses FindRow<T>() for type safety
*/

#include "StarshatterDesignSubsystem.h"

DEFINE_LOG_CATEGORY(LogStarshatterDesign);

UStarshatterDesignSubsystem::UStarshatterDesignSubsystem()
{
}

void UStarshatterDesignSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
    Super::Initialize(Collection);
}

void UStarshatterDesignSubsystem::Deinitialize()
{
    WeaponNameIndex.Reset();
    SystemNameIndex.Reset();
    ShipNameIndex.Reset();

    WeaponTable = nullptr;
    SystemTable = nullptr;
    ShipTable = nullptr;

    Super::Deinitialize();
}

// ---------------------------------------------------------------------
// Table registration
// ---------------------------------------------------------------------
void UStarshatterDesignSubsystem::SetWeaponTable(UDataTable* InTable)
{
    WeaponTable = InTable;
    RebuildWeaponIndex();
}

void UStarshatterDesignSubsystem::SetSystemTable(UDataTable* InTable)
{
    SystemTable = InTable;
    RebuildSystemIndex();
}

void UStarshatterDesignSubsystem::SetShipTable(UDataTable* InTable)
{
    ShipTable = InTable;
    RebuildShipIndex();
}

// ---------------------------------------------------------------------
// Normalize key (for indexes)
// ---------------------------------------------------------------------
FString UStarshatterDesignSubsystem::NormalizeKey(const FString& In)
{
    FString S = In;
    S.TrimStartAndEndInline();
    S = S.ToUpper();
    return S;
}

// ---------------------------------------------------------------------
// Weapons
// ---------------------------------------------------------------------
const FWeaponDesign* UStarshatterDesignSubsystem::GetWeaponByRow(FName RowName) const
{
    if (!WeaponTable || RowName.IsNone())
        return nullptr;

    static const FString Context(TEXT("UStarshatterDesignSubsystem::GetWeaponByRow"));
    return WeaponTable->FindRow<FWeaponDesign>(RowName, Context, /*bWarnIfRowMissing*/ false);
}

bool UStarshatterDesignSubsystem::HasWeaponRow(FName RowName) const
{
    return GetWeaponByRow(RowName) != nullptr;
}

const FWeaponDesign* UStarshatterDesignSubsystem::FindWeaponByName(const FString& Name) const
{
    if (!WeaponTable)
        return nullptr;

    const FName* Row = WeaponNameIndex.Find(NormalizeKey(Name));
    if (Row)
        return GetWeaponByRow(*Row);

    // Fallback scan:
    const TArray<FName> Rows = WeaponTable->GetRowNames();
    for (const FName& R : Rows)
    {
        const FWeaponDesign* W = GetWeaponByRow(R);
        if (W && W->Name.Equals(Name, ESearchCase::IgnoreCase))
            return W;
    }

    return nullptr;
}

void UStarshatterDesignSubsystem::GetWeaponRowNames(TArray<FName>& OutRows) const
{
    OutRows.Reset();
    if (!WeaponTable)
        return;

    OutRows = WeaponTable->GetRowNames();
}

void UStarshatterDesignSubsystem::GetWeapons(TArray<const FWeaponDesign*>& OutWeapons) const
{
    OutWeapons.Reset();
    if (!WeaponTable)
        return;

    const TArray<FName> Rows = WeaponTable->GetRowNames();
    OutWeapons.Reserve(Rows.Num());

    for (const FName& R : Rows)
    {
        if (const FWeaponDesign* W = GetWeaponByRow(R))
        {
            OutWeapons.Add(W);
        }
    }
}

// ---------------------------------------------------------------------
// Systems
// ---------------------------------------------------------------------
const FSystemDesign* UStarshatterDesignSubsystem::GetSystemByRow(FName RowName) const
{
    if (!SystemTable || RowName.IsNone())
        return nullptr;

    static const FString Context(TEXT("UStarshatterDesignSubsystem::GetSystemByRow"));
    return SystemTable->FindRow<FSystemDesign>(RowName, Context, /*bWarnIfRowMissing*/ false);
}

bool UStarshatterDesignSubsystem::HasSystemRow(FName RowName) const
{
    return GetSystemByRow(RowName) != nullptr;
}

const FSystemDesign* UStarshatterDesignSubsystem::FindSystemByName(const FString& Name) const
{
    if (!SystemTable)
        return nullptr;

    const FName* Row = SystemNameIndex.Find(NormalizeKey(Name));
    if (Row)
        return GetSystemByRow(*Row);

    const TArray<FName> Rows = SystemTable->GetRowNames();
    for (const FName& R : Rows)
    {
        const FSystemDesign* S = GetSystemByRow(R);
        if (S && S->Name.Equals(Name, ESearchCase::IgnoreCase))
            return S;
    }

    return nullptr;
}

void UStarshatterDesignSubsystem::GetSystemRowNames(TArray<FName>& OutRows) const
{
    OutRows.Reset();
    if (!SystemTable)
        return;

    OutRows = SystemTable->GetRowNames();
}

void UStarshatterDesignSubsystem::GetSystems(TArray<const FSystemDesign*>& OutSystems) const
{
    OutSystems.Reset();
    if (!SystemTable)
        return;

    const TArray<FName> Rows = SystemTable->GetRowNames();
    OutSystems.Reserve(Rows.Num());

    for (const FName& R : Rows)
    {
        if (const FSystemDesign* S = GetSystemByRow(R))
        {
            OutSystems.Add(S);
        }
    }
}

// ---------------------------------------------------------------------
// Ships
// ---------------------------------------------------------------------
const FShipDesign* UStarshatterDesignSubsystem::GetShipByRow(FName RowName) const
{
    if (!ShipTable || RowName.IsNone())
        return nullptr;

    static const FString Context(TEXT("UStarshatterDesignSubsystem::GetShipByRow"));
    return ShipTable->FindRow<FShipDesign>(RowName, Context, /*bWarnIfRowMissing*/ false);
}

bool UStarshatterDesignSubsystem::HasShipRow(FName RowName) const
{
    return GetShipByRow(RowName) != nullptr;
}

const FShipDesign* UStarshatterDesignSubsystem::FindShipByName(const FString& Name) const
{
    if (!ShipTable)
        return nullptr;

    const FName* Row = ShipNameIndex.Find(NormalizeKey(Name));
    if (Row)
        return GetShipByRow(*Row);

    const TArray<FName> Rows = ShipTable->GetRowNames();
    for (const FName& R : Rows)
    {
        const FShipDesign* S = GetShipByRow(R);
        if (!S)
            continue;

        if (S->ShipName.Equals(Name, ESearchCase::IgnoreCase) ||
            S->DisplayName.Equals(Name, ESearchCase::IgnoreCase))
        {
            return S;
        }
    }

    return nullptr;
}

void UStarshatterDesignSubsystem::GetShipRowNames(TArray<FName>& OutRows) const
{
    OutRows.Reset();
    if (!ShipTable)
        return;

    OutRows = ShipTable->GetRowNames();
}

void UStarshatterDesignSubsystem::GetShips(TArray<const FShipDesign*>& OutShips) const
{
    OutShips.Reset();
    if (!ShipTable)
        return;

    const TArray<FName> Rows = ShipTable->GetRowNames();
    OutShips.Reserve(Rows.Num());

    for (const FName& R : Rows)
    {
        if (const FShipDesign* S = GetShipByRow(R))
        {
            OutShips.Add(S);
        }
    }
}

void UStarshatterDesignSubsystem::GetShipsByClass(const FString& ShipClass, TArray<const FShipDesign*>& OutShips) const
{
    OutShips.Reset();
    if (!ShipTable)
        return;

    const TArray<FName> Rows = ShipTable->GetRowNames();
    for (const FName& R : Rows)
    {
        const FShipDesign* S = GetShipByRow(R);
        if (S && S->ShipClass.Equals(ShipClass, ESearchCase::IgnoreCase))
        {
            OutShips.Add(S);
        }
    }
}

// ---------------------------------------------------------------------
// Index rebuilds
// ---------------------------------------------------------------------
void UStarshatterDesignSubsystem::RebuildAllIndexes()
{
    RebuildWeaponIndex();
    RebuildSystemIndex();
    RebuildShipIndex();
}

void UStarshatterDesignSubsystem::RebuildWeaponIndex()
{
    WeaponNameIndex.Reset();

    if (!WeaponTable)
        return;

    const TArray<FName> Rows = WeaponTable->GetRowNames();
    for (const FName& R : Rows)
    {
        const FWeaponDesign* W = GetWeaponByRow(R);
        if (!W)
            continue;

        const FString Key = NormalizeKey(W->Name);
        if (!Key.IsEmpty())
        {
            WeaponNameIndex.FindOrAdd(Key) = R;
        }
    }

    UE_LOG(LogStarshatterDesign, Log, TEXT("[DesignSS] Weapon index built: %d entries"), WeaponNameIndex.Num());
}

void UStarshatterDesignSubsystem::RebuildSystemIndex()
{
    SystemNameIndex.Reset();

    if (!SystemTable)
        return;

    const TArray<FName> Rows = SystemTable->GetRowNames();
    for (const FName& R : Rows)
    {
        const FSystemDesign* S = GetSystemByRow(R);
        if (!S)
            continue;

        const FString Key = NormalizeKey(S->Name);
        if (!Key.IsEmpty())
        {
            SystemNameIndex.FindOrAdd(Key) = R;
        }
    }

    UE_LOG(LogStarshatterDesign, Log, TEXT("[DesignSS] System index built: %d entries"), SystemNameIndex.Num());
}

void UStarshatterDesignSubsystem::RebuildShipIndex()
{
    ShipNameIndex.Reset();

    if (!ShipTable)
        return;

    const TArray<FName> Rows = ShipTable->GetRowNames();
    for (const FName& R : Rows)
    {
        const FShipDesign* S = GetShipByRow(R);
        if (!S)
            continue;

        FString Key = NormalizeKey(S->ShipName);
        if (Key.IsEmpty())
        {
            Key = NormalizeKey(S->DisplayName);
        }

        if (!Key.IsEmpty())
        {
            ShipNameIndex.FindOrAdd(Key) = R;
        }
    }

    UE_LOG(LogStarshatterDesign, Log, TEXT("[DesignSS] Ship index built: %d entries"), ShipNameIndex.Num());
}
