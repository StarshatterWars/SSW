#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "PlayerFlightAccess.h"
#include "FreeFlightAssignment.h"
#include "JsonObjectConverter.h"
#include "Misc/FileHelper.h"
#include "Mission.h"
#include "MissionElement.h"
#include "StarshatterShipDesignSubsystem.h"
#include "Engine/DataTable.h"
#include "StarshatterGameDataSubsystem.h"
#include "SSWGameInstance.h"
#include "Misc/Paths.h"
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FFreeFlightRankAccessTest,"Starshatter.Agents.UnassignedFlightRequiresEnsign",
    EAutomationTestFlags_ApplicationContextMask|EAutomationTestFlags::EngineFilter)
bool FFreeFlightRankAccessTest::RunTest(const FString&)
{
    TestFalse(TEXT("Cadets cannot launch without an assignment"),PlayerFlightAccess::CanLaunch(false,0));
    TestFalse(TEXT("Ensigns still require an agent assignment"),PlayerFlightAccess::CanLaunch(false,1));
    TestFalse(TEXT("Senior officers also require departure clearance"),PlayerFlightAccess::CanLaunch(false,6));
    TestTrue(TEXT("Assigned training sorties remain available to cadets"),PlayerFlightAccess::CanLaunch(true,0));
    TestFalse(TEXT("Invalid rank cannot unlock unassigned flight"),PlayerFlightAccess::CanLaunch(false,-1));
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FFreeFlightClearanceTest,"Starshatter.Mission.FreeFlightAgentClearance",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FFreeFlightClearanceTest::RunTest(const FString&)
{
    TestFalse(TEXT("Free Flight remains unavailable in training campaigns"),FreeFlightAssignment::Unlocked(UAwardInfoRegistry::RankFromName(TEXT("Ensign")),true));
    FS_PlayerGameInfo Graduate;Graduate.Campaign=1;
    for(int32 Lesson=101;Lesson<=103;++Lesson)Graduate.MarkMissionCompleted(1,Lesson);
    TestFalse(TEXT("Ensign promotion after T3 does not skip the final lesson"),FreeFlightAssignment::Unlocked(UAwardInfoRegistry::RankFromName(TEXT("Ensign")),Graduate));
    Graduate.MarkMissionCompleted(1,104);
    TestTrue(TEXT("Graduates receive Free Flight before accepting the next campaign"),FreeFlightAssignment::Unlocked(UAwardInfoRegistry::RankFromName(TEXT("Ensign")),Graduate));
    TestFalse(TEXT("Graduation never removes the Ensign rank gate"),FreeFlightAssignment::Unlocked(0,Graduate));
    AgentAI Journal;const FName Pilot(TEXT("Sortie fixture"));const int32 Id=FreeFlightAssignment::Id;
    TestFalse(TEXT("Cadets cannot request independent flight"),FreeFlightAssignment::Unlocked(0));
    TestTrue(TEXT("Ensigns may request a flight assignment"),FreeFlightAssignment::Unlocked(UAwardInfoRegistry::RankFromName(TEXT("Ensign"))));
    TestFalse(TEXT("Docked idle session is not departure clearance"),FreeFlightAssignment::IsStarted(Journal,Pilot));
    const TArray<int32> Pool={Id};
    for(int32 Sortie=0;Sortie<2;++Sortie)
    {
        TestNotNull(TEXT("Agent offers a repeatable free flight"),Journal.Request(Pilot,Pool,TEXT("FlightOperations"),true));
        TestTrue(TEXT("Pilot accepts clearance"),Journal.Accept(Pilot,Id));
        TestFalse(TEXT("Acceptance alone does not skip Start Mission"),FreeFlightAssignment::IsStarted(Journal,Pilot));
        TestTrue(TEXT("Start Mission authorizes the active craft"),Journal.Start(Pilot,Id));
        TestTrue(TEXT("Authorized sortie can launch"),FreeFlightAssignment::IsStarted(Journal,Pilot));
        TestTrue(TEXT("Departure consumes the one-sortie clearance"),Journal.Resolve(Pilot,Id,true));
        TestFalse(TEXT("Landing requires a new clearance"),FreeFlightAssignment::IsStarted(Journal,Pilot));
        TestFalse(TEXT("Free Flight does not become a one-time story completion"),Journal.IsCompleted(Pilot,Id));
    }
    FS_CampaignMission Row;FString Json;
    TestTrue(TEXT("Flight clearance briefing ingests from JSON"),FFileHelper::LoadFileToString(Json,*(FPaths::ProjectContentDir()/TEXT("GameData/Agents/free_flight.json"))) && FJsonObjectConverter::JsonObjectStringToUStruct(Json,&Row,0,0));
    Mission Preview(Row.Id);
    Preview.SetName(TCHAR_TO_UTF8(*Row.MissionName));
    TestEqual(TEXT("Clearance briefing needs no fabricated mission ships"),FString(UTF8_TO_TCHAR(Preview.GetName())),Row.MissionName);
    auto* Ships=NewObject<UDataTable>();
    if(TestTrue(TEXT("Updated native ship catalog ingests"),UStarshatterShipDesignSubsystem::ImportJsonCatalog(*Ships,FPaths::ProjectContentDir()/TEXT("GameData/Ships/ships.json"))))
    {
        const auto* Shuttle=Ships->FindRow<FShipDesign>(TEXT("CrewShuttle"),TEXT("Crew shuttle audit"),false);
        if(TestNotNull(TEXT("Crew shuttle is present in the master catalog"),Shuttle))
        {TestEqual(TEXT("Crew shuttle has one quantum drive"),Shuttle->Quantum.Num(),1);TestEqual(TEXT("Crew shuttle uses the temporary Zodiac model"),Shuttle->Model,FString(TEXT("Zodiac")));}
    }
    TestEqual(TEXT("Clearance has a distinct assignment ID"),Row.Id,Id);
    return true;
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FFreeFlightEntrySpawnTest,"Starshatter.Mission.CareerEntryPlayerIsNotSquadron",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FFreeFlightEntrySpawnTest::RunTest(const FString&)
{
    auto* Context=NewObject<USSWGameInstance>();
    auto* Data=NewObject<UStarshatterGameDataSubsystem>(Context);
    for (const TCHAR* Scene : {TEXT("FreeFlight.json"), TEXT("FreeFlightSurface.json"), TEXT("FreeFlightStation.json")})
    {
        FS_CampaignMission Row;
        if (!TestTrue(Scene,Data->ReadLooseMission(FPaths::ProjectContentDir()/TEXT("GameData/Campaigns/01/Scenes")/Scene,Row))) continue;
        const auto* PlayerEntry=Row.Element.FindByPredicate([](const FS_MissionElement& Entry){return Entry.Player>0;});
        if (!TestNotNull(TEXT("Career entry has a player"),PlayerEntry)) continue;
        MissionElement Element;
        Element.SetCarrier(TCHAR_TO_UTF8(*PlayerEntry->Carrier));
        Element.SetSquadron(TCHAR_TO_UTF8(*PlayerEntry->Squadron));
        TestFalse(TEXT("Player is spawned as a fighter, not hangar inventory"),Element.IsSquadron());
        const auto* Reserve=Row.Element.FindByPredicate([&](const FS_MissionElement& Entry){return Entry.Name==PlayerEntry->Squadron;});
        if(TestNotNull(TEXT("Player references an authored reserve squadron"),Reserve))
            TestFalse(TEXT("Reserve squadron identifies the dock"),Reserve->Carrier.IsEmpty());
    }
    return true;
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FJanekDeckDesignTest,"Starshatter.Mission.JanekUsesMatchingRunwayDesign",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FJanekDeckDesignTest::RunTest(const FString&)
{
    auto* Context=NewObject<USSWGameInstance>();auto* Data=NewObject<UStarshatterGameDataSubsystem>(Context);
    auto* Designs=NewObject<UDataTable>();
    if(!TestTrue(TEXT("Native ship JSON loads"),UStarshatterShipDesignSubsystem::ImportJsonCatalog(*Designs,FPaths::ProjectContentDir()/TEXT("GameData/Ships/ships.json"))))return false;
    const auto* Base=Designs->FindRow<FShipDesign>(TEXT("JanekStation"),TEXT("Janek deck"),false);
    if(!TestNotNull(TEXT("Janek has its own native design"),Base))return false;
    TestEqual(TEXT("Janek retains its distinct runtime design identity"),Base->ShipName,FString(TEXT("JanekStation")));
    TestEqual(TEXT("Janek uses the runway complex model"),Base->Model,FString(TEXT("Base1")));
    TestTrue(TEXT("Janek launch deck exists"),Base->FlightDeck.Num()>0 && Base->FlightDeck[0].Slots.Num()>0);
    if(Base->FlightDeck.Num()>0 && Base->FlightDeck[0].Slots.Num()>0)
        TestEqual(TEXT("Janek deck height is 16, not the unrelated Telmera height 4"),double(Base->FlightDeck[0].Slots[0].Location.Y),16.0);
    for(const TCHAR* Scene:{TEXT("FreeFlightSurface.json"),TEXT("T3.json"),TEXT("T4.json")})
    {
        FS_CampaignMission Row;
        if(!TestTrue(Scene,Data->ReadLooseMission(FPaths::ProjectContentDir()/TEXT("GameData/Campaigns/01/Scenes")/Scene,Row)))continue;
        const auto* Station=Row.Element.FindByPredicate([](const FS_MissionElement& Entry){return Entry.Name==TEXT("Janek Station");});
        if(TestNotNull(TEXT("Surface scene contains Janek Station"),Station))
            TestEqual(TEXT("The mission and visual use the same Janek deck definition"),Station->Design,FString(TEXT("JanekStation")));
    }
    return true;
}
#endif

