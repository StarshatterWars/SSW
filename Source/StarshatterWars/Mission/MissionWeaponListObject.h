/*
    Project Starshatter Wars
    Fractal Dev Studios
    Copyright (C) 2025-2026. All Rights Reserved.

    ORIGINAL AUTHOR AND STUDIO
    =========================
    John DiCamillo / Destroyer Studios LLC

    SUBSYSTEM:    UI / Mission Briefing
    FILE:         MissionWeaponListObject.h
    AUTHOR:       Carlos Bott

    OVERVIEW
    ========
    UObject row-model for mission weapon/loadout entries.
*/

#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "MissionWeaponListObject.generated.h"

class MissionElement;

UCLASS()
class STARSHATTERWARS_API UMissionWeaponListObject : public UObject
{
    GENERATED_BODY()

public:
    UMissionWeaponListObject();

    void InitRow(
        MissionElement* InElement,
        int32 InRowIndex,
        int32 InStationIndex,
        const FString& InStationText,
        const FString& InAllowedWeaponsText,
        const FString& InSelectedWeaponText,
        const FString& InAmmoText);

    MissionElement* GetElement() const { return ElementPtr; }

    int32 GetRowIndex() const { return RowIndex; }
    int32 GetStationIndex() const { return StationIndex; }

    const FString& GetStationText() const { return StationText; }
    const FString& GetAllowedWeaponsText() const { return AllowedWeaponsText; }
    const FString& GetSelectedWeaponText() const { return SelectedWeaponText; }
    const FString& GetAmmoText() const { return AmmoText; }

private:
    MissionElement* ElementPtr = nullptr;

    int32 RowIndex = INDEX_NONE;
    int32 StationIndex = INDEX_NONE;

    FString StationText;
    FString AllowedWeaponsText;
    FString SelectedWeaponText;
    FString AmmoText;
};