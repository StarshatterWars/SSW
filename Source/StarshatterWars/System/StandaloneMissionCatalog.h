#pragma once
#include "GameStructs.h"
namespace StandaloneMissionCatalog
{
    // Atomic JSON ingestion; authoring tables are optional export artifacts.
    bool Load(const FString& Directory,TArray<FS_CampaignMissionList>& Lists,TArray<FS_CampaignMission>& Scenarios,FString& Error);
}
