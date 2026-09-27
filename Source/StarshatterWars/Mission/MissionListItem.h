/*
    Project Starshatter Wars
    Fractal Dev Studios
    Copyright (C) 2025–2026. All Rights Reserved.

    ORIGINAL AUTHOR AND STUDIO
    =========================
    John DiCamillo / Destroyer Studios LLC

    SUBSYSTEM:    Stars.exe
    FILE:         MissionListItem.h
    AUTHOR:       Carlos Bott

    OVERVIEW
    ========
    UObject row-model for mission selection lists.

    This class replaces the legacy ListBox row pattern used in
    MsnSelectDlg and related dialogs. It wraps Campaign::MissionInfo
    into a UObject suitable for UListView while preserving all
    Starshatter core data and semantics.

    This is a lightweight data container:
    - No ownership of Campaign or MissionInfo
    - Cached strings for safe UI lifetime
*/

#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "MissionListItem.generated.h"

// Forward declarations (keep header light):
class Campaign;
class MissionInfo;

/**
 * Mission list row item for UListView.
 * Mirrors legacy ListBox entries backed by MissionInfo.
 */
UCLASS()
class STARSHATTERWARS_API UMissionListItem : public UObject
{
    GENERATED_BODY()

public:
    UMissionListItem();

    // Initialize from legacy mission info:
    void InitFromMissionInfo(Campaign* InCampaign, MissionInfo* InInfo, int32 InIndex);

    // Accessors:
    const FString& GetDisplayName() const { return DisplayName; }
    const FString& GetDescription() const { return Description; }
    const FString& GetRegion() const { return Region; }
    const FString& GetSystem() const { return System; }

    int32 GetIndex() const { return Index; }
    int32 GetMissionId() const { return MissionId; }
    int32 GetMissionType() const { return MissionType; }
    int32 GetStartTime() const { return StartTime; }

    Campaign* GetCampaign() const { return CampaignPtr; }

private:
    // Raw pointers (non-owning):
    Campaign* CampaignPtr = nullptr;
    MissionInfo* InfoPtr = nullptr;

    // Cached UI-safe values:
    FString DisplayName;
    FString Description;
    FString Region;
    FString System;

    int32 Index = -1;
    int32 MissionId = 0;
    int32 MissionType = 0;
    int32 StartTime = 0;
};

