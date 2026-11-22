#if WITH_DEV_AUTOMATION_TESTS && WITH_EDITOR
#include "SurfaceFlightRoute.h"
#include "Misc/AutomationTest.h"
#include "StarshatterGameDataSubsystem.h"
#include "Engine/GameInstance.h"
#include "SimRegion.h"
#include "Misc/Paths.h"
#include <limits>

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FSurfaceFlightRouteTest, "Starshatter.Travel.SurfaceFlightRouteClearance",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FSurfaceFlightRouteTest::RunTest(const FString&)
{
    const FVector Start(0,0,24), End(10000,0,7000);
    TestEqual(TEXT("An intervening mountain raises the simulation route"),
        SurfaceFlightRoute::ClearedAltitude(Start,End,[](double X,double){return X>=4500 && X<=5500 ? 9000.0 : -2.0;}),10500.0);
    TestEqual(TEXT("An already clear authored altitude is retained"),
        SurfaceFlightRoute::ClearedAltitude(Start,End,[](double,double){return -2.0;}),7000.0);
    TestEqual(TEXT("A waypoint below the surface is raised above ground"),
        SurfaceFlightRoute::ClearedAltitude(Start,FVector(0,0,100),[](double,double){return 1200.0;}),2700.0);
    TestEqual(TEXT("Invalid height samples cannot corrupt the route"),
        SurfaceFlightRoute::ClearedAltitude(Start,End,[](double,double){return std::numeric_limits<double>::quiet_NaN();}),7000.0);
    TestEqual(TEXT("A mountain ahead triggers recovery before current AGL is low"),
        SurfaceFlightRoute::RecoveryAltitude(FVector(0,3000,0),FVector(500,0,0),
            [](double X,double){return X>=2000 ? 4000.0 : 0.0;}),5500.0);
    TestEqual(TEXT("Rapid descent triggers recovery before impact"),
        SurfaceFlightRoute::RecoveryAltitude(FVector(0,3000,0),FVector(300,-400,0),
            [](double,double){return 0.0;}),4700.0);
    TestEqual(TEXT("Clear level flight keeps authored altitude"),
        SurfaceFlightRoute::RecoveryAltitude(FVector(0,3000,0),FVector(300,0,0),
            [](double,double){return 0.0;}),3000.0);
    return true;
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FT4SurfaceRouteTest,"Starshatter.Travel.T4UnresolvedSurfaceRoute",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FT4SurfaceRouteTest::RunTest(const FString&)
{
    auto* Data=NewObject<UStarshatterGameDataSubsystem>(NewObject<UGameInstance>());
    FS_CampaignMission Mission;
    if(!TestTrue(TEXT("Actual T4 JSON loads"),Data->ReadLooseMission(FPaths::ProjectContentDir()/TEXT("GameData/Campaigns/01/Scenes/T4.json"),Mission)))return false;
    const FS_MissionElement* Player=Mission.Element.FindByPredicate([](const auto& E){return E.Player==1;});
    if(!TestNotNull(TEXT("T4 player route"),Player))return false;
    TestEqual(TEXT("T4 retains three flight waypoints"),Player->Navpoint.Num(),3);
    SimRegion Region(nullptr,"Janek Station",SimRegion::REAL_SPACE);
    FVector Previous(25000,0,24);
    int32 Prepared=0;
    for(const auto& Source:Player->Navpoint)
    {
        Instruction Point(TCHAR_TO_UTF8(*Source.OrderRegionName),Source.ObjectiveLocation,Source.Action);
        TestNull(TEXT("Fixture reproduces unresolved local mission region"),Point.GetRegion());
        const bool Ready=SurfaceFlightRoute::PrepareWaypoint(Point,&Region,Region.GetName(),Previous,
            [](double,double){return 20000.0;});
        TestTrue(TEXT("Every T4 waypoint, including the first, Strike and RTB, gets clearance"),Ready);
        Prepared+=Ready;
        TestTrue(TEXT("Simulation instruction is raised above terrain"),Point.GetLocation().Z>=21500.0);
        TestTrue(TEXT("Waypoint binds to the local surface region"),Point.GetRegion()==&Region);
    }
    TestEqual(TEXT("No T4 route points silently skipped"),Prepared,3);
    Instruction Landing("",FVector(0,0,24),EInstruction::Dock);
    TestFalse(TEXT("ILS landing altitude is not raised to cruise height"),SurfaceFlightRoute::PrepareWaypoint(Landing,&Region,Region.GetName(),Previous,[](double,double){return 20000.0;}));
    TestEqual(TEXT("Landing controller retains deck altitude"),Landing.GetLocation().Z,24.0);
    Instruction Other("Different sector",FVector(0,0,100),EInstruction::Vector);
    TestFalse(TEXT("Remote waypoints are not moved onto Janek terrain"),SurfaceFlightRoute::PrepareWaypoint(Other,&Region,Region.GetName(),Previous,[](double,double){return 20000.0;}));
    return true;
}
#endif
