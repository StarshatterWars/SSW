#pragma once
#include "SSWGameInstance.h"
#include "PlayerCharacter.h"

namespace TrainingReplay
{
    inline bool Matches(TConstArrayView<AgentAI::FAssignment> Queue,int32 MissionId)
    {
        return MissionId>=101 && MissionId<=107 && !Queue.IsEmpty() &&
            Queue[0].MissionId==MissionId && Queue[0].bTestReplay;
    }
    inline bool IsActive(USSWGameInstance* Game,int32 MissionId)
    {
        const auto* Pilot=PlayerCharacter::GetCurrentPlayer();
        return Game && Pilot && Matches(Game->GetTrainingAgent().GetQueue(AgentAI::PilotKey(*Pilot)),MissionId);
    }
}
