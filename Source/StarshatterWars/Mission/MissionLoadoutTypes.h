/*  Project Starshatter Wars
    Fractal Dev Studios
    Copyright (C) 2025-2026. All Rights Reserved.

    SUBSYSTEM:    Stars.exe
    FILE:         MissionLoadoutTypes.h
    AUTHOR:       Carlos Bott

    OVERVIEW
    ========
    Unreal-facing mission loadout data types.

    These structures represent runtime loadout data derived
    from legacy MissionLoad objects and adapted for use in
    Unreal Engine systems such as UMG, ListView, and Blueprint.

    ARCHITECTURE ROLE
    =================
    FShipDesign        -> static definition (hardpoints, allowed weapons)
    FShipLoadout       -> preset templates
    MissionLoad        -> legacy runtime container (mutable)
    FMissionRuntimeLoadout -> Unreal-facing runtime copy (THIS FILE)

    NOTES
    =====
    - Safe for UI and Blueprint exposure
    - Does not modify or replace MissionLoad
    - Acts as a conversion layer between legacy and UE systems
*/

#pragma once

#include "CoreMinimal.h"
#include "MissionLoadoutTypes.generated.h"

// +--------------------------------------------------------------------+

USTRUCT(BlueprintType)
struct FMissionRuntimeLoadout
{
    GENERATED_BODY()

    // Ship index this loadout applies to
    UPROPERTY(BlueprintReadOnly)
    int32 ShipIndex = INDEX_NONE;

    // Optional loadout name
    UPROPERTY(BlueprintReadOnly)
    FString Name;

    // StationIndex -> WeaponID mapping
    UPROPERTY(BlueprintReadOnly)
    TArray<int32> Stations;
};

// +--------------------------------------------------------------------+

USTRUCT(BlueprintType)
struct FMissionWeaponStationRow
{
    GENERATED_BODY()

    // Index of the station / hardpoint
    UPROPERTY(BlueprintReadOnly)
    int32 StationIndex = INDEX_NONE;

    // Display name of the hardpoint
    UPROPERTY(BlueprintReadOnly)
    FString StationName;

    // Optional type/category of hardpoint
    UPROPERTY(BlueprintReadOnly)
    FString StationType;

    // Weapon ID assigned to this station
    UPROPERTY(BlueprintReadOnly)
    int32 WeaponId = INDEX_NONE;

    // Resolved display name of the weapon
    UPROPERTY(BlueprintReadOnly)
    FString WeaponName;
};
