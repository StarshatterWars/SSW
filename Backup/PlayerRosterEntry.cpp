/*=============================================================================
    Project:        Starshatter Wars (nGenEx Unreal Port)
    Studio:         Fractal Dev Games
    Copyright:      (C) 2025–2026. All Rights Reserved.

    SUBSYSTEM:      UI / Player
    FILE:           PlayerRosterEntry.cpp
    AUTHOR:         Carlos Bott

    IMPLEMENTATION
    ==============
    UPlayerRosterEntry row widget implementation.
=============================================================================*/

#include "PlayerRosterEntry.h"
#include "Components/TextBlock.h"
#include "PlayerRosterItem.h"

// ------------------------------------------------------------
// ListView binding
// ------------------------------------------------------------

void UPlayerRosterEntry::NativeOnListItemObjectSet(UObject* ListItemObject)
{
    const UPlayerRosterItem* Item = Cast<UPlayerRosterItem>(ListItemObject);

    if (!Item)
    {
        RefreshFromPlayerInfo(FS_PlayerGameInfo{});
        return;
    }

    RefreshFromPlayerInfo(Item->GetInfo());
}

// ------------------------------------------------------------
// UI refresh
// ------------------------------------------------------------

void UPlayerRosterEntry::RefreshFromPlayerInfo(const FS_PlayerGameInfo& Info)
{
    // ------------------------------------------------------------
    // Name
    // ------------------------------------------------------------

    if (txt_name)
    {
        txt_name->SetText(FText::FromString(Info.Name));
    }

    // ------------------------------------------------------------
    // Rank
    // ------------------------------------------------------------

    if (txt_rank)
    {
        txt_rank->SetText(FText::AsNumber(Info.Rank));
    }

    // ------------------------------------------------------------
    // Stats (Missions / Kills / Deaths)
    // ------------------------------------------------------------

    if (txt_stats)
    {
        // Using FString::Printf is now safe since fields exist.
        const FString StatsString = FString::Printf(
            TEXT("M %d  K %d  D %d"),
            Info.Missions,
            Info.Kills,
            Info.Deaths
        );

        txt_stats->SetText(FText::FromString(StatsString));
    }
}
