#include "GuildAI.h"

namespace
{
constexpr int32 MemberRole=static_cast<int32>(EGuildRole::Member);
constexpr int32 LeaderRole=static_cast<int32>(EGuildRole::Leader);
constexpr int32 DirectorRole=static_cast<int32>(EGuildRole::Director);
constexpr int32 AllRoles=63;
bool Reject(FText& Reason,const FText& Text){Reason=Text;return false;}
bool Trusted(const FGuildPilotContext& Pilot,FText& Reason)
{
    return Pilot.bTrustedAuthority && !Pilot.Pilot.IsNone() && !Pilot.Empire.IsNone()
        ? true : Reject(Reason,INVTEXT("Guild changes require an authoritative pilot identity."));
}
FName Entitlement(FName Pilot,FName Definition){return FName(*(Pilot.ToString()+TEXT("/")+Definition.ToString()));}
FName StarterClaimKey(FName Pilot){return FName(*(Pilot.ToString()+TEXT("/@CareerStarter")));}
bool StarterClaimed(const FGuildState& State,FName Pilot)
{
    const FString Prefix=Pilot.ToString()+TEXT("/");
    for(FName Key:State.ClaimedStarterShips)if(Key.ToString().StartsWith(Prefix,ESearchCase::IgnoreCase))return true;
    return false;
}
bool Matches(const FGuildOrganization* Org,const FGuildDefinition& Definition)
{return Org && !Org->bDisbanded && Org->Definition==Definition.Name && Org->Kind==Definition.Kind && Org->Career==Definition.Career;}
bool MembershipFree(const FGuildState& State,const FGuildDefinition& Definition,FName Pilot,FText& Reason)
{
    for(const auto& Pair:State.Organizations)
        if(!Pair.Value.bDisbanded && Pair.Value.Members.Contains(Pilot) && Pair.Value.Kind==Definition.Kind &&
            (Definition.Kind==EGuildKind::PlayerOrganization || Pair.Value.Career==Definition.Career))
            return Reject(Reason,INVTEXT("Leave your existing organization or this career's guild before joining another."));
    return true;
}
bool CanJoin(const FGuildState& State,const FGuildOrganization* Org,const FGuildDefinition& Definition,const FGuildPilotContext& Pilot,FText& Reason)
{
    if(!GuildAI::Eligible(Definition,Pilot,Reason))return false;
    if(!Matches(Org,Definition))return Reject(Reason,INVTEXT("This guild is unavailable."));
    if(Org->Empire!=Pilot.Empire)return Reject(Reason,INVTEXT("Guild membership cannot cross empire boundaries."));
    if(Org->Members.Num()>=128)return Reject(Reason,INVTEXT("This organization has reached its member limit."));
    return MembershipFree(State,Definition,Pilot.Pilot,Reason);
}
void AddMember(FGuildState& State,FGuildOrganization& Org,const FGuildDefinition& Definition,const FGuildPilotContext& Pilot,int64 Now)
{
    FGuildMember Member;Member.Pilot=Pilot.Pilot;Member.Empire=Pilot.Empire;Member.JoinedUtc=Now;Org.Members.Add(Pilot.Pilot,Member);
    const FName Key=Entitlement(Pilot.Pilot,Definition.Name);
    bool PendingStarter=false;const FString Prefix=Pilot.Pilot.ToString()+TEXT("/");
    for(const auto& Pending:State.PendingStarterShips)if(Pending.Key.ToString().StartsWith(Prefix,ESearchCase::IgnoreCase)){PendingStarter=true;break;}
    if(Definition.Kind==EGuildKind::Career && !Definition.StarterShip.IsNone() && !StarterClaimed(State,Pilot.Pilot) && !PendingStarter)
        State.PendingStarterShips.Add(Key,Definition.StarterShip);
    for(auto& Pair:State.Applications)if(Pair.Value.Organization==Org.Id && Pair.Value.Pilot==Pilot.Pilot && Pair.Value.State==EGuildRequestState::Pending)Pair.Value.State=EGuildRequestState::Withdrawn;
    for(auto& Pair:State.Invitations)if(Pair.Value.Organization==Org.Id && Pair.Value.Pilot==Pilot.Pilot && Pair.Value.State==EGuildRequestState::Pending)Pair.Value.State=EGuildRequestState::Withdrawn;
}
bool Recruiter(const FGuildOrganization* Org,const FGuildPilotContext& Actor,FText& Reason)
{
    if(!Trusted(Actor,Reason))return false;
    return Org && !Org->bDisbanded && Org->Kind==EGuildKind::PlayerOrganization && Org->Empire==Actor.Empire &&
        (GuildAI::HasRole(*Org,Actor.Pilot,EGuildRole::Recruiter) || GuildAI::HasRole(*Org,Actor.Pilot,EGuildRole::Director))
        ? true : Reject(Reason,INVTEXT("Recruitment permission is required."));
}
}

