#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "GuildAI.h"
#include "JsonObjectConverter.h"
#include "PlayerSaveGame.h"
#include "Kismet/GameplayStatics.h"
#include "GuildDirectorySaveGame.h"
#include "GuildCatalogSubsystem.h"
#include "Engine/GameInstance.h"

namespace
{
FGuildPilotContext GuildPilot(const TCHAR* Name,const TCHAR* Empire=TEXT("Terellian"))
{
    FGuildPilotContext Result;Result.Pilot=Name;Result.Empire=Empire;Result.Rank=3;Result.bTrustedAuthority=true;Result.Skills.Add(TEXT("fighter_operations"));return Result;
}
FGuildDefinition CareerGuild()
{
    FGuildDefinition Result;Result.Name=TEXT("AlliancePilots");Result.DisplayName=TEXT("Alliance Pilots Guild");Result.Empire=TEXT("Terellian");
    Result.MinimumRank=1;Result.RequiredSkills.Add(TEXT("fighter_operations"));Result.StarterShip=TEXT("Falcon");return Result;
}
FGuildDefinition OrganizationGuild()
{
    FGuildDefinition Result;Result.Name=TEXT("PlayerOrganization");Result.DisplayName=TEXT("Player Organization");Result.Kind=EGuildKind::PlayerOrganization;return Result;
}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGuildCareerEntitlementTest,"Starshatter.Guilds.CareerGatesAndOnceOnlyStarterShip",EAutomationTestFlags_ApplicationContextMask|EAutomationTestFlags::EngineFilter)
bool FGuildCareerEntitlementTest::RunTest(const FString&)
{
    FGuildState State;FText Reason;const auto Definition=CareerGuild();auto Pilot=GuildPilot(TEXT("Alice"));
    Pilot.bTrustedAuthority=false;TestFalse(TEXT("Untrusted enrollment cannot mutate membership"),GuildAI::EnrollCareer(State,Definition,Pilot,100,Reason).IsValid());
    Pilot.bTrustedAuthority=true;Pilot.Rank=0;TestFalse(TEXT("Rank gate enforced"),GuildAI::EnrollCareer(State,Definition,Pilot,100,Reason).IsValid());
    Pilot.Rank=3;Pilot.Skills.Reset();TestFalse(TEXT("Skill gate enforced"),GuildAI::EnrollCareer(State,Definition,Pilot,100,Reason).IsValid());
    Pilot.Skills.Add(TEXT("fighter_operations"));auto Enemy=GuildPilot(TEXT("Enemy"),TEXT("Marakan"));
    TestFalse(TEXT("Hard empire boundary enforced"),GuildAI::EnrollCareer(State,Definition,Enemy,100,Reason).IsValid());
    TestTrue(TEXT("Failed enrollments leave registry empty"),State.Organizations.IsEmpty());
    const FGuid Membership=GuildAI::EnrollCareer(State,Definition,Pilot,100,Reason);
    if(!TestTrue(TEXT("Qualified career enrollment succeeds"),Membership.IsValid()))return false;
    TestEqual(TEXT("Repeat enrollment returns existing membership"),GuildAI::EnrollCareer(State,Definition,Pilot,101,Reason),Membership);
    TestEqual(TEXT("Only one starter entitlement exists"),State.PendingStarterShips.Num(),1);
    FName Ship;const auto Before=State;
    TestTrue(TEXT("Starter entitlement can be consumed for hangar transaction"),GuildAI::ClaimStarterShip(State,Definition,Pilot,Ship,Reason));
    TestEqual(TEXT("Authored starter design returned"),Ship,FName(TEXT("Falcon")));
    TestFalse(TEXT("Starter cannot be claimed twice"),GuildAI::ClaimStarterShip(State,Definition,Pilot,Ship,Reason));
    State=Before;TestTrue(TEXT("Failed outer save can restore entitlement with its state snapshot"),GuildAI::ClaimStarterShip(State,Definition,Pilot,Ship,Reason));
    TestTrue(TEXT("Member can leave career guild"),GuildAI::Leave(State,Pilot,Membership,Reason));
    TestTrue(TEXT("Member can reenroll"),GuildAI::EnrollCareer(State,Definition,Pilot,102,Reason).IsValid());
    TestFalse(TEXT("Leaving and reenrolling cannot farm starter ships"),GuildAI::ClaimStarterShip(State,Definition,Pilot,Ship,Reason));
    auto Mining=Definition;Mining.Name=TEXT("AllianceMiners");Mining.Career=EGuildCareer::Miner;Mining.StarterShip=TEXT("Prospector");
    TestTrue(TEXT("Pilot may join a different career profession"),GuildAI::EnrollCareer(State,Mining,Pilot,103,Reason).IsValid());
    TestFalse(TEXT("Changing career cannot grant a second free hull"),GuildAI::ClaimStarterShip(State,Mining,Pilot,Ship,Reason));
    State.LocalPilot=FGuid::NewGuid();auto* Save=NewObject<UPlayerSaveGame>();Save->PlayerInfo.Guilds=State;TArray<uint8> Bytes;
    if(!TestTrue(TEXT("Guild state serializes with actual player profile"),UGameplayStatics::SaveGameToMemory(Save,Bytes)))return false;
    const auto* Saved=Cast<UPlayerSaveGame>(UGameplayStatics::LoadGameFromMemory(Bytes));if(!TestNotNull(TEXT("Player guild state reloads"),Saved))return false;
    FGuildState Loaded=Saved->PlayerInfo.Guilds;
    TestEqual(TEXT("Stable local guild identity survives reload"),Loaded.LocalPilot,State.LocalPilot);
    TestFalse(TEXT("Once-only starter claim survives reload"),GuildAI::ClaimStarterShip(Loaded,Definition,Pilot,Ship,Reason));
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGuildApplicationLeadershipTest,"Starshatter.Guilds.ApplicationAuthorityAndLeadershipTransfer",EAutomationTestFlags_ApplicationContextMask|EAutomationTestFlags::EngineFilter)
bool FGuildApplicationLeadershipTest::RunTest(const FString&)
{
    FGuildState State;FText Reason;const auto Definition=OrganizationGuild();auto Alice=GuildPilot(TEXT("Alice"));auto Bob=GuildPilot(TEXT("Bob"));auto Enemy=GuildPilot(TEXT("Enemy"),TEXT("Marakan"));
    const FGuid Org=GuildAI::FoundOrganization(State,Definition,Alice,TEXT("Janus Prospectors"),100,Reason);
    if(!TestTrue(TEXT("Trusted founder creates local organization"),Org.IsValid()))return false;
    TestFalse(TEXT("Founder cannot create a second player organization"),GuildAI::FoundOrganization(State,Definition,Alice,TEXT("Second"),101,Reason).IsValid());
    TestFalse(TEXT("Enemy cannot apply to another empire's organization"),GuildAI::Apply(State,Definition,Org,Enemy,TEXT(""),101,Reason).IsValid());
    const FGuid Application=GuildAI::Apply(State,Definition,Org,Bob,TEXT("Ready to haul supplies."),101,Reason);
    if(!TestTrue(TEXT("Eligible pilot can apply"),Application.IsValid()))return false;
    TestFalse(TEXT("Duplicate pending application rejected"),GuildAI::Apply(State,Definition,Org,Bob,TEXT("Again"),102,Reason).IsValid());
    TestFalse(TEXT("Applicant cannot approve own application"),GuildAI::ReviewApplication(State,Definition,Bob,Bob,Application,true,102,Reason));
    TestTrue(TEXT("Authorized leader accepts eligible applicant"),GuildAI::ReviewApplication(State,Definition,Alice,Bob,Application,true,102,Reason));
    TestFalse(TEXT("Accepted application cannot replay"),GuildAI::ReviewApplication(State,Definition,Alice,Bob,Application,true,103,Reason));
    TestFalse(TEXT("Leader must transfer command before leaving members"),GuildAI::Leave(State,Alice,Org,Reason));
    TestFalse(TEXT("Member cannot promote self"),GuildAI::SetMemberRoles(State,Bob,Org,Bob.Pilot,63,Reason));
    TestTrue(TEXT("Leader can transfer to existing member"),GuildAI::TransferLeadership(State,Alice,Org,Bob.Pilot,Reason));
    TestEqual(TEXT("Current leader changed"),State.Organizations.FindChecked(Org).Leader,Bob.Pilot);
    TestFalse(TEXT("Old leader cannot transfer command again"),GuildAI::TransferLeadership(State,Alice,Org,Bob.Pilot,Reason));
    TestTrue(TEXT("Former leader can leave"),GuildAI::Leave(State,Alice,Org,Reason));
    TestTrue(TEXT("Sole remaining leader can disband"),GuildAI::Leave(State,Bob,Org,Reason));
    TestTrue(TEXT("Organization marked disbanded"),State.Organizations.FindChecked(Org).bDisbanded);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGuildInvitationPermissionsTest,"Starshatter.Guilds.InvitationExpiryAndRevokedRecruiter",EAutomationTestFlags_ApplicationContextMask|EAutomationTestFlags::EngineFilter)
