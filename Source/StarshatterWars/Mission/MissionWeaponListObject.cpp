/*
    Project Starshatter Wars
    Fractal Dev Studios
    Copyright (C) 2025-2026. All Rights Reserved.
*/

#include "MissionWeaponListObject.h"

UMissionWeaponListObject::UMissionWeaponListObject()
{
}

void UMissionWeaponListObject::InitRow(
    MissionElement* InElement,
    int32 InRowIndex,
    int32 InStationIndex,
    const FString& InStationText,
    const FString& InAllowedWeaponsText,
    const FString& InSelectedWeaponText,
    const FString& InAmmoText)
{
    ElementPtr = InElement;
    RowIndex = InRowIndex;
    StationIndex = InStationIndex;

    StationText = InStationText;
    AllowedWeaponsText = InAllowedWeaponsText;
    SelectedWeaponText = InSelectedWeaponText;
    AmmoText = InAmmoText;
}