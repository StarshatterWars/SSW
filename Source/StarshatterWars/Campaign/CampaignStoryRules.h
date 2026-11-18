#pragma once
#include "GameStructs.h"

// A playable companion assignment is distinct from the cinematic that unlocks it.
namespace CampaignStoryRules
{
    inline bool IsPlayable(const FS_CampaignMission& Row, int32 CampaignId)
    {
        if (CampaignId < 2 || CampaignId > 5 || Row.StoryCampaignId != CampaignId || Row.Id <= 0 ||
            Row.DisplayType != EMissionDisplayType::PlayerMission || !Row.Scene.TrimStartAndEnd().IsEmpty() ||
            Row.StoryUnlockScene.TrimStartAndEnd().IsEmpty() || Row.Objective.TrimStartAndEnd().IsEmpty()) return false;
        return Row.Element.ContainsByPredicate([](const FS_MissionElement& Element)
        {
            return Element.Player > 0 && Element.Count > 0 && !Element.Design.IsEmpty() &&
                !Element.Design.Equals(TEXT("CameraPod"), ESearchCase::IgnoreCase) &&
                !Element.Design.Equals(TEXT("CameraPodShip"), ESearchCase::IgnoreCase) &&
                !Element.Instruction.IsEmpty();
        });
    }
    inline bool IsUnlocked(const FS_CampaignMission& Row, int32 CampaignId, bool bScenePresented)
    {
        return IsPlayable(Row, CampaignId) && bScenePresented;
    }
}
