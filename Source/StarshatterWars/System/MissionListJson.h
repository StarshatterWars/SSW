#pragma once
#include "GameStructs.h"
namespace MissionListJson
{
    enum class EReadResult : uint8 { Missing, Loaded, Invalid };
    bool ParseMissions(const FString& Text,TArray<FS_CampaignMissionList>& Out,FString& Error);
    bool ParseTemplates(const FString& Text,TArray<FS_CampaignTemplateList>& Out,FString& Error);
    EReadResult ReadMissions(const FString& Directory,TArray<FS_CampaignMissionList>& Out,FString& Error);
    EReadResult ReadTemplates(const FString& Directory,TArray<FS_CampaignTemplateList>& Out,FString& Error);
    bool WriteMissions(const FString& Directory,const TArray<FS_CampaignMissionList>& Rows,FString& Error);
    bool WriteTemplates(const FString& Directory,const TArray<FS_CampaignTemplateList>& Rows,FString& Error);
}
