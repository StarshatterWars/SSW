/*=============================================================================
    Project:        Starshatter Wars (nGenEx Unreal Port)
    Studio:         Fractal Dev Games
    Copyright:      (C) 2024–2026. All Rights Reserved.

    SUBSYSTEM:      SSW
    FILE:           PlayerSaveGame.cpp
    AUTHOR:         Carlos Bott

    IMPLEMENTATION
    ==============
    UPlayerSaveGame constructor initializes default player state and
    sets initial save schema version.
=============================================================================*/

#include "PlayerSaveGame.h"
#include "GameStructs.h"

UPlayerSaveGame::UPlayerSaveGame()
{
    SaveVersion = 1;
    PlayerInfo.Name = TEXT("Default Player");
}