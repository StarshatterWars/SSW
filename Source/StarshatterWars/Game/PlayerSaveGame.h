/*=============================================================================
    Project:        Starshatter Wars (nGenEx Unreal Port)
    Studio:         Fractal Dev Games
    Copyright:      (C) 2024–2026. All Rights Reserved.

    SUBSYSTEM:      SSW
    FILE:           PlayerSaveGame.h
    AUTHOR:         Carlos Bott

    OVERVIEW
    ========
    UPlayerSaveGame

    USaveGame payload for player profile / progress persistence.

    CONTENTS
    ========
    - SaveVersion: format version for forward-compatible migrations
    - PlayerInfo:  FS_PlayerGameInfo payload (profile + progress)
=============================================================================*/

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/SaveGame.h"
#include "GameStructs.h"
#include "PlayerSaveGame.generated.h"

UCLASS()
class STARSHATTERWARS_API UPlayerSaveGame : public USaveGame
{
    GENERATED_BODY()

public:
    UPlayerSaveGame();

public:
    /* --------------------------------------------------------------------
       Save format version
       -------------------------------------------------------------------- */

    UPROPERTY(BlueprintReadOnly, SaveGame, Category = "SaveGame Data")
    int32 SaveVersion = 1;

    /* --------------------------------------------------------------------
       Authoritative player payload
       -------------------------------------------------------------------- */

    UPROPERTY(BlueprintReadWrite, SaveGame, Category = "SaveGame Data")
    FS_PlayerGameInfo PlayerInfo;
};
