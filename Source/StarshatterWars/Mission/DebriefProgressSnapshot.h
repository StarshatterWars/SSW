#pragma once
#include "AgentAI.h"
#include "GameStructs.h"
#include "PlayerCharacter.h"
#include "CampaignSave.h"

// Debrief rollback retains the exact live journal and runtime pilot, not load-time migration.
struct FDebriefProgressSnapshot
{
    AgentAI Agent;
    FS_PlayerGameInfo Player, RuntimePlayer;
    PlayerCharacter* Runtime = nullptr;
    TArray<FString> Scenes;
    TArray<int32> Completed;
    TArray<FMissionDebriefRecord> Debriefs;
    bool Graduate = false, ClockInitialized = false;
    uint64 ClockStart = 0, ClockElapsed = 0;
    FDebriefProgressSnapshot(const AgentAI& InAgent,const FS_PlayerGameInfo& InPlayer,PlayerCharacter* InRuntime,UCampaignSave* Save)
        : Agent(InAgent),Player(InPlayer),RuntimePlayer(InPlayer),Runtime(InRuntime)
    {
        if(Runtime)Runtime->ToPlayerInfo(RuntimePlayer);
        if(Save){Scenes=Save->PlayedSceneNames;Completed=Save->CompletedMissionIds;Debriefs=Save->MissionDebriefs;
            Graduate=Save->bTrainingGraduate;ClockStart=Save->CampaignStartUniverseSeconds;
            ClockElapsed=Save->CampaignElapsedAtAnchor;ClockInitialized=Save->bInitialized;}
    }
    void Restore(AgentAI& OutAgent,FS_PlayerGameInfo& OutPlayer,UCampaignSave* Save) const
    {
        OutAgent=Agent;OutPlayer=Player;
        if(Runtime)Runtime->FromPlayerInfo(RuntimePlayer);
        if(Save){Save->PlayedSceneNames=Scenes;Save->CompletedMissionIds=Completed;Save->MissionDebriefs=Debriefs;
            Save->bTrainingGraduate=Graduate;Save->CampaignStartUniverseSeconds=ClockStart;
            Save->CampaignElapsedAtAnchor=ClockElapsed;Save->bInitialized=ClockInitialized;}
    }
};