bool GuildAI::Eligible(const FGuildDefinition& Definition,const FGuildPilotContext& Pilot,FText& Reason)
{
    if(!Trusted(Pilot,Reason))return false;
    if(Definition.Name.IsNone() || Definition.MinimumRank<0 || static_cast<uint8>(Definition.Kind)>1 || static_cast<uint8>(Definition.Career)>6)
        return Reject(Reason,INVTEXT("Invalid guild definition."));
    if(!Definition.Empire.IsNone() && Definition.Empire!=Pilot.Empire)return Reject(Reason,INVTEXT("This guild belongs to another empire."));
    if(Pilot.Rank<Definition.MinimumRank)return Reject(Reason,FText::Format(INVTEXT("Requires rank {0}."),FText::AsNumber(Definition.MinimumRank)));
    for(FName Skill:Definition.RequiredSkills)if(Skill.IsNone() || !Pilot.Skills.Contains(Skill))return Reject(Reason,FText::Format(INVTEXT("Requires skill: {0}."),FText::FromName(Skill)));
    Reason=INVTEXT("Eligible to join.");return true;
}
bool GuildAI::HasRole(const FGuildOrganization& Org,FName Pilot,EGuildRole Role)
{
    const auto* Member=Org.Members.Find(Pilot);
    return !Org.bDisbanded && Member && Member->Pilot==Pilot && Member->Empire==Org.Empire && (Member->Roles&static_cast<int32>(Role))!=0;
}
FGuid GuildAI::EnrollCareer(FGuildState& State,const FGuildDefinition& Definition,const FGuildPilotContext& Pilot,int64 Now,FText& Reason)
{
    if(Definition.Kind!=EGuildKind::Career || Now<0 || !Eligible(Definition,Pilot,Reason))return {};
    FGuildOrganization* Org=nullptr;
    for(auto& Pair:State.Organizations)if(Matches(&Pair.Value,Definition) && Pair.Value.Empire==Pilot.Empire){Org=&Pair.Value;break;}
    if(Org && Org->Members.Contains(Pilot.Pilot)){Reason=INVTEXT("Already enrolled.");return Org->Id;}
    if(!MembershipFree(State,Definition,Pilot.Pilot,Reason))return {};
    if(!Org)
    {
        if(State.Organizations.Num()>=256){Reject(Reason,INVTEXT("The guild registry is full."));return {};}
        FGuildOrganization New;New.Id=FGuid::NewGuid();New.Definition=Definition.Name;New.DisplayName=Definition.DisplayName;New.Kind=Definition.Kind;
        New.Career=Definition.Career;New.Empire=Pilot.Empire;New.CreatedUtc=Now;Org=&State.Organizations.Add(New.Id,New);
    }
    if(!CanJoin(State,Org,Definition,Pilot,Reason))return {};
    AddMember(State,*Org,Definition,Pilot,Now);Reason=INVTEXT("Career guild enrollment accepted.");return Org->Id;
}
FGuid GuildAI::FoundOrganization(FGuildState& State,const FGuildDefinition& Definition,const FGuildPilotContext& Pilot,const FString& Name,int64 Now,FText& Reason)
{
    const FString Clean=Name.TrimStartAndEnd();
    if(Definition.Kind!=EGuildKind::PlayerOrganization || Now<0 || !Eligible(Definition,Pilot,Reason) || !MembershipFree(State,Definition,Pilot.Pilot,Reason))return {};
    if(Clean.Len()<3 || Clean.Len()>64 || State.Organizations.Num()>=256){Reject(Reason,INVTEXT("Choose an organization name between 3 and 64 characters."));return {};}
    for(const auto& Pair:State.Organizations)if(!Pair.Value.bDisbanded && Pair.Value.DisplayName.Equals(Clean,ESearchCase::IgnoreCase)){Reject(Reason,INVTEXT("This organization name is already in use."));return {};}
    FGuildOrganization Org;Org.Id=FGuid::NewGuid();Org.Definition=Definition.Name;Org.DisplayName=Clean;Org.Kind=Definition.Kind;Org.Career=Definition.Career;
    Org.Empire=Pilot.Empire;Org.Founder=Org.Leader=Pilot.Pilot;Org.CreatedUtc=Now;
    FGuildMember Founder;Founder.Pilot=Pilot.Pilot;Founder.Empire=Pilot.Empire;Founder.JoinedUtc=Now;Founder.Roles=AllRoles;Org.Members.Add(Pilot.Pilot,Founder);
    State.Organizations.Add(Org.Id,Org);Reason=INVTEXT("Organization founded.");return Org.Id;
}
FGuid GuildAI::Apply(FGuildState& State,const FGuildDefinition& Definition,FGuid Id,const FGuildPilotContext& Pilot,const FString& Message,int64 Now,FText& Reason)
{
    if(Definition.Kind!=EGuildKind::PlayerOrganization || !Definition.bPublicRecruitment || Now<0 || Now>MAX_int64-7*86400 || Message.Len()>2000 ||
        !CanJoin(State,State.Organizations.Find(Id),Definition,Pilot,Reason))return {};
    for(const auto& Pair:State.Applications)if(Pair.Value.Organization==Id && Pair.Value.Pilot==Pilot.Pilot && Pair.Value.State==EGuildRequestState::Pending && Pair.Value.ExpiresUtc>Now)
    {Reject(Reason,INVTEXT("An application is already pending."));return {};}
    if(State.Applications.Num()>=4096){Reject(Reason,INVTEXT("The application registry is full."));return {};}
    FGuildApplication Entry;Entry.Id=FGuid::NewGuid();Entry.Organization=Id;Entry.Pilot=Pilot.Pilot;Entry.Message=Message;Entry.CreatedUtc=Now;Entry.ExpiresUtc=Now+7*86400;
    State.Applications.Add(Entry.Id,Entry);Reason=INVTEXT("Application submitted for review.");return Entry.Id;
}
bool GuildAI::ReviewApplication(FGuildState& State,const FGuildDefinition& Definition,const FGuildPilotContext& Actor,const FGuildPilotContext& Applicant,FGuid Id,bool Accept,int64 Now,FText& Reason)
{
    auto* Entry=State.Applications.Find(Id);auto* Org=Entry?State.Organizations.Find(Entry->Organization):nullptr;
    if(!Entry || Entry->State!=EGuildRequestState::Pending || Now<0 || Entry->ExpiresUtc<=Now || Entry->Pilot!=Applicant.Pilot || !Recruiter(Org,Actor,Reason))return false;
    if(Accept)
    {
        if(!CanJoin(State,Org,Definition,Applicant,Reason))return false;
        AddMember(State,*Org,Definition,Applicant,Now);
    }
    Entry->State=Accept?EGuildRequestState::Accepted:EGuildRequestState::Rejected;Reason=Accept?INVTEXT("Application accepted."):INVTEXT("Application rejected.");return true;
}
FGuid GuildAI::Invite(FGuildState& State,const FGuildDefinition& Definition,FGuid Id,const FGuildPilotContext& Actor,const FGuildPilotContext& Target,int64 Now,int64 Expires,FText& Reason)
{
    auto* Org=State.Organizations.Find(Id);
    if(Now<0 || Expires<=Now || Expires-Now>30*86400 || !Recruiter(Org,Actor,Reason) || !CanJoin(State,Org,Definition,Target,Reason))return {};
    for(const auto& Pair:State.Invitations)if(Pair.Value.Organization==Id && Pair.Value.Pilot==Target.Pilot && Pair.Value.State==EGuildRequestState::Pending && Pair.Value.ExpiresUtc>Now)
    {Reject(Reason,INVTEXT("An invitation is already pending."));return {};}
    if(State.Invitations.Num()>=4096){Reject(Reason,INVTEXT("The invitation registry is full."));return {};}
    FGuildInvitation Entry;Entry.Id=FGuid::NewGuid();Entry.Organization=Id;Entry.Pilot=Target.Pilot;Entry.InvitedBy=Actor.Pilot;Entry.CreatedUtc=Now;Entry.ExpiresUtc=Expires;
    State.Invitations.Add(Entry.Id,Entry);Reason=INVTEXT("Invitation issued.");return Entry.Id;
}
bool GuildAI::AcceptInvitation(FGuildState& State,const FGuildDefinition& Definition,const FGuildPilotContext& Pilot,FGuid Id,int64 Now,FText& Reason)
{
    auto* Entry=State.Invitations.Find(Id);auto* Org=Entry?State.Organizations.Find(Entry->Organization):nullptr;
    if(!Entry || Entry->State!=EGuildRequestState::Pending || Entry->Pilot!=Pilot.Pilot || Now<0 || Entry->ExpiresUtc<=Now || !CanJoin(State,Org,Definition,Pilot,Reason))return false;
    if(!HasRole(*Org,Entry->InvitedBy,EGuildRole::Recruiter) && !HasRole(*Org,Entry->InvitedBy,EGuildRole::Director))return Reject(Reason,INVTEXT("The inviter no longer has recruitment permission."));
    AddMember(State,*Org,Definition,Pilot,Now);Entry->State=EGuildRequestState::Accepted;Reason=INVTEXT("Invitation accepted.");return true;
}
bool GuildAI::Leave(FGuildState& State,const FGuildPilotContext& Pilot,FGuid Id,FText& Reason)
{
    if(!Trusted(Pilot,Reason))return false;auto* Org=State.Organizations.Find(Id);
    if(!Org || Org->bDisbanded || Org->Empire!=Pilot.Empire || !Org->Members.Contains(Pilot.Pilot))return Reject(Reason,INVTEXT("You are not a member of this organization."));
    if(Org->Leader==Pilot.Pilot && Org->Members.Num()>1)return Reject(Reason,INVTEXT("Transfer leadership before leaving."));
    Org->Members.Remove(Pilot.Pilot);
    if(Org->Kind==EGuildKind::PlayerOrganization && Org->Members.IsEmpty())
    {
        Org->bDisbanded=true;Org->Leader=NAME_None;
        for(auto& Pair:State.Invitations)if(Pair.Value.Organization==Id && Pair.Value.State==EGuildRequestState::Pending)Pair.Value.State=EGuildRequestState::Withdrawn;
        for(auto& Pair:State.Applications)if(Pair.Value.Organization==Id && Pair.Value.State==EGuildRequestState::Pending)Pair.Value.State=EGuildRequestState::Withdrawn;
    }
    Reason=INVTEXT("Membership ended.");return true;
}
bool GuildAI::SetMemberRoles(FGuildState& State,const FGuildPilotContext& Actor,FGuid Id,FName Target,int32 Roles,FText& Reason)
{
    if(!Trusted(Actor,Reason))return false;auto* Org=State.Organizations.Find(Id);auto* Member=Org?Org->Members.Find(Target):nullptr;
    if(!Org || !Member || Org->bDisbanded || Org->Kind!=EGuildKind::PlayerOrganization || Org->Empire!=Actor.Empire || (Roles&~AllRoles)!=0 || Roles<0 ||
        (Roles&LeaderRole)!=0 || Target==Org->Leader || Target==Actor.Pilot)return Reject(Reason,INVTEXT("Use leadership transfer to change the leader. Members cannot grant their own roles."));
    const bool Leader=Org->Leader==Actor.Pilot && HasRole(*Org,Actor.Pilot,EGuildRole::Leader);
    if(!Leader && (!HasRole(*Org,Actor.Pilot,EGuildRole::Director) || (Roles&DirectorRole)!=0 || (Member->Roles&DirectorRole)!=0))return Reject(Reason,INVTEXT("You cannot grant or remove these roles."));
    Member->Roles=Roles|MemberRole;Reason=INVTEXT("Member roles updated.");return true;
}
bool GuildAI::TransferLeadership(FGuildState& State,const FGuildPilotContext& Actor,FGuid Id,FName Target,FText& Reason)
{
    if(!Trusted(Actor,Reason))return false;auto* Org=State.Organizations.Find(Id);auto* Next=Org?Org->Members.Find(Target):nullptr;
    if(!Org || !Next || Org->Kind!=EGuildKind::PlayerOrganization || Org->Empire!=Actor.Empire || Org->Leader!=Actor.Pilot || Target==Actor.Pilot ||
        !HasRole(*Org,Actor.Pilot,EGuildRole::Leader) || Next->Empire!=Org->Empire)return Reject(Reason,INVTEXT("Only the current leader can transfer leadership to an existing member."));
    Org->Members.FindChecked(Actor.Pilot).Roles=MemberRole|DirectorRole;Next->Roles=AllRoles;Org->Leader=Target;Reason=INVTEXT("Leadership transferred.");return true;
}
bool GuildAI::ClaimStarterShip(FGuildState& State,const FGuildDefinition& Definition,const FGuildPilotContext& Pilot,FName& Ship,FText& Reason)
{
    Ship=NAME_None;if(Definition.Kind!=EGuildKind::Career || !Eligible(Definition,Pilot,Reason))return false;
    const FName Key=Entitlement(Pilot.Pilot,Definition.Name);const auto* Pending=State.PendingStarterShips.Find(Key);
    bool Member=false;for(const auto& Pair:State.Organizations)if(Matches(&Pair.Value,Definition) && Pair.Value.Empire==Pilot.Empire && Pair.Value.Members.Contains(Pilot.Pilot)){Member=true;break;}
    if(!Member || !Pending || Pending->IsNone() || *Pending!=Definition.StarterShip || StarterClaimed(State,Pilot.Pilot))return Reject(Reason,INVTEXT("No unclaimed career starter ship is available."));
    Ship=*Pending;State.PendingStarterShips.Remove(Key);State.ClaimedStarterShips.Add(Key);State.ClaimedStarterShips.Add(StarterClaimKey(Pilot.Pilot));Reason=INVTEXT("Starter ship entitlement claimed.");return true;
}
bool GuildAI::CancelRequest(FGuildState& State,const FGuildPilotContext& Pilot,FGuid Id,bool Invitation,FText& Reason)
{
    if(!Trusted(Pilot,Reason))return false;
    if(!Invitation)
    {
        auto* Request=State.Applications.Find(Id);
        if(!Request || Request->Pilot!=Pilot.Pilot || Request->State!=EGuildRequestState::Pending)return Reject(Reason,INVTEXT("Only your own pending application can be withdrawn."));
        Request->State=EGuildRequestState::Withdrawn;Reason=INVTEXT("Application withdrawn.");return true;
    }
    auto* Request=State.Invitations.Find(Id);auto* Org=Request?State.Organizations.Find(Request->Organization):nullptr;
    if(!Request || Request->State!=EGuildRequestState::Pending)return Reject(Reason,INVTEXT("This invitation is no longer pending."));
    if(Request->Pilot==Pilot.Pilot){Request->State=EGuildRequestState::Rejected;Reason=INVTEXT("Invitation declined.");return true;}
    if(!Recruiter(Org,Pilot,Reason))return false;
    Request->State=EGuildRequestState::Withdrawn;Reason=INVTEXT("Invitation withdrawn.");return true;
}
