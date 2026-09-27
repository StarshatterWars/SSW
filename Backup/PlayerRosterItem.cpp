/*=============================================================================
    Project:        Starshatter Wars
    Studio:         Fractal Dev Studios
    FILE:           PlayerRosterItem.cpp
    AUTHOR:         Carlos Bott
=============================================================================*/

#include "PlayerRosterItem.h"

// +--------------------------------------------------------------------+

void UPlayerRosterItem::Initialize(const FS_PlayerGameInfo& InInfo)
{
    PlayerInfo = InInfo;
}

// +--------------------------------------------------------------------+

const FS_PlayerGameInfo& UPlayerRosterItem::GetInfo() const
{
    return PlayerInfo;
}
