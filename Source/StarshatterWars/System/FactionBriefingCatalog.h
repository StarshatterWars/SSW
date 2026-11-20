#pragma once
#include "GameStructs.h"
struct FFactionCampaignBriefing
{
    FName Id;EEMPIRE_NAME Empire=EEMPIRE_NAME::Unknown;int32 CampaignId=0,FirstMissionId=0;
    FString Title,Body;TArray<int32> Prerequisites;
};
namespace FactionBriefingCatalog
{
    const TArray<FFactionCampaignBriefing>& All();
    bool Available(const FFactionCampaignBriefing&,const FS_PlayerGameInfo&);
    const FFactionCampaignBriefing* Find(FName);
}
