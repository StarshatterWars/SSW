/*  Project Starshatter Wars
    Fractal Dev Studios
    Copyright (c) 2025-2026.
    All Rights Reserved.

    ORIGINAL AUTHOR AND STUDIO
    ==========================
    John DiCamillo / Destroyer Studios LLC

    SUBSYSTEM:    Stars.exe
    FILE:         PlayerRosterItem.h
    AUTHOR:       Carlos Bott

    OVERVIEW
    ========
    Player roster list item (data model)
    - UObject-based ListView item
    - Wraps legacy PlayerCharacter*
    - Used by UPlayerDlg roster ListView
*/

#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"

class PlayerCharacter;

#include "PlayerRosterItem.generated.h"

// +--------------------------------------------------------------------+

UCLASS()
class STARSHATTERWARS_API UPlayerRosterItem : public UObject
{
    GENERATED_BODY()

public:
    void Initialize(PlayerCharacter* InPlayer)
    {
        player = InPlayer;
    }

    PlayerCharacter* GetPlayer() const
    {
        return player;
    }

private:
    PlayerCharacter* player = nullptr;
};
