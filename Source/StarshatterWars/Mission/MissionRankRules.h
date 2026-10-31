#pragma once
#include "GameStructs.h"
#include "ShipDesignRegistry.h"
#include "PlayerCharacter.h"
namespace MissionRankRules
{
    inline int32 RequiredForClass(FString Class)
    {
        Class=Class.TrimStartAndEnd().ToLower();
        if(Class==TEXT("fighter"))return 0;
        if(Class==TEXT("attack") || Class==TEXT("lca"))return 1;
        if(Class==TEXT("frigate"))return 3;
        if(Class==TEXT("destroyer"))return 4;
        if(Class==TEXT("cruiser"))return 5;
        if(Class==TEXT("carrier"))return 6;
        return 6;
    }
    inline int32 Required(const FS_CampaignMission& Scenario)
    {
        int32 Rank=0;bool Found=false;
        for(const auto& Element:Scenario.Element)
            if(Element.Player>0)
            {
                Found=true;
                const auto* Design=ShipDesignRegistry::Find(Element.Design);
                Rank=FMath::Max(Rank,Design?RequiredForClass(Design->ShipClass):6);
            }
        return Found?Rank:6;
    }
    inline bool Unlocked(int32 RequiredRank)
    {
        const auto* Player=PlayerCharacter::GetCurrentPlayer();
        return RequiredRank<=(Player?Player->GetRank():0);
    }
    inline FString Requirement(int32 Rank)
    {
        return FString::Printf(TEXT("Requires %s"),UTF8_TO_TCHAR(PlayerCharacter::RankName(Rank)));
    }
}
