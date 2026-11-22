#pragma once
#include "GameStructs.h"
#include "PlayerCharacter.h"
namespace TrainingShuttleRules
{
    inline int32 TransferStage(const FS_PlayerGameInfo& Pilot,bool FreeFlight,bool Docked,bool AboardOrion,bool AtJanek)
    {
        if(!FreeFlight || !Docked || Pilot.Empire!=int32(EEMPIRE_NAME::Terellian))return 0;
        if(AtJanek && Pilot.HasCompletedMission(1,104))return 2;
        if(AboardOrion && Pilot.Campaign==1 && Pilot.HasCompletedMission(1,102) && !Pilot.HasCompletedMission(1,104))return 1;
        return 0;
    }
    inline bool CanResumeSavedMission(const FS_PlayerGameInfo& Pilot, int32 MissionId)
    {
        // A shutdown checkpoint can precede the lesson's committed completion.
        const bool Training = Pilot.Campaign == 1 && Pilot.Empire == int32(EEMPIRE_NAME::Terellian) && MissionId >= 101 && MissionId <= 104;
        return !Training || !Pilot.HasCompletedMission(1, MissionId);
    }
    inline bool AtJanek(const FS_PlayerGameInfo& Pilot)
    { return Pilot.Campaign == 1 && Pilot.Empire == int32(EEMPIRE_NAME::Terellian) && Pilot.HasCompletedMission(1, 103) && !Pilot.HasCompletedMission(1, 104); }
}
