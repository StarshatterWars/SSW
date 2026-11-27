#pragma once
#include "GameStructs.h"

// Journal IDs must not collide with authored training or regional missions.
// Campaign missions keep their original identity in the simulation and saves.
namespace AgentMissionRules
{
    inline bool FactionAllows(EEMPIRE_NAME PilotEmpire, const FS_CampaignMission& Mission)
    {
        return PilotEmpire != EEMPIRE_NAME::NONE && PilotEmpire != EEMPIRE_NAME::Unknown &&
            (Mission.RequiredEmpire == EEMPIRE_NAME::Neutral || Mission.RequiredEmpire == PilotEmpire);
    }
    constexpr int32 CampaignBase = 2000000;
    constexpr int32 CampaignStride = 1000000;
    inline int32 Encode(int32 Campaign, int32 Mission)
    {
        return Campaign >= 2 && Campaign <= 5 && Mission > 0 && Mission < CampaignStride
            ? CampaignBase + Campaign * CampaignStride + Mission : INDEX_NONE;
    }
    inline bool Decode(int32 Id, int32& Campaign, int32& Mission)
    {
        if (Id < CampaignBase) return false;
        Campaign = (Id - CampaignBase) / CampaignStride;
        Mission = (Id - CampaignBase) % CampaignStride;
        return Encode(Campaign, Mission) == Id;
    }
    inline bool LocalSystem(const FString& AgentSystem, const FString& MissionSystem)
    {
        return !AgentSystem.IsEmpty() && AgentSystem.Equals(MissionSystem, ESearchCase::IgnoreCase);
    }
}
