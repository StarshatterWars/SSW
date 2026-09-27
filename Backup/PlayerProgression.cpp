#include "PlayerProgression.h"

#include "Engine/World.h"
#include "Engine/GameInstance.h"

#include "StarshatterPlayerSubsystem.h"
#include "AwardInfoRegistry.h"
#include "GameStructs.h"
#include "ShipStats.h" 

int PlayerProgression::GCurrentRankId = 0;

UStarshatterPlayerSubsystem* PlayerProgression::GetPlayerSubsystem(UWorld* World)
{
    if (!World)
        return nullptr;

    UGameInstance* GI = World->GetGameInstance();
    if (!GI)
        return nullptr;

    return GI->GetSubsystem<UStarshatterPlayerSubsystem>();
}

int32 PlayerProgression::RankFromName(UWorld* World, const FString& RankName)
{
    // Registry owns lookup logic; no subsystem needed for pure name->id lookup
    return UAwardInfoRegistry::RankIdFromName(RankName);
}

void PlayerProgression::SetRank(UWorld* World, int32 RankId)
{
    UStarshatterPlayerSubsystem* PlayerSS = GetPlayerSubsystem(World);
    if (!PlayerSS)
        return;

    FS_PlayerGameInfo& Info = PlayerSS->GetMutablePlayerInfo();
    Info.Rank = RankId;

    PlayerSS->SavePlayer(true);
}

void PlayerProgression::SetCampaignComplete(UWorld* World, int32 CampaignId)
{
    UStarshatterPlayerSubsystem* PlayerSS = GetPlayerSubsystem(World);
    if (!PlayerSS)
        return;

    FS_PlayerGameInfo& Info = PlayerSS->GetMutablePlayerInfo();
    Info.SetCampaignComplete(CampaignId, true);

    PlayerSS->SavePlayer(true);
}

void PlayerProgression::GrantMedal(UWorld* World, int32 MedalId)
{
    UStarshatterPlayerSubsystem* PlayerSS = GetPlayerSubsystem(World);
    if (!PlayerSS)
        return;

    FS_PlayerGameInfo& Info = PlayerSS->GetMutablePlayerInfo();
    Info.MedalsMask |= MedalId;

    PlayerSS->SavePlayer(true);
}

int PlayerProgression::GetCurrentRankId()
{
    return GCurrentRankId;
}

void PlayerProgression::SetCurrentRankId(int InRankId)
{
    GCurrentRankId = InRankId;
}

int32 PlayerProgression::ComputeMissionPoints(const ShipStats* Stats, int32 /*MissionStartTime*/)
{
    if (!Stats)
        return 0;

    // Minimal faithful behavior:
    // Legacy often did additional time-based or award-based tweaks.
    // Start with the canonical stats score.
    return Stats->GetPoints();
}