#pragma once
#include "CoreMinimal.h"
#include "GameStructs.h"

namespace CampaignPresentationRules
{
    inline bool HasRuntimeSceneTrigger(const FS_MissionEvent& Event)
    {
        return Event.TriggerNParams > 0 || !Event.TriggerShip.IsEmpty() || !Event.TriggerTarget.IsEmpty() ||
            (Event.EventTrigger != MISSIONEVENT_TRIGGER::TRIGGER_TIME &&
             Event.EventTrigger != MISSIONEVENT_TRIGGER::TRIGGER_EVENT);
    }
    inline bool IsCampaignUnlocked(int32 CampaignIndex, const FS_PlayerGameInfo& Player)
    {
        if (CampaignIndex < 1 || CampaignIndex > 5) return false;
        for (int32 Previous = 2; Previous < CampaignIndex; ++Previous)
            if (!Player.IsCampaignComplete(Previous)) return false;
        return true;
    }
    inline bool IsSceneDue(bool bVisited, bool bHasScene, double EventTime, double CampaignTime)
    {
        return !bVisited && bHasScene && FMath::IsFinite(EventTime) &&
            FMath::IsFinite(CampaignTime) && EventTime <= CampaignTime;
    }
    inline bool IsAuthoredScene(int32 Id, const TArray<FS_CampaignMission>& Missions)
    {
        for (const FS_CampaignMission& Mission : Missions)
            if (Mission.Id == Id && Mission.MissionType != EMissionType::TRAINING && !Mission.Scene.TrimStartAndEnd().IsEmpty()) return true;
        return false;
    }
    inline EMissionDisplayType MissionDisplayType(const FS_CampaignMissionList& Row,
        const TArray<FS_CampaignMission>& Missions)
    {
        return IsAuthoredScene(Row.Id, Missions) ? EMissionDisplayType::CutsceneOnly : Row.DisplayType;
    }
}
