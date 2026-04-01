/*  Project Starshatter Wars
    Fractal Dev Studios
    Copyright (C) 2025-2026. All Rights Reserved.

    SUBSYSTEM:    Stars.exe
    FILE:         MissionWeaponStationRowObject.h
    AUTHOR:       Carlos Bott

    OVERVIEW
    ========
    UI row object for displaying the currently selected runtime
    weapon per hardpoint station.
*/

#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "MissionWeaponStationRowObject.generated.h"

UCLASS(BlueprintType)
class STARSHATTERWARS_API UMissionWeaponStationRowObject : public UObject
{
    GENERATED_BODY()

public:
    void Init(
        int32 InStationIndex,
        const FString& InStationLabel,
        const FString& InWeaponName)
    {
        StationIndex = InStationIndex;
        StationLabel = InStationLabel;
        WeaponName = InWeaponName;
    }

    int32 GetStationIndex() const { return StationIndex; }
    const FString& GetStationLabel() const { return StationLabel; }
    const FString& GetWeaponName() const { return WeaponName; }

private:
    UPROPERTY(BlueprintReadOnly, meta = (AllowPrivateAccess = "true"))
    int32 StationIndex = INDEX_NONE;

    UPROPERTY(BlueprintReadOnly, meta = (AllowPrivateAccess = "true"))
    FString StationLabel;

    UPROPERTY(BlueprintReadOnly, meta = (AllowPrivateAccess = "true"))
    FString WeaponName;
};
