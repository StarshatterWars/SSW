#pragma once
#include "GameStructs_guilds.h"

struct FGuildPilotContext
{
    FName Pilot;
    FName Empire;
    int32 Rank=0;
    TSet<FName> Skills;
    bool bTrustedAuthority=false;
};

// Local authoritative foundation. Network identity, shared funds and shared inventories are separate services.
class STARSHATTERWARS_API GuildAI
{
public:
    static bool Eligible(const FGuildDefinition&,const FGuildPilotContext&,FText& Reason);
    static FGuid EnrollCareer(FGuildState&,const FGuildDefinition&,const FGuildPilotContext&,int64 NowUtc,FText& Reason);
    static FGuid FoundOrganization(FGuildState&,const FGuildDefinition&,const FGuildPilotContext&,const FString& Name,int64 NowUtc,FText& Reason);
    static FGuid Apply(FGuildState&,const FGuildDefinition&,FGuid Organization,const FGuildPilotContext&,const FString& Message,int64 NowUtc,FText& Reason);
    static bool ReviewApplication(FGuildState&,const FGuildDefinition&,const FGuildPilotContext& Actor,const FGuildPilotContext& Applicant,FGuid Application,bool Accept,int64 NowUtc,FText& Reason);
    static FGuid Invite(FGuildState&,const FGuildDefinition&,FGuid Organization,const FGuildPilotContext& Actor,const FGuildPilotContext& Target,int64 NowUtc,int64 ExpiresUtc,FText& Reason);
    static bool AcceptInvitation(FGuildState&,const FGuildDefinition&,const FGuildPilotContext&,FGuid Invitation,int64 NowUtc,FText& Reason);
    static bool Leave(FGuildState&,const FGuildPilotContext&,FGuid Organization,FText& Reason);
    static bool SetMemberRoles(FGuildState&,const FGuildPilotContext& Actor,FGuid Organization,FName Target,int32 Roles,FText& Reason);
    static bool TransferLeadership(FGuildState&,const FGuildPilotContext& Actor,FGuid Organization,FName Target,FText& Reason);
    static bool ClaimStarterShip(FGuildState&,const FGuildDefinition&,const FGuildPilotContext&,FName& Ship,FText& Reason);
    static bool CancelRequest(FGuildState&,const FGuildPilotContext&,FGuid Request,bool Invitation,FText& Reason);
    static bool HasRole(const FGuildOrganization&,FName Pilot,EGuildRole Role);
};
