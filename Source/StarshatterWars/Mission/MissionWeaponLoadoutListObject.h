/*  Project Starshatter Wars
    Fractal Dev Studios
    Copyright (C) 2025-2026. All Rights Reserved.

    SUBSYSTEM:    Stars.exe
    FILE:         MissionWeaponLoadoutListObject.h
    AUTHOR:       Carlos Bott

    OVERVIEW
    ========
    UObject wrapper for ship loadout presets used by the
    Mission Weapon dialog ListView.

    Each instance represents one FShipLoadout entry and
    exposes display data for UI rendering and selection.

    ARCHITECTURE ROLE
    =================
    FShipDesign::Loadout
        ->
    UMissionWeaponLoadoutListObject
        ->
    UListView / EntryWidget

    NOTES
    =====
    - Stores LoadoutIndex for resolving back to FShipDesign::Loadout
    - Used for selection -> MissionLoad population
    - Pure UI adapter
*/

#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "MissionWeaponLoadoutListObject.generated.h"

struct FShipLoadout;

// +--------------------------------------------------------------------+

UCLASS(BlueprintType)
class STARSHATTERWARS_API UMissionWeaponLoadoutListObject : public UObject
{
    GENERATED_BODY()

public:
    void InitFromShipLoadout(
        const FShipLoadout& InLoadout,
        int32 InLoadoutIndex,
        const FString& InWeightText,
        bool bInSelected);

    // ------------------------------------------------------------
    // Accessors
    // ------------------------------------------------------------

    int32 GetLoadoutIndex() const { return LoadoutIndex; }

    const FString& GetLoadoutName() const { return LoadoutName; }
    const FString& GetWeightText() const { return WeightText; }

    bool IsSelected() const { return bSelected; }
    void SetSelected(bool bInSelected) { bSelected = bInSelected; }

public:
    UPROPERTY(BlueprintReadOnly)
    FString LoadoutName;

    UPROPERTY(BlueprintReadOnly)
    FString WeightText;

    UPROPERTY(BlueprintReadOnly)
    bool bSelected = false;

private:
    UPROPERTY()
    int32 LoadoutIndex = INDEX_NONE;
};