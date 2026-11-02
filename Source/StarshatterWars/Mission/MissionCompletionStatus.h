#pragma once
#include "CampaignSave.h"
#include "GameStructs.h"

namespace MissionCompletionStatus
{
    inline bool IsCompleted(const UCampaignSave* Save,const FS_PlayerGameInfo* Player,int32 CampaignId,int32 MissionId)
    {
        return MissionId>0 && ((Save && Save->CampaignIndex==CampaignId && Save->CompletedMissionIds.Contains(MissionId))
            || (Player && Player->HasCompletedMission(CampaignId,MissionId)));
    }
    // Older campaign saves already contain successful assignment IDs. Import
    // these into the current pilot once, without inferring success from points.
    inline bool ImportCampaign(FS_PlayerGameInfo& Player,const UCampaignSave& Save,int32 CampaignId)
    {
        if(Save.CampaignIndex!=CampaignId)return false;
        bool Changed=false;
        for(int32 Id:Save.CompletedMissionIds)Changed|=Player.MarkMissionCompleted(CampaignId,Id);
        if(CampaignId==1 && (Save.bTrainingGraduate || Save.CompletedMissionIds.Contains(103)) && !Player.bTrainingGraduate)
        {Player.bTrainingGraduate=true;Changed=true;}
        return Changed;
    }
}
