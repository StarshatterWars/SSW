#pragma once
#include "Subsystems/GameInstanceSubsystem.h"
#include "GuildAI.h"
#include "GuildDirectorySaveGame.h"
#include "GuildCatalogSubsystem.generated.h"

UCLASS()
class STARSHATTERWARS_API UGuildCatalogSubsystem : public UGameInstanceSubsystem
{
    GENERATED_BODY()
public:
    void Initialize(FSubsystemCollectionBase& Collection) override;
    bool LoadCatalog();
    const TMap<FName,FGuildDefinition>& GetCatalog() const{return Definitions;}
    const FGuildState& GetState() const;
    FGuildPilotContext GetContext() const;
    FText GetPilotDisplayName(FName Pilot) const;
    bool RegisterLocalPilot(FText& Reason);
    bool EnrollCareer(FName Definition,FText& Reason);
    bool CreateOrganization(const FString& Name,FText& Reason);
    bool Apply(FGuid Organization,FText& Reason);
    bool Invite(FGuid Organization,FName Pilot,FText& Reason);
    bool Review(FGuid Application,bool Accept,FText& Reason);
    bool AcceptInvitation(FGuid Invitation,FText& Reason);
    bool Leave(FGuid Organization,FText& Reason);
    bool SetRoles(FGuid Organization,FName Pilot,int32 Roles,FText& Reason);
    bool TransferLeader(FGuid Organization,FName Pilot,FText& Reason);
    bool ClaimStarter(FName Definition,FText& Reason);
    bool CancelRequest(FGuid Request,bool bInvitation,FText& Reason);
private:
    TMap<FName,FGuildDefinition> Definitions;
    mutable TMap<FName,FString> DisplayNames;
    UPROPERTY() TObjectPtr<UGuildDirectorySaveGame> Directory;
    mutable FGuildState MergedState;
    bool DirectoryValid=true;
    void LoadDirectory();
    bool EnsureIdentity(FText& Reason);
    void UpdateDirectoryPilot(const FGuildPilotContext& Pilot);
    bool SaveDirectory();
    bool CanChange(FText& Reason) const;
    bool ResolvePilot(FName Input,FGuildPilotContext& Pilot) const;
    bool Commit(TFunctionRef<bool(FGuildState&,const FGuildPilotContext&,FText&)> Action,FText& Reason);
    bool CommitOrganization(TFunctionRef<bool(FGuildState&,const FGuildPilotContext&,FText&)> Action,FText& Reason);
};
