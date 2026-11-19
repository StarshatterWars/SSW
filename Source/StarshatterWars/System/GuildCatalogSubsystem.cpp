#include "GuildCatalogSubsystem.h"
#include "StarshatterPlayerSubsystem.h"
#include "StationServices.h"
#include "ShipDesignRegistry.h"
#include "Ship.h"
#include "Sim.h"
#include "SimRegion.h"
#include "Engine/GameInstance.h"
#include "Kismet/GameplayStatics.h"
#include "JsonObjectConverter.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"

namespace
{
FName PilotIdentity(FGuid Id){return FName(*(TEXT("GuildPilot/")+Id.ToString(EGuidFormats::Digits)));}
FGuildPilotContext ProfileContext(const FS_PlayerGameInfo& Profile,bool Trusted)
{
    FGuildPilotContext Result;Result.Pilot=Profile.Guilds.LocalPilot.IsValid()?PilotIdentity(Profile.Guilds.LocalPilot):FName(TEXT("UnenrolledPilot"));
    Result.Rank=FMath::Max(0,Profile.Rank);Result.bTrustedAuthority=Trusted;
    if(StaticEnum<EEMPIRE_NAME>()->IsValidEnumValue(Profile.Empire))Result.Empire=FName(*StaticEnum<EEMPIRE_NAME>()->GetNameStringByValue(Profile.Empire));
    for(const auto& Skill:Profile.Skills)if(Skill.Value.bComplete || Skill.Value.TrainedLevel>0)Result.Skills.Add(Skill.Key);
    return Result;
}
}
void UGuildCatalogSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{Super::Initialize(Collection);LoadCatalog();LoadDirectory();}
bool UGuildCatalogSubsystem::LoadCatalog()
{
    FString Json;TArray<TSharedPtr<FJsonValue>> Rows;
    if(!FFileHelper::LoadFileToString(Json,*(FPaths::ProjectContentDir()/TEXT("GameData/Guilds/guilds.json"))) ||
        !FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(Json),Rows))return false;
    TMap<FName,FGuildDefinition> Candidate;
    for(const auto& Row:Rows)
    {
        const auto Object=Row->AsObject();FGuildDefinition Definition;
        if(!Object || !FJsonObjectConverter::JsonObjectToUStruct(Object.ToSharedRef(),&Definition) || Definition.Name.IsNone() ||
            Definition.DisplayName.IsEmpty() || Definition.Description.IsEmpty() || Definition.Empire.IsNone() || Definition.MinimumRank<0 ||
            static_cast<uint8>(Definition.Kind)>1 || static_cast<uint8>(Definition.Career)>6 || Candidate.Contains(Definition.Name))return false;
        for(FName Skill:Definition.RequiredSkills)if(Skill.IsNone())return false;
        Candidate.Add(Definition.Name,MoveTemp(Definition));
    }
    if(Candidate.IsEmpty())return false;Definitions=MoveTemp(Candidate);return true;
}
const FGuildState& UGuildCatalogSubsystem::GetState() const
{
    const auto* Players=GetGameInstance()->GetSubsystem<UStarshatterPlayerSubsystem>();
    MergedState=Players?Players->GetPlayerInfo().Guilds:FGuildState();
    for(auto It=MergedState.Organizations.CreateIterator();It;++It)if(It.Value().Kind==EGuildKind::PlayerOrganization)It.RemoveCurrent();
    MergedState.Applications.Reset();MergedState.Invitations.Reset();
    if(Directory && DirectoryValid)
    {
        for(const auto& Org:Directory->State.Organizations)MergedState.Organizations.Add(Org.Key,Org.Value);
        MergedState.Applications=Directory->State.Applications;MergedState.Invitations=Directory->State.Invitations;
    }
    return MergedState;
}

