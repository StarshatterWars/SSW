#pragma once
#include "CoreMinimal.h"
#include "GameStructs.h"

namespace CampaignPresentationRules
{
    inline bool IsSceneDue(bool bVisited, bool bHasScene, double EventTime, double CampaignTime)
    {
        return !bVisited && bHasScene && FMath::IsFinite(EventTime) &&
            FMath::IsFinite(CampaignTime) && EventTime <= CampaignTime;
    }
    inline bool IsAuthoredScene(int32 Id, const TArray<FS_CampaignMission>& Missions)
    {
        for (const FS_CampaignMission& Mission : Missions)
            if (Mission.Id == Id && !Mission.Scene.TrimStartAndEnd().IsEmpty()) return true;
        return false;
    }
    inline EMissionDisplayType MissionDisplayType(const FS_CampaignMissionList& Row,
        const TArray<FS_CampaignMission>& Missions)
    {
        return IsAuthoredScene(Row.Id, Missions) ? EMissionDisplayType::CutsceneOnly : Row.DisplayType;
    }
}
