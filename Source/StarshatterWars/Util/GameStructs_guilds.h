#pragma once
#include "CoreMinimal.h"
#include "Engine/DataTable.h"
#include "GameStructs_guilds.generated.h"

UENUM()
enum class EGuildKind : uint8 { Career, PlayerOrganization };
UENUM()
enum class EGuildCareer : uint8 { Pilot, Miner, Industrialist, Trader, Hauler, Explorer, Security };
UENUM(meta=(Bitflags))
enum class EGuildRole : uint8 { Member=1, Recruiter=2, Quartermaster=4, Treasurer=8, Director=16, Leader=32 };
UENUM()
enum class EGuildRequestState : uint8 { Pending, Accepted, Rejected, Withdrawn, Expired };

USTRUCT()
struct FGuildDefinition : public FTableRowBase
{
    GENERATED_BODY()
    UPROPERTY(EditAnywhere) FName Name;
    UPROPERTY(EditAnywhere) FString DisplayName;
    UPROPERTY(EditAnywhere) FString Description;
    UPROPERTY(EditAnywhere) EGuildKind Kind=EGuildKind::Career;
    UPROPERTY(EditAnywhere) EGuildCareer Career=EGuildCareer::Pilot;
    UPROPERTY(EditAnywhere) FName Empire;
    UPROPERTY(EditAnywhere) int32 MinimumRank=0;
    UPROPERTY(EditAnywhere) TArray<FName> RequiredSkills;
    UPROPERTY(EditAnywhere) FName StarterShip;
    UPROPERTY(EditAnywhere) bool bPublicRecruitment=true;
};
USTRUCT()
struct FGuildMember
{
    GENERATED_BODY()
    UPROPERTY(SaveGame) FName Pilot;
    UPROPERTY(SaveGame) FName Empire;
    UPROPERTY(SaveGame) int32 Roles=1;
    UPROPERTY(SaveGame) int64 JoinedUtc=0;
};
USTRUCT()
struct FGuildOrganization
{
    GENERATED_BODY()
    UPROPERTY(SaveGame) FGuid Id;
    UPROPERTY(SaveGame) FName Definition;
    UPROPERTY(SaveGame) FString DisplayName;
    UPROPERTY(SaveGame) EGuildKind Kind=EGuildKind::Career;
    UPROPERTY(SaveGame) EGuildCareer Career=EGuildCareer::Pilot;
    UPROPERTY(SaveGame) FName Empire;
    UPROPERTY(SaveGame) FName Founder;
    UPROPERTY(SaveGame) FName Leader;
    UPROPERTY(SaveGame) int64 CreatedUtc=0;
    UPROPERTY(SaveGame) bool bDisbanded=false;
    UPROPERTY(SaveGame) TMap<FName,FGuildMember> Members;
};
USTRUCT()
struct FGuildApplication
{
    GENERATED_BODY()
    UPROPERTY(SaveGame) FGuid Id;
    UPROPERTY(SaveGame) FGuid Organization;
    UPROPERTY(SaveGame) FName Pilot;
    UPROPERTY(SaveGame) FString Message;
    UPROPERTY(SaveGame) int64 CreatedUtc=0;
    UPROPERTY(SaveGame) int64 ExpiresUtc=0;
    UPROPERTY(SaveGame) EGuildRequestState State=EGuildRequestState::Pending;
};
USTRUCT()
struct FGuildInvitation
{
    GENERATED_BODY()
    UPROPERTY(SaveGame) FGuid Id;
    UPROPERTY(SaveGame) FGuid Organization;
    UPROPERTY(SaveGame) FName Pilot;
    UPROPERTY(SaveGame) FName InvitedBy;
    UPROPERTY(SaveGame) int64 CreatedUtc=0;
    UPROPERTY(SaveGame) int64 ExpiresUtc=0;
    UPROPERTY(SaveGame) EGuildRequestState State=EGuildRequestState::Pending;
};
USTRUCT()
struct FGuildState
{
    GENERATED_BODY()
    UPROPERTY(SaveGame) FGuid LocalPilot;
    UPROPERTY(SaveGame) TMap<FGuid,FGuildOrganization> Organizations;
    UPROPERTY(SaveGame) TMap<FGuid,FGuildApplication> Applications;
    UPROPERTY(SaveGame) TMap<FGuid,FGuildInvitation> Invitations;
    UPROPERTY(SaveGame) TMap<FName,FName> PendingStarterShips;
    UPROPERTY(SaveGame) TSet<FName> ClaimedStarterShips;
};
