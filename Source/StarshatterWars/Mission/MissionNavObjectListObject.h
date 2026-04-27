/*
    Project Starshatter Wars
    Fractal Dev Studios
    Copyright (C) 2025-2026. All Rights Reserved.

    ORIGINAL AUTHOR AND STUDIO
    =========================
    John DiCamillo / Destroyer Studios LLC

    SUBSYSTEM:    UI / Mission Briefing
    FILE:         MissionNavObjectListObject.h
    AUTHOR:       Carlos Bott

    OVERVIEW
    ========
    UObject row-model for the right-side Mission Navigation object list.

    This mirrors the MissionNavListObject pattern, but is intended for
    object-selection rows such as System, Planet, Sector, Station,
    Starship, and Fighter entries.
*/

#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "MissionNavObjectListObject.generated.h"

class MissionElement;

UENUM(BlueprintType)
enum class EMissionNavObjectType : uint8
{
    None = 0,
    System,
    Planet,
    Sector,
    Station,
    Starship,
    Fighter
};

UCLASS()
class STARSHATTERWARS_API UMissionNavObjectListObject : public UObject
{
    GENERATED_BODY()

public:
    UMissionNavObjectListObject();

    void InitObjectRow(
        EMissionNavObjectType InType,
        int32 InIndex,
        const FString& InPrimaryText,
        const FString& InSecondaryText,
        const FString& InDetailText);

    const FString& GetPrimaryText() const { return PrimaryText; }
    const FString& GetSecondaryText() const { return SecondaryText; }
    const FString& GetDetailText() const { return DetailText; }

    EMissionNavObjectType GetObjectType() const { return ObjectType; }
    int32 GetIndex() const { return Index; }

    void SetSourceMissionElement(MissionElement* InElement) { SourceMissionElement = InElement; }
    MissionElement* GetSourceMissionElement() const { return SourceMissionElement; }

private:
    UPROPERTY()
    EMissionNavObjectType ObjectType = EMissionNavObjectType::None;

    UPROPERTY()
    int32 Index = INDEX_NONE;

    UPROPERTY()
    FString PrimaryText;

    UPROPERTY()
    FString SecondaryText;

    UPROPERTY()
    FString DetailText;

    MissionElement* SourceMissionElement = nullptr;

};
