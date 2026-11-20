#pragma once
#include "GameStructs.h"
class Sim;class AgentAI;
struct FDynamicAgentContext
{
 FName Id;FString Name,System,Region,Station;TArray<FName> Pools;int32 Level=1;EEMPIRE_NAME Empire=EEMPIRE_NAME::Neutral;
};
class DynamicMissionAI
{
public:
 static bool IsDynamic(int32 Id){return Id>=950001 && Id<1000000;}
 static bool Generate(FS_PlayerGameInfo&,const FDynamicAgentContext&,int64 Now,FText& Reason);
 static TArray<int32> Offers(const FS_PlayerGameInfo&,FName Agent,int64 Now);
 static bool CanUse(const FS_PlayerGameInfo&,int32 Id,FText& Reason);
 static FText Title(const FS_PlayerGameInfo&,int32 Id);
 static bool Preview(const FS_PlayerGameInfo&,int32 Id,FS_CampaignMission& Out);
 static int32 Points(const FS_PlayerGameInfo&,int32 Id);
 static int64 Reward(const FS_PlayerGameInfo&,int32 Id);
 static bool Start(Sim&,FS_PlayerGameInfo&,AgentAI&,FName Pilot,int32 Id,FText& Reason);
 static bool Advance(Sim&,FS_PlayerGameInfo&,AgentAI&,FName Pilot,double Seconds);
};
