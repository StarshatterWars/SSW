#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "AgentCatalogSubsystem.h"
#include "AgentMissionRules.h"
#include "CampaignPresentationRules.h"
#include "MissionInfo.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCampaignAgentOriginsTest, "Starshatter.Agents.CampaignOrigins",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FCampaignAgentOriginsTest::RunTest(const FString&)
{
    FString Json;
    if (!TestTrue(TEXT("Agent JSON readable"), FFileHelper::LoadFileToString(Json, *(FPaths::ProjectContentDir()/TEXT("GameData/Agents/agents.json"))))) return false;
    auto* Table = NewObject<UDataTable>(); Table->RowStruct = FS_AgentCatalogRow::StaticStruct();
    TestTrue(TEXT("Agent JSON imports cleanly"), Table->CreateTableFromJSONString(Json).IsEmpty());
    const TCHAR* Systems[] = { TEXT("Solus"),TEXT("Borova"),TEXT("Jarnell"),TEXT("Jarnell") };
    const TCHAR* Regions[] = { TEXT("Meridian"),TEXT("Borova"),TEXT("Adonis"),TEXT("Adonis") };
    for (int32 Id=2;Id<=5;++Id)
    {
        FS_Campaign Row; Row.System=Systems[Id-2]; Row.Region=Regions[Id-2];
        int32 Owners=0;
        for (const auto& Entry : Table->GetRowMap())
        {
            const auto& Agent=*reinterpret_cast<const FS_AgentCatalogRow*>(Entry.Value);
            if (!Agent.CampaignIds.Contains(Id)) continue;
            ++Owners;
            TestTrue(TEXT("Campaign officer stationed at authored origin"), UAgentCatalogSubsystem::IsCampaignOrigin(Agent,Row));
            TestTrue(TEXT("Campaign officer named"), !Agent.DisplayName.IsEmpty());
        }
        TestEqual(TEXT("One issuing agent per story campaign"),Owners,1);
        FS_AgentCatalogRow Wrong; Wrong.System=TEXT("Janus"); Wrong.Region=TEXT("Janek");
        TestFalse(TEXT("Janek cannot introduce foreign campaign"),UAgentCatalogSubsystem::IsCampaignOrigin(Wrong,Row));
    }
    return true;
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCampaignAgentOffersTest, "Starshatter.Agents.CampaignOfferGates",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FCampaignAgentOffersTest::RunTest(const FString&)
{
    FS_AgentCatalogRow Agent; Agent.CampaignIds={2}; Agent.MinimumRank=1;
    MissionInfo Info; Info.id=42; Info.DisplayType=EMissionDisplayType::PlayerMission;
    Info.start_after=10; Info.start_before=100; Info.min_rank=1; Info.max_rank=5; Info.exec_once=1; Info.region="Meridian";
    auto Offer=[&](int32 Campaign=2,double Time=50,int32 Rank=2,bool Completed=false,bool Action=true)
    {return UAgentCatalogSubsystem::CanOfferCampaignMission(Agent,Campaign,Info,Time,Rank,TEXT("Meridian"),Completed,Action);};
    Info.Source=EMissionSource::Scripted; TestTrue(TEXT("Story order retained in live list"),Offer());
    Info.Source=EMissionSource::Dynamic; TestTrue(TEXT("Generated order retained in same list"),Offer());
    TestFalse(TEXT("Foreign campaign refused"),Offer(3));
    TestFalse(TEXT("Not yet due refused"),Offer(2,9));
    TestFalse(TEXT("Expired refused"),Offer(2,101));
    TestFalse(TEXT("Rank gate retained"),Offer(2,50,0));
    TestFalse(TEXT("Max rank retained"),Offer(2,50,6));
    TestFalse(TEXT("Completion retained"),Offer(2,50,2,true));
    TestFalse(TEXT("Action prerequisite retained"),Offer(2,50,2,false,false));
    TestFalse(TEXT("Wrong group region refused"),UAgentCatalogSubsystem::CanOfferCampaignMission(Agent,2,Info,50,2,TEXT("Borova"),false,true));
    Info.is_cutscene=true; TestFalse(TEXT("Cutscene cannot be accepted as combat mission"),Offer()); Info.is_cutscene=false;
    Info.exec_once=-1; TestFalse(TEXT("Consumed mission refused"),Offer()); Info.exec_once=1;
    int32 Campaign=0,Original=0;
    TestTrue(TEXT("Journal encoding roundtrip"),AgentMissionRules::Decode(AgentMissionRules::Encode(2,42),Campaign,Original));
    TestEqual(TEXT("Original sim mission ID retained"),Original,42);
    FS_PlayerGameInfo Pilot;
    TestFalse(TEXT("Campaign progression gate remains"),CampaignPresentationRules::IsCampaignUnlocked(3,Pilot));
    return true;
}
#endif