bool UGuildCatalogSubsystem::CanChange(FText& Reason) const
{
    const auto* Simulation=Sim::GetSim();const auto* Players=GetGameInstance()->GetSubsystem<UStarshatterPlayerSubsystem>();
    if(!Players || !Players->HasLoaded() || !Simulation || Simulation->IsNetworked() || !Simulation->HasSimulationAuthority() || !FStationServices::IsDocked())
    {Reason=INVTEXT("Dock in an offline game to change guild membership or claim a career ship.");return false;}
    return true;
}
FGuildPilotContext UGuildCatalogSubsystem::GetContext() const
{
    const auto* Players=GetGameInstance()->GetSubsystem<UStarshatterPlayerSubsystem>();FText Reason;
    return Players && Players->HasLoaded()?ProfileContext(Players->GetPlayerInfo(),CanChange(Reason)):FGuildPilotContext();
}
bool UGuildCatalogSubsystem::ResolvePilot(FName Input,FGuildPilotContext& Pilot) const
{
    if(!Directory || !DirectoryValid || Input.IsNone())return false;
    bool Found=false;
    for(const auto& Pair:Directory->Pilots)
    {
        const auto& Record=Pair.Value;
        if(Input!=Pair.Key && Input!=FName(*Record.DisplayName))continue;
        if(Found && Pilot.Pilot!=Pair.Key)return false;
        Pilot.Pilot=Record.Identity;Pilot.Empire=Record.Empire;Pilot.Rank=Record.Rank;Pilot.Skills=Record.Skills;
        Pilot.bTrustedAuthority=Record.Identity==Pair.Key && !Record.Identity.IsNone() && !Record.Empire.IsNone();Found=Pilot.bTrustedAuthority;
    }
    return Found;
}
FText UGuildCatalogSubsystem::GetPilotDisplayName(FName Pilot) const
{
    const auto* Players=GetGameInstance()->GetSubsystem<UStarshatterPlayerSubsystem>();
    if(Players && Players->GetPlayerInfo().Guilds.LocalPilot.IsValid() && Pilot==PilotIdentity(Players->GetPlayerInfo().Guilds.LocalPilot))return FText::FromString(Players->GetPlayerInfo().Name);
    if(Directory)if(const auto* Record=Directory->Pilots.Find(Pilot))return FText::FromString(Record->DisplayName);
    return INVTEXT("Unknown pilot");
}
void UGuildCatalogSubsystem::LoadDirectory()
{
    const auto* Players=GetGameInstance()->GetSubsystem<UStarshatterPlayerSubsystem>();const int32 User=Players?Players->GetUserIndex():0;
    const FString Slot=TEXT("SSW_LocalGuildDirectory_v1");
    if(UGameplayStatics::DoesSaveGameExist(Slot,User))
    {
        Directory=Cast<UGuildDirectorySaveGame>(UGameplayStatics::LoadGameFromSlot(Slot,User));DirectoryValid=Directory!=nullptr;
    }
    else {Directory=NewObject<UGuildDirectorySaveGame>(this);DirectoryValid=true;}
}
bool UGuildCatalogSubsystem::SaveDirectory()
{
    const auto* Players=GetGameInstance()->GetSubsystem<UStarshatterPlayerSubsystem>();
    return DirectoryValid && Directory && Players && UGameplayStatics::SaveGameToSlot(Directory,TEXT("SSW_LocalGuildDirectory_v1"),Players->GetUserIndex());
}
bool UGuildCatalogSubsystem::EnsureIdentity(FText& Reason)
{
    if(!CanChange(Reason))return false;auto* Players=GetGameInstance()->GetSubsystem<UStarshatterPlayerSubsystem>();
    if(Players->GetPlayerInfo().Guilds.LocalPilot.IsValid())return true;
    const auto Before=Players->GetPlayerInfo();Players->GetMutablePlayerInfo().Guilds.LocalPilot=FGuid::NewGuid();
    if(!Players->SaveActivePlayerProgress()){Players->GetMutablePlayerInfo()=Before;Reason=INVTEXT("Could not save your permanent guild identity.");return false;}
    return true;
}
void UGuildCatalogSubsystem::UpdateDirectoryPilot(const FGuildPilotContext& Pilot)
{
    auto& Record=Directory->Pilots.FindOrAdd(Pilot.Pilot);Record.Identity=Pilot.Pilot;Record.Empire=Pilot.Empire;Record.Rank=Pilot.Rank;Record.Skills=Pilot.Skills;
    Record.DisplayName=GetGameInstance()->GetSubsystem<UStarshatterPlayerSubsystem>()->GetPlayerInfo().Name;Record.LastSeenUtc=FDateTime::UtcNow().ToUnixTimestamp();
}
bool UGuildCatalogSubsystem::RegisterLocalPilot(FText& Reason)
{
    if(!EnsureIdentity(Reason))return false;
    if(!Directory || !DirectoryValid){Reason=INVTEXT("The local guild directory could not be loaded; it was not overwritten.");return false;}
    const auto Previous=Directory->Pilots;const auto Pilot=GetContext();UpdateDirectoryPilot(Pilot);
    if(!SaveDirectory()){Directory->Pilots=Previous;Reason=INVTEXT("Could not save your local guild directory registration.");return false;}
    return true;
}
bool UGuildCatalogSubsystem::CommitOrganization(TFunctionRef<bool(FGuildState&,const FGuildPilotContext&,FText&)> Action,FText& Reason)
{
    if(!EnsureIdentity(Reason))return false;
    if(!Directory || !DirectoryValid){Reason=INVTEXT("The local guild directory is unavailable; no organization changes were made.");return false;}
    const auto Before=Directory->State;const auto BeforePilots=Directory->Pilots;const auto Pilot=GetContext();UpdateDirectoryPilot(Pilot);
    if(!Action(Directory->State,Pilot,Reason)){Directory->State=Before;Directory->Pilots=BeforePilots;return false;}
    if(!SaveDirectory()){Directory->State=Before;Directory->Pilots=BeforePilots;Reason=INVTEXT("Could not save the organization change. The local directory was restored.");return false;}
    return true;
}

