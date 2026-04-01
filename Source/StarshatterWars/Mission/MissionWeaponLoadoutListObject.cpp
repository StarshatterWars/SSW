/*
    Project Starshatter Wars
    Fractal Dev Studios
    Copyright (C) 2025-2026. All Rights Reserved.
*/

#include "MissionWeaponLoadoutListObject.h"
#include "GameStructs_System.h"

UMissionWeaponLoadoutListObject::UMissionWeaponLoadoutListObject()
{
}

void UMissionWeaponLoadoutListObject::InitFromShipLoadout(
    const FShipLoadout& InLoadout,
    int32 InIndex,
    const FString& InWeightText,
    bool bInSelected)
{
    LoadoutName = InLoadout.Name;
    WeightText = InWeightText;
    Index = InIndex;
    bSelected = bInSelected;
}