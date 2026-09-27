/*
    Project Starshatter Wars
    Fractal Dev Studios
    Copyright (C) 2025–2026. All Rights Reserved.

    ORIGINAL AUTHOR AND STUDIO
    =========================
    John DiCamillo / Destroyer Studios LLC

    SUBSYSTEM:    Stars.exe
    FILE:         MissionListItem.cpp
    AUTHOR:       Carlos Bott

    OVERVIEW
    ========
    Implementation of UMissionListItem.

    Converts legacy Campaign::MissionInfo data into cached FString
    fields suitable for Unreal UI widgets (UListView).

    This preserves original Starshatter behavior while eliminating
    lifetime hazards caused by direct List<MissionInfo> access.
*/

#include "MissionListItem.h"

#include "Campaign.h"   // Campaign, MissionInfo
#include "Text.h"       // Starshatter Text wrapper

// +--------------------------------------------------------------------+

UMissionListItem::UMissionListItem()
{
}

// +--------------------------------------------------------------------+

void UMissionListItem::InitFromMissionInfo(
    Campaign* InCampaign,
    MissionInfo* InInfo,
    int32 InIndex)
{
    CampaignPtr = InCampaign;
    InfoPtr = InInfo;
    Index = InIndex;

    DisplayName.Empty();
    Description.Empty();
    Region.Empty();
    System.Empty();

    MissionId = 0;
    MissionType = 0;
    StartTime = 0;

    if (!InfoPtr)
        return;

    // Cache legacy fields:
    MissionId = InfoPtr->id;
    MissionType = InfoPtr->type;
    StartTime = InfoPtr->start;

    // Starshatter Text -> FString:
    DisplayName = FString(InfoPtr->name.data());
    Description = FString(InfoPtr->description.data());
    Region = FString(InfoPtr->region.data());
    System = FString(InfoPtr->system.data());
}