bool UGuildCatalogSubsystem::Commit(TFunctionRef<bool(FGuildState&,const FGuildPilotContext&,FText&)> Action,FText& Reason)
{
    if(!CanChange(Reason))return false;auto* Players=GetGameInstance()->GetSubsystem<UStarshatterPlayerSubsystem>();
    const auto Before=Players->GetPlayerInfo();auto& Profile=Players->GetMutablePlayerInfo();
    if(!Profile.Guilds.LocalPilot.IsValid())Profile.Guilds.LocalPilot=FGuid::NewGuid();
    const auto Context=ProfileContext(Profile,true);
    if(!Action(Profile.Guilds,Context,Reason)){Profile=Before;return false;}
    if(!Players->SaveActivePlayerProgress()){Profile=Before;Reason=INVTEXT("Could not save this guild change. Membership, starter entitlement and hangar were restored.");return false;}
    return true;
}
bool UGuildCatalogSubsystem::EnrollCareer(FName Id,FText& Reason)
{
    const auto* Definition=Definitions.Find(Id);if(!Definition){Reason=INVTEXT("Guild catalog entry unavailable.");return false;}
    return Commit([&](FGuildState& State,const FGuildPilotContext& Pilot,FText& Message){return GuildAI::EnrollCareer(State,*Definition,Pilot,FDateTime::UtcNow().ToUnixTimestamp(),Message).IsValid();},Reason);
}
bool UGuildCatalogSubsystem::CreateOrganization(const FString& Name,FText& Reason)
{
    return CommitOrganization([&](FGuildState& State,const FGuildPilotContext& Pilot,FText& Message)
    {
        for(const auto& Entry:Definitions)if(Entry.Value.Kind==EGuildKind::PlayerOrganization && Entry.Value.Empire==Pilot.Empire)
            return GuildAI::FoundOrganization(State,Entry.Value,Pilot,Name,FDateTime::UtcNow().ToUnixTimestamp(),Message).IsValid();
        Message=INVTEXT("No organization charter is available for your empire.");return false;
    },Reason);
}
bool UGuildCatalogSubsystem::Apply(FGuid Id,FText& Reason)
{
    return CommitOrganization([&](FGuildState& State,const FGuildPilotContext& Pilot,FText& Message)
    {
        const auto* Org=State.Organizations.Find(Id);const auto* Definition=Org?Definitions.Find(Org->Definition):nullptr;
        return Definition && GuildAI::Apply(State,*Definition,Id,Pilot,TEXT("Membership requested."),FDateTime::UtcNow().ToUnixTimestamp(),Message).IsValid();
    },Reason);
}
bool UGuildCatalogSubsystem::Invite(FGuid Id,FName Target,FText& Reason)
{
    return CommitOrganization([&](FGuildState& State,const FGuildPilotContext& Pilot,FText& Message)
    {
        FGuildPilotContext Recipient;if(!ResolvePilot(Target,Recipient)){Message=INVTEXT("A known saved pilot with an established guild identity is required. Unknown names cannot be invited.");return false;}
        const auto* Org=State.Organizations.Find(Id);const auto* Definition=Org?Definitions.Find(Org->Definition):nullptr;const int64 Now=FDateTime::UtcNow().ToUnixTimestamp();
        return Definition && GuildAI::Invite(State,*Definition,Id,Pilot,Recipient,Now,Now+7*86400,Message).IsValid();
    },Reason);
}
bool UGuildCatalogSubsystem::Review(FGuid Id,bool Accept,FText& Reason)
{
    return CommitOrganization([&](FGuildState& State,const FGuildPilotContext& Pilot,FText& Message)
    {
        const auto* Request=State.Applications.Find(Id);const auto* Org=Request?State.Organizations.Find(Request->Organization):nullptr;const auto* Definition=Org?Definitions.Find(Org->Definition):nullptr;
        FGuildPilotContext Applicant;if(!Request || !ResolvePilot(Request->Pilot,Applicant)){Message=INVTEXT("The applicant's saved identity and qualifications are unavailable.");return false;}
        return Definition && GuildAI::ReviewApplication(State,*Definition,Pilot,Applicant,Id,Accept,FDateTime::UtcNow().ToUnixTimestamp(),Message);
    },Reason);
}
bool UGuildCatalogSubsystem::AcceptInvitation(FGuid Id,FText& Reason)
{
    return CommitOrganization([&](FGuildState& State,const FGuildPilotContext& Pilot,FText& Message)
    {
        const auto* Request=State.Invitations.Find(Id);const auto* Org=Request?State.Organizations.Find(Request->Organization):nullptr;const auto* Definition=Org?Definitions.Find(Org->Definition):nullptr;
        return Definition && GuildAI::AcceptInvitation(State,*Definition,Pilot,Id,FDateTime::UtcNow().ToUnixTimestamp(),Message);
    },Reason);
}
bool UGuildCatalogSubsystem::Leave(FGuid Id,FText& Reason)
{
    const auto* Org=GetState().Organizations.Find(Id);const bool Career=Org && Org->Kind==EGuildKind::Career;
    const auto Action=[&](FGuildState& State,const FGuildPilotContext& Pilot,FText& Message){return GuildAI::Leave(State,Pilot,Id,Message);};
    return Career?Commit(Action,Reason):CommitOrganization(Action,Reason);
}
bool UGuildCatalogSubsystem::SetRoles(FGuid Id,FName Target,int32 Roles,FText& Reason)
{return CommitOrganization([&](FGuildState& State,const FGuildPilotContext& Pilot,FText& Message){return GuildAI::SetMemberRoles(State,Pilot,Id,Target,Roles,Message);},Reason);}
bool UGuildCatalogSubsystem::TransferLeader(FGuid Id,FName Target,FText& Reason)
{return CommitOrganization([&](FGuildState& State,const FGuildPilotContext& Pilot,FText& Message){return GuildAI::TransferLeadership(State,Pilot,Id,Target,Message);},Reason);}
bool UGuildCatalogSubsystem::ClaimStarter(FName Id,FText& Reason)
{
    const auto* Definition=Definitions.Find(Id);if(!Definition){Reason=INVTEXT("Guild catalog entry unavailable.");return false;}
    return Commit([&](FGuildState& State,const FGuildPilotContext& Pilot,FText& Message)
    {
        FName Design;if(!GuildAI::ClaimStarterShip(State,*Definition,Pilot,Design,Message))return false;
        auto* Players=GetGameInstance()->GetSubsystem<UStarshatterPlayerSubsystem>();auto& Profile=Players->GetMutablePlayerInfo();const auto* Row=ShipDesignRegistry::Find(Design);
        if(!Row || Profile.HangarShips.Num()>=128){Message=INVTEXT("The starter hull design is unavailable or your hangar is full. No entitlement was consumed.");return false;}
        if(FName(*StaticEnum<EShipEmpire>()->GetNameStringByValue(static_cast<int64>(Row->ShipEmpire)))!=Pilot.Empire)
        {Message=INVTEXT("This starter hull is not available to your empire.");return false;}
        FPlayerHangarShip Hull;Hull.HullId=FGuid::NewGuid();Hull.Design=Design;Hull.Name=Definition->DisplayName+TEXT(" starter");Hull.HullValue=FMath::Clamp(Row->BaseCost,0LL,1000000000000000LL);
        Hull.bCashPurchased=false;Hull.bAssigned=false;Hull.bLost=false;Hull.bHasCondition=true;Hull.HullCondition=1;
        auto* Simulation=Sim::GetSim();const auto* Craft=Simulation?Simulation->GetPlayerShip():nullptr;
        Hull.Location=Craft && Craft->GetCarrier()?UTF8_TO_TCHAR(Craft->GetCarrier()->GetName()):Profile.HomeRegion.ToString();
        if(!Row->Loadout.IsEmpty()){Hull.LoadoutName=Row->Loadout[0].Name;Hull.LoadoutStations=Row->Loadout[0].Stations;}
        Profile.HangarShips.Add(MoveTemp(Hull));Message=INVTEXT("Starter ship added to your hangar with standard insurance eligibility. Your current mission ship is unchanged.");return true;
    },Reason);
}

bool UGuildCatalogSubsystem::CancelRequest(FGuid Id,bool Invitation,FText& Reason)
{return CommitOrganization([&](FGuildState& State,const FGuildPilotContext& Pilot,FText& Message){return GuildAI::CancelRequest(State,Pilot,Id,Invitation,Message);},Reason);}
