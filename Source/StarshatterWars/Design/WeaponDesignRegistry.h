/*  Project Starshatter Wars
    Fractal Dev Studios
    Copyright (c) 2025-2026. All Rights Reserved.

    ORIGINAL AUTHOR AND STUDIO: John DiCamillo / Destroyer Studios LLC

    SUBSYSTEM:    Stars.exe
    FILE:         WeaponDesignRegistry.h
    AUTHOR:       Carlos Bott

    OVERVIEW
    ========
    Registry for weapon designs.

    - Stores UE-side FWeaponDesign rows
    - Builds legacy WeaponDesign objects
    - Provides lookup for both UE and legacy layers

    NOTE:
    Weapon systems require legacy WeaponDesign objects.
*/

#pragma once

#include "CoreMinimal.h"
#include "GameStructs_System.h"

class WeaponDesign;

class WeaponDesignRegistry
{
public:
    // Clear all data and free legacy objects
    static void Clear();

    // Register a design (preferred)
    static void RegisterDesign(const FName& RowName, const FWeaponDesign& Row);

    // Convenience overload
    static void RegisterDesign(const FString& Name, const FWeaponDesign& Row);

    // UE-side lookup
    static const FWeaponDesign* Find(const FName& Name);
    static const FWeaponDesign* Find(const FString& Name);
    static const FWeaponDesign* Find(const char* Name);

    // Legacy lookup (used by Weapon systems)
    static WeaponDesign* FindLegacy(const FName& Name);
    static WeaponDesign* FindLegacy(const FString& Name);
    static WeaponDesign* FindLegacy(const char* Name);

    static const TMap<FName, FWeaponDesign>& GetAll();

private:
    static TMap<FName, FWeaponDesign> DesignsByName;
    static TMap<FName, WeaponDesign*> LegacyDesignsByName;

private:
    static WeaponDesign* ConvertToLegacyDesign(
        const FName& RowName,
        const FWeaponDesign& Row);

    static bool IsBadVector(const FVector& V);
};