/*
    Project Starshatter Wars
    Fractal Dev Studios
    Copyright (C) 2025-2026. All Rights Reserved.

    ORIGINAL AUTHOR AND STUDIO
    =========================
    John DiCamillo / Destroyer Studios LLC

    SUBSYSTEM:    UI / Mission Briefing
    FILE:         MissionWeaponLoadoutListObject.h
    AUTHOR:       Carlos Bott

    OVERVIEW
    ========
    UObject row-model for standard mission weapon loadouts.

    This class wraps one FShipLoadout entry from FShipDesign into
    a UObject suitable for UListView.
*/

#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "MissionWeaponLoadoutListObject.generated.h"

struct FShipLoadout;

UCLASS()
class STARSHATTERWARS_API UMissionWeaponLoadoutListObject : public UObject
{
    GENERATED_BODY()

public:
    UMissionWeaponLoadoutListObject();

    void InitFromShipLoadout(
        const FShipLoadout& InLoadout,
        int32 InIndex,
        const FString& InWeightText,
        bool bInSelected);

    const FString& GetLoadoutName() const { return LoadoutName; }
    const FString& GetWeightText() const { return WeightText; }

    int32 GetIndex() const { return Index; }
    bool IsSelected() const { return bSelected; }

    void SetSelected(bool bInSelected) { bSelected = bInSelected; }

private:
    FString LoadoutName;
    FString WeightText;

    int32 Index = INDEX_NONE;
    bool bSelected = false;
};