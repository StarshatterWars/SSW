#pragma once
#include "GameStructs.h"
struct FMedalEligibilityContext
{
    int32 Rank=0, Medals=0, ShipClass=0, TotalPoints=0, TotalMissions=0;
    int32 MissionPoints=0, Kills=0, Deaths=0, Collisions=0;
    bool bCompletedCampaign=false, bHasCampaign=false, bDynamicCampaign=false;
};
namespace MedalEligibility
{
    // Eligibility is deterministic; the caller performs the authored final lottery.
    bool MeetsRequirements(const FMedalInfo& Medal,const FMedalEligibilityContext& Context);
}
