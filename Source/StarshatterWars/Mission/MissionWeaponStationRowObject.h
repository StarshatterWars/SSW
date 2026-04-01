/*  Project Starshatter Wars
    Fractal Dev Studios

    SUBSYSTEM:    Stars.exe
    FILE:         MissionWeaponStationRowObject.h

    OVERVIEW
    ========
    UI row object for displaying and editing the currently selected runtime
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
        const FString& InWeaponName,
        const TArray<FString>& InAllowedWeapons,
        int32 InCurrentSelection)
    {
        StationIndex = InStationIndex;
        StationLabel = InStationLabel;
        WeaponName = InWeaponName;
        AllowedWeapons = InAllowedWeapons;
        CurrentSelection = InCurrentSelection;
    }

    int32 GetStationIndex() const { return StationIndex; }
    const FString& GetStationLabel() const { return StationLabel; }
    const FString& GetWeaponName() const { return WeaponName; }
    const TArray<FString>& GetAllowedWeapons() const { return AllowedWeapons; }
    int32 GetCurrentSelection() const { return CurrentSelection; }

    void SetWeaponName(const FString& InWeaponName) { WeaponName = InWeaponName; }
    void SetCurrentSelection(int32 InCurrentSelection) { CurrentSelection = InCurrentSelection; }

private:
    UPROPERTY(BlueprintReadOnly, meta = (AllowPrivateAccess = "true"))
    int32 StationIndex = INDEX_NONE;

    UPROPERTY(BlueprintReadOnly, meta = (AllowPrivateAccess = "true"))
    FString StationLabel;

    UPROPERTY(BlueprintReadOnly, meta = (AllowPrivateAccess = "true"))
    FString WeaponName;

    UPROPERTY(BlueprintReadOnly, meta = (AllowPrivateAccess = "true"))
    TArray<FString> AllowedWeapons;

    UPROPERTY(BlueprintReadOnly, meta = (AllowPrivateAccess = "true"))
    int32 CurrentSelection = INDEX_NONE;
};