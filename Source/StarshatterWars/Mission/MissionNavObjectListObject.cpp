/*
    Project Starshatter Wars
    Fractal Dev Studios
    Copyright (C) 2025-2026. All Rights Reserved.

    SUBSYSTEM:    UI / Mission Briefing
    FILE:         MissionNavObjectListObject.cpp
    AUTHOR:       Carlos Bott

    OVERVIEW
    ========
    Implementation of UMissionNavObjectListObject.
*/

#include "MissionNavObjectListObject.h"

UMissionNavObjectListObject::UMissionNavObjectListObject()
{
}

void UMissionNavObjectListObject::InitObjectRow(
    EMissionNavObjectType InType,
    int32 InIndex,
    const FString& InPrimaryText,
    const FString& InSecondaryText,
    const FString& InDetailText)
{
    ObjectType = InType;
    Index = InIndex;
    PrimaryText = InPrimaryText;
    SecondaryText = InSecondaryText;
    DetailText = InDetailText;
}