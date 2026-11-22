#pragma once
#include "AgentAI.h"
#include "GameStructs.h"
namespace FreeFlightAssignment
{
    inline constexpr int32 Id = 900010;
    inline bool Unlocked(int32 Rank, bool bTrainingCampaign = false, bool bTrainingComplete = false)
    { return Rank >= 2 && (!bTrainingCampaign || bTrainingComplete); } // Ensign
    inline bool Unlocked(int32 Rank, const FS_PlayerGameInfo& PilotState)
    {
        bool Complete = true;
        for (int32 Lesson = 101; Lesson <= 104; ++Lesson)
            Complete &= PilotState.HasCompletedMission(1, Lesson);
        return Unlocked(Rank, PilotState.Campaign == 1, Complete);
    }
    inline bool IsStarted(const AgentAI& Journal, FName Pilot)
    {
        const auto Queue=Journal.GetQueue(Pilot);
        return !Queue.IsEmpty() && Queue[0].MissionId==Id && Queue[0].State==AgentAI::EState::InProgress;
    }
}
