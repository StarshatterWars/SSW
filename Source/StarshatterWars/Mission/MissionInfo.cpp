/*  Project Starshatter Wars
    Fractal Dev Studios
    Copyright (c) 2025-2026.

    SUBSYSTEM:    Stars.exe
    FILE:         MissionInfo.cpp
    AUTHOR:       Carlos Bott

    ORIGINAL AUTHOR AND STUDIO
    ==========================
    John DiCamillo / Destroyer Studios LLC

    OVERVIEW
    ========
    MissionInfo
    - Lightweight campaign-level mission metadata
    - Controls availability rules and mission instancing
*/

#include "MissionInfo.h"

#include "Campaign.h"
#include "Mission.h"
#include "CombatGroup.h"
#include "PlayerCharacter.h"

// Unreal minimal support:
#include "CoreMinimal.h"
#include "GameStructs.h"

// --------------------------------------------------------------------
// Constants
// --------------------------------------------------------------------

static const int TIME_NEVER = (int)1e9;

// --------------------------------------------------------------------
// MissionInfo
// --------------------------------------------------------------------

MissionInfo::MissionInfo()
    : mission(nullptr)
    , start(0)
    , type(0)
    , id(0)
    , min_rank(0)
    , max_rank(0)
    , action_id(0)
    , action_status(0)
    , exec_once(0)
    , start_before(TIME_NEVER)
    , start_after(0)
{
}

MissionInfo::~MissionInfo()
{
    delete mission;
    mission = nullptr;
}

// --------------------------------------------------------------------
// Availability logic
// --------------------------------------------------------------------

bool MissionInfo::IsAvailable()
{
    Campaign* campaign = Campaign::GetCampaign();
    PlayerCharacter* player = PlayerCharacter::GetCurrentPlayer();
    CombatGroup* player_group = campaign ? campaign->GetPlayerGroup() : nullptr;

    if (!campaign || !player || !player_group)
        return false;

    const double campaign_time = campaign->GetTime();

    if (campaign_time < start_after)
        return false;

    if (campaign_time > start_before)
        return false;

    if (region.length() && player_group->GetRegion() != region)
        return false;

    if (min_rank && player->GetRank() < min_rank)
        return false;

    if (max_rank && player->GetRank() > max_rank)
        return false;

    // One-shot mission handling:
    if (exec_once < 0)
        return false;

    if (exec_once > 0)
        exec_once = -1;

    return true;
}
