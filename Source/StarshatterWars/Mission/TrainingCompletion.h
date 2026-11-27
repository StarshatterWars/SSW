#pragma once
#include "GameStructs.h"
namespace TrainingCompletion
{
    inline bool Record(FS_PlayerGameInfo& State)
    {
        for (int32 Lesson=101; Lesson<=104; ++Lesson)
            if (!State.HasCompletedMission(1, Lesson)) return false;
        State.SetCampaignComplete(1, true);
        return true;
    }
}
