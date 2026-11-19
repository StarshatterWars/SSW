#pragma once
#include "GameStructs.h"

class UDataTable;
namespace CampaignJsonCatalog
{
    // Existing campaign metadata stays in DEF. Present JSON is authoritative:
    // invalid input never silently falls back to older embedded actions/zones.
    bool Apply(const FString& Directory,FS_Campaign& Campaign,UDataTable* Actions,UDataTable* Zones,FString& Error);
    bool Export(const FString& Directory,const FS_Campaign& Campaign,FString& Error);
}