bool FGuildInvitationPermissionsTest::RunTest(const FString&)
{
    FGuildState State;FText Reason;const auto Definition=OrganizationGuild();const auto Alice=GuildPilot(TEXT("Alice"));const auto Bob=GuildPilot(TEXT("Bob"));const auto Charlie=GuildPilot(TEXT("Charlie"));
    const FGuid Org=GuildAI::FoundOrganization(State,Definition,Alice,TEXT("Janus Flight"),100,Reason);
    const FGuid Invite=GuildAI::Invite(State,Definition,Org,Alice,Bob,101,200,Reason);
    if(!TestTrue(TEXT("Founder can invite same-empire pilot"),Invite.IsValid()))return false;
    TestFalse(TEXT("Invitation cannot be claimed by wrong identity"),GuildAI::AcceptInvitation(State,Definition,Charlie,Invite,102,Reason));
    TestFalse(TEXT("Expiry boundary enforced"),GuildAI::AcceptInvitation(State,Definition,Bob,Invite,200,Reason));
    TestTrue(TEXT("Unexpired invitation accepted once"),GuildAI::AcceptInvitation(State,Definition,Bob,Invite,102,Reason));
    TestFalse(TEXT("Invitation cannot replay"),GuildAI::AcceptInvitation(State,Definition,Bob,Invite,103,Reason));
    TestTrue(TEXT("Leader can appoint recruiter"),GuildAI::SetMemberRoles(State,Alice,Org,Bob.Pilot,static_cast<int32>(EGuildRole::Recruiter),Reason));
    const FGuid Next=GuildAI::Invite(State,Definition,Org,Bob,Charlie,104,300,Reason);
    if(!TestTrue(TEXT("Appointed recruiter can invite"),Next.IsValid()))return false;
    TestTrue(TEXT("Leader can revoke recruiter"),GuildAI::SetMemberRoles(State,Alice,Org,Bob.Pilot,static_cast<int32>(EGuildRole::Member),Reason));
    TestFalse(TEXT("Revoked invitation authority is revalidated at acceptance"),GuildAI::AcceptInvitation(State,Definition,Charlie,Next,105,Reason));
    TestFalse(TEXT("Unknown role bits rejected"),GuildAI::SetMemberRoles(State,Alice,Org,Bob.Pilot,1024,Reason));
    TestFalse(TEXT("Director cannot be installed by ordinary member"),GuildAI::SetMemberRoles(State,Bob,Org,Alice.Pilot,static_cast<int32>(EGuildRole::Director),Reason));
    return true;
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGuildSharedDirectoryTest,"Starshatter.Guilds.SharedLocalDirectoryAndRequestCancellation",EAutomationTestFlags_ApplicationContextMask|EAutomationTestFlags::EngineFilter)
bool FGuildSharedDirectoryTest::RunTest(const FString&)
{
    const auto Definition=OrganizationGuild();const auto Alice=GuildPilot(TEXT("RegisteredAlice"));const auto Bob=GuildPilot(TEXT("RegisteredBob"));const auto Outsider=GuildPilot(TEXT("Outsider"));FText Reason;
    auto* Directory=NewObject<UGuildDirectorySaveGame>();
    FGuildDirectoryPilot A;A.Identity=Alice.Pilot;A.DisplayName=TEXT("Alice");A.Empire=Alice.Empire;A.Rank=Alice.Rank;A.Skills=Alice.Skills;Directory->Pilots.Add(A.Identity,A);
    auto B=A;B.Identity=Bob.Pilot;B.DisplayName=TEXT("Bob");Directory->Pilots.Add(B.Identity,B);
    const FGuid Org=GuildAI::FoundOrganization(Directory->State,Definition,Alice,TEXT("Shared Local Charter"),100,Reason);
    const FGuid Invite=GuildAI::Invite(Directory->State,Definition,Org,Alice,Bob,101,200,Reason);
    if(!TestTrue(TEXT("Registered pilot invitation issued"),Invite.IsValid()))return false;
    TArray<uint8> Bytes;if(!TestTrue(TEXT("Shared directory serializes independent of a player wallet"),UGameplayStatics::SaveGameToMemory(Directory,Bytes)))return false;
    auto* Loaded=Cast<UGuildDirectorySaveGame>(UGameplayStatics::LoadGameFromMemory(Bytes));if(!TestNotNull(TEXT("Another local profile can load the canonical directory"),Loaded))return false;
    TestTrue(TEXT("Invite recipient accepts through the same shared organization state"),GuildAI::AcceptInvitation(Loaded->State,Definition,Bob,Invite,102,Reason));
    TestEqual(TEXT("Directory now contains both actual members"),Loaded->State.Organizations.FindChecked(Org).Members.Num(),2);
    auto Charlie=GuildPilot(TEXT("Charlie"));const FGuid Application=GuildAI::Apply(Loaded->State,Definition,Org,Charlie,TEXT("Request"),103,Reason);
    TestFalse(TEXT("Unrelated pilot cannot withdraw applicant's request"),GuildAI::CancelRequest(Loaded->State,Outsider,Application,false,Reason));
    TestTrue(TEXT("Applicant can withdraw pending request"),GuildAI::CancelRequest(Loaded->State,Charlie,Application,false,Reason));
    TestFalse(TEXT("Withdrawn application cannot later be approved"),GuildAI::ReviewApplication(Loaded->State,Definition,Alice,Charlie,Application,true,104,Reason));
    const FGuid NextInvite=GuildAI::Invite(Loaded->State,Definition,Org,Alice,Charlie,104,110,Reason);
    TestFalse(TEXT("Unrelated pilot cannot cancel invitation"),GuildAI::CancelRequest(Loaded->State,Outsider,NextInvite,true,Reason));
    TestTrue(TEXT("Invitation recipient can decline"),GuildAI::CancelRequest(Loaded->State,Charlie,NextInvite,true,Reason));
    TestFalse(TEXT("Declined invitation cannot be accepted"),GuildAI::AcceptInvitation(Loaded->State,Definition,Charlie,NextInvite,105,Reason));
    const FGuid FinalInvite=GuildAI::Invite(Loaded->State,Definition,Org,Alice,Charlie,106,120,Reason);
    TestTrue(TEXT("Authorized recruiter can withdraw issued invitation"),GuildAI::CancelRequest(Loaded->State,Alice,FinalInvite,true,Reason));
    TestFalse(TEXT("Withdrawn invitation cannot replay"),GuildAI::AcceptInvitation(Loaded->State,Definition,Charlie,FinalInvite,107,Reason));
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGuildCatalogTest,"Starshatter.Guilds.JsonCatalogAndOfflineMutationGate",EAutomationTestFlags_ApplicationContextMask|EAutomationTestFlags::EngineFilter)
bool FGuildCatalogTest::RunTest(const FString&)
{
    auto* Game=NewObject<UGameInstance>();auto* Catalog=NewObject<UGuildCatalogSubsystem>(Game);
    if(!TestTrue(TEXT("JSON-only guild catalog loads"),Catalog->LoadCatalog()))return false;
    TestEqual(TEXT("Seven careers and one charter for each of four empires"),Catalog->GetCatalog().Num(),32);
    for(const TCHAR* Empire:{TEXT("Terellian"),TEXT("Marakan"),TEXT("Dantari"),TEXT("Solus")})
    {
        int32 Career=0,Organization=0;
        for(const auto& Entry:Catalog->GetCatalog())if(Entry.Value.Empire==FName(Empire))
        {if(Entry.Value.Kind==EGuildKind::Career)++Career;else ++Organization;}
        TestEqual(TEXT("Each empire has seven career paths"),Career,7);TestEqual(TEXT("Each empire has an organization charter"),Organization,1);
    }
    FText Reason;TestFalse(TEXT("Catalog UI alone cannot found an organization outside docked offline play"),Catalog->CreateOrganization(TEXT("Unsafe mutation"),Reason));
    return true;
}
#endif
