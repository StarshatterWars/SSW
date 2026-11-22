#if WITH_DEV_AUTOMATION_TESTS && WITH_EDITOR
#include "Misc/AutomationTest.h"
#include "ScopedTestWorldContext.h"
#include "Misc/ScopeExit.h"
#include "Misc/Paths.h"
#include "PlanetActor.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/World.h"
#include "Engine/GameInstance.h"
#include "Engine/DataTable.h"
#include "StarshatterGameDataSubsystem.h"
#include "StarshatterShipDesignSubsystem.h"
#include "Ship.h"
#include "MissionElement.h"
#include "ShipDesign.h"
#include "FlightDeck.h"
#include "DataLoader.h"
#include <limits>

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPlanetVisualPhaseTest,"Starshatter.Campaign.Cinematics.VisualRotationPreservesAnchors",
    EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FPlanetVisualPhaseTest::RunTest(const FString&)
{
    const auto Options=UWorld::InitializationValues().AllowAudioPlayback(false).CreatePhysicsScene(false)
        .CreateNavigation(false).CreateAISystem(false).ShouldSimulatePhysics(false);
    auto* World=UWorld::CreateWorld(EWorldType::EditorPreview,false,NAME_None,nullptr,true,ERHIFeatureLevel::Num,&Options);
    FScopedSSWTestWorldContext TestWorldContext1(World);
    if(!TestNotNull(TEXT("Fixture world"),World))return false;
    ON_SCOPE_EXIT {World->DestroyWorld(false);};
    auto* Planet=World->SpawnActor<APlanetActor>();
    if(!TestNotNull(TEXT("Native planet"),Planet))return false;
    Planet->SetActorLocationAndRotation(FVector(100,200,300),FRotator(5,8,11));
    auto* Mesh=Planet->GetPlanetMeshComponent();Mesh->SetRelativeRotation(FRotator(12,17,5));
    const auto RootBefore=Planet->GetActorTransform(),AnchorBefore=Planet->RegionAnchor->GetComponentTransform();
    const FQuat MeshBefore=Mesh->GetRelativeRotation().Quaternion();
    Planet->SetAxialRotationPhaseRadians(PI/2);
    TestFalse(TEXT("Visible sphere rotates"),MeshBefore.Equals(Mesh->GetRelativeRotation().Quaternion(),.0001));
    TestTrue(TEXT("Actor physics frame does not rotate"),RootBefore.Equals(Planet->GetActorTransform(),.0001));
    TestTrue(TEXT("Region and station anchor stays fixed"),AnchorBefore.Equals(Planet->RegionAnchor->GetComponentTransform(),.0001));
    const auto First=Mesh->GetRelativeTransform();
    Planet->SetAxialRotationPhaseRadians(PI/2);
    TestTrue(TEXT("Repeated simulation timestamp is idempotent"),First.Equals(Mesh->GetRelativeTransform(),.0001));
    Planet->SetAxialRotationPhaseRadians(PI/2+2*PI);
    TestTrue(TEXT("Full period wraps without accumulated drift"),First.Equals(Mesh->GetRelativeTransform(),.0001));
    Planet->SetAxialRotationPhaseRadians(std::numeric_limits<double>::quiet_NaN());
    TestTrue(TEXT("Invalid clock phase cannot corrupt transform"),First.Equals(Mesh->GetRelativeTransform(),.0001));
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCinematicAuthoredContinuityTest,"Starshatter.Campaign.Cinematics.AuthoredNewsAndRecovery",
    EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FCinematicAuthoredContinuityTest::RunTest(const FString&)
{
    auto* Instance=NewObject<UGameInstance>();auto* Data=NewObject<UStarshatterGameDataSubsystem>(Instance);
    FS_CampaignMission Intro,News;
    if(!TestTrue(TEXT("Intro parses through normal mission ingestion"),Data->ReadLooseMission(FPaths::ProjectContentDir()/TEXT("GameData/Campaigns/01/Scenes/Intro.def"),Intro)) ||
       !TestTrue(TEXT("News parses through normal mission ingestion"),Data->ReadLooseMission(FPaths::ProjectContentDir()/TEXT("GameData/Campaigns/02/Scenes/01-News-Start.def"),News)))return false;
    const auto* Proteus=Intro.Element.FindByPredicate([](const auto& Item){return Item.Name==TEXT("Proteus");});
    if(TestNotNull(TEXT("Intro shuttle retained"),Proteus))
    {
        TestTrue(TEXT("Airborne shuttle cannot be declared carrier inventory"),Proteus->Carrier.IsEmpty());
        TestEqual(TEXT("Airborne shuttle retains its home squadron"),Proteus->Squadron,FString(TEXT("Lifters")));
        MissionElement Flying;Flying.SetCarrier(TCHAR_TO_ANSI(*Proteus->Carrier));Flying.SetSquadron(TCHAR_TO_ANSI(*Proteus->Squadron));
        TestFalse(TEXT("Actual spawn classifier creates a flying shuttle"),Flying.IsSquadron());
    }
    for(const auto& Event:Intro.Event)
        if(Event.EventType==MISSIONEVENT_TYPE::JUMP || Event.EventType==MISSIONEVENT_TYPE::END_SCENE)
        {
            TestEqual(TEXT("Departure and completion require real docking"),Event.EventTrigger,MISSIONEVENT_TRIGGER::TRIGGER_DOCK);
            TestEqual(TEXT("Departure tracks the actual shuttle"),Event.TriggerShip,FString(TEXT("Proteus")));
        }
    TestTrue(TEXT("Observatory shot exists when its first title appears"),News.Event.ContainsByPredicate([](const auto& Event)
        {return Event.EventType==MISSIONEVENT_TYPE::CAMERA && FMath::IsNearlyEqual(Event.EventTime,52.55,.001) && Event.EventTarget==TEXT("Tal Vesta Stellar Observatory");}));
    for(double Time:{60.50,68.50,72.55,85.50})
        TestTrue(FString::Printf(TEXT("New shot at %.2f replaces stale location title"),Time),News.Event.ContainsByPredicate([Time](const auto& Event)
            {return Event.EventType==MISSIONEVENT_TYPE::DISPLAY && FMath::IsNearlyEqual(Event.EventTime,Time,.001) && !Event.EventMessage.IsEmpty();}));
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FAlliancePlumeSizingTest,"Starshatter.Ships.Alliance.MainPlumeVisualScale",
    EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FAlliancePlumeSizingTest::RunTest(const FString&)
{
    auto* Table=NewObject<UDataTable>();
    if(!TestTrue(TEXT("Native JSON ship catalog imports"),UStarshatterShipDesignSubsystem::ImportJsonCatalog(*Table,FPaths::ProjectContentDir()/TEXT("GameData/Ships/ships.json"))))return false;
    for(const TCHAR* Name:{TEXT("Asher"),TEXT("Spectre")})
    {
        const auto* Row=Table->FindRow<FShipDesign>(FName(Name),TEXT("Plume regression"));
        if(!TestNotNull(TEXT("Alliance design"),Row))continue;
        TestEqual(TEXT("Four main engine ports retained"),Row->Drive.Num()?Row->Drive[0].Ports.Num():0,4);
        if(Row->Drive.IsEmpty())continue;
        TestEqual(TEXT("Legacy drive scale unchanged"),double(Row->Drive[0].DriveScale),4.0);
        for(const auto& Port:Row->Drive[0].Ports)
        {
            TestTrue(TEXT("Main flare uses bounded proven carrier visual scale"),FMath::IsNearlyEqual(double(Port.FlareScale),1.15,.0001));
            TestTrue(TEXT("Main trail uses bounded proven carrier visual scale"),FMath::IsNearlyEqual(double(Port.TrailScale),1.15,.0001));
        }
    }
    return true;
}
// Exercises the real clearance/capture/docking state machine. The complete
// cinematic intercept remains a separate playback check, rather than faking an
// approach here or forcing DOCKED to make the test pass.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCinematicShuttleDockLifecycleTest,"Starshatter.Campaign.Cinematics.ShuttleDockLifecycle",
    EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FCinematicShuttleDockLifecycleTest::RunTest(const FString&)
{
    const bool OwnLoader=DataLoader::GetLoader()==nullptr;if(OwnLoader)DataLoader::Initialize();
    ON_SCOPE_EXIT {if(OwnLoader)DataLoader::Close();};
    ShipDesign CarrierDesign,ShuttleDesign;
    CarrierDesign.type=static_cast<int>(CLASSIFICATION::CARRIER);
    ShuttleDesign.type=static_cast<int>(CLASSIFICATION::LCA);
    CarrierDesign.integrity=1000;ShuttleDesign.integrity=1000;
    Ship Carrier("Archon","",&CarrierDesign,1,0,nullptr,false);
    Ship Shuttle("Proteus","",&ShuttleDesign,1,0,nullptr,false);
    FlightDeck Deck;Deck.SetRecoveryDeck();Deck.SetCarrier(&Carrier);
    Deck.AddSlot(FVector::ZeroVector,static_cast<uint32>(CLASSIFICATION::LCA));
    auto* Inbound=new InboundSlot(&Shuttle,&Deck,0,1);Deck.Inbound(Inbound);
    Inbound->Clear(false);
    TestFalse(TEXT("Shuttle cannot hard dock without landing clearance"),Deck.Dock(&Shuttle));
    Inbound->Clear();
    if(!TestTrue(TEXT("Cleared shuttle is captured through normal deck docking"),Deck.Dock(&Shuttle)))return false;
    TestEqual(TEXT("Capture starts physical docking sequence"),Shuttle.GetFlightPhase(),EOPSMode::DOCKING);
    TestTrue(TEXT("Capture binds actual carrier"),Shuttle.GetCarrier()==&Carrier);
    Deck.ExecFrame(6);Deck.ExecFrame(.01);
    TestEqual(TEXT("Deck completes real hard docking without forced state"),Shuttle.GetFlightPhase(),EOPSMode::DOCKED);
    return true;
}
#endif
