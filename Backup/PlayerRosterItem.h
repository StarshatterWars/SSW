/*=============================================================================
    Project:        Starshatter Wars (nGenEx Unreal Port)
    Studio:         Fractal Dev Games
    Copyright:      (C) 2025–2026. All Rights Reserved.

    SUBSYSTEM:      UI / Player
    FILE:           PlayerRosterItem.h
    AUTHOR:         Carlos Bott

    OVERVIEW
    ========
    UPlayerRosterItem

    Lightweight UObject wrapper used by UListView.

    - Holds a copy of FS_PlayerGameInfo
    - Provides read-only access to row widgets
=============================================================================*/

#pragma once

#include "CoreMinimal.h"
#include "GameStructs.h"              // FS_PlayerGameInfo
#include "PlayerRosterItem.generated.h"

// +--------------------------------------------------------------------+

UCLASS()
class STARSHATTERWARS_API UPlayerRosterItem : public UObject
{
    GENERATED_BODY()

public:

    // ------------------------------------------------------------
    // Initialization
    // ------------------------------------------------------------

    void Initialize(const FS_PlayerGameInfo& InInfo);

    // ------------------------------------------------------------
    // Accessors
    // ------------------------------------------------------------

    const FS_PlayerGameInfo& GetInfo() const;

private:

    // Stored player data snapshot
    UPROPERTY()
    FS_PlayerGameInfo PlayerInfo;
};
