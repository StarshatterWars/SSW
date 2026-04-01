/*  Project Starshatter Wars
    Fractal Dev Studios
    Copyright (C) 2025-2026. All Rights Reserved.

    SUBSYSTEM:    Stars.exe
    FILE:         MissionWeaponLoadoutListObject.cpp
    AUTHOR:       Carlos Bott

    OVERVIEW
    ========
    Implementation of UObject wrapper for ship loadout presets.

    This class adapts FShipLoadout data into a UI-friendly format
    for use in UListView and entry widgets.

    NOTES
    =====
    - Stores LoadoutIndex for resolving back to FShipDesign::Loadout
    - No gameplay logic
*/

#include "MissionWeaponLoadoutListObject.h"
#include "GameStructs_System.h"

// +--------------------------------------------------------------------+

void UMissionWeaponLoadoutListObject::InitFromShipLoadout(
    const FShipLoadout& InLoadout,
    int32 InLoadoutIndex,
    const FString& InWeightText,
    bool bInSelected)
{
    LoadoutIndex = InLoadoutIndex;
    LoadoutName = InLoadout.Name;
    WeightText = InWeightText;
    bSelected = bInSelected;
}