/*=============================================================================
    Project:        Starshatter Wars (nGenEx Unreal Port)
    Studio:         Fractal Dev Games
    Copyright:      (C) 2025–2026. All Rights Reserved.

    SUBSYSTEM:      UI / Player
    FILE:           PlayerRosterEntry.h
    AUTHOR:         Carlos Bott

    OVERVIEW
    ========
    UPlayerRosterEntry

    Row widget used by UListView to display a player profile.

    Displays:
        - Name
        - Rank
        - Missions / Kills / Deaths
=============================================================================*/

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/IUserObjectListEntry.h"
#include "Blueprint/UserWidget.h"
#include "GameStructs.h"
#include "PlayerRosterEntry.generated.h"

class UTextBlock;

UCLASS()
class STARSHATTERWARS_API UPlayerRosterEntry
    : public UUserWidget
    , public IUserObjectListEntry
{
    GENERATED_BODY()

public:

    // ------------------------------------------------------------
    // ListView binding
    // ------------------------------------------------------------

    virtual void NativeOnListItemObjectSet(UObject* ListItemObject) override;

private:

    // ------------------------------------------------------------
    // Internal helpers
    // ------------------------------------------------------------

    void RefreshFromPlayerInfo(const FS_PlayerGameInfo& Info);

protected:

    // ------------------------------------------------------------
    // Bound widgets
    // ------------------------------------------------------------

    UPROPERTY(meta = (BindWidget))
    UTextBlock* txt_name;

    UPROPERTY(meta = (BindWidget))
    UTextBlock* txt_rank;

    UPROPERTY(meta = (BindWidget))
    UTextBlock* txt_stats;
};
