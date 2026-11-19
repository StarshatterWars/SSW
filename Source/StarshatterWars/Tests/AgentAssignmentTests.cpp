#if WITH_DEV_AUTOMATION_TESTS

#include "AgentAI.h"
#include "TrainingTestRules.h"
#include "AgentCatalogSubsystem.h"
#include "PlayerCharacter.h"
#include "Engine/GameInstance.h"
#include "Misc/ScopeExit.h"
#include "ShipDesignRegistry.h"
#include "StarshatterWeaponDesignSubsystem.h"
#include "WeaponDesignRegistry.h"
#include "Misc/AutomationTest.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Mission.h"
#include "MissionElement.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FAgentAssignmentTest, "Starshatter.Mission.AgentAssignment",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FAgentAssignmentTest::RunTest(const FString&)
{
    AgentAI Agent;
    const FName Pilot(TEXT("Cadet")), OtherPilot(TEXT("Another Cadet"));
    const int32 Eligible[] = {103, 101, 102};
    const auto* Offer = Agent.Request(Pilot, Eligible);
    if (!TestNotNull(TEXT("Eligible pilot receives an offer"), Offer)) return false;
    TestEqual(TEXT("Officer offers the first eligible lesson"), Offer->MissionId, 101);
    TestFalse(TEXT("A different pilot cannot accept this offer"), Agent.Accept(OtherPilot, 101));
    TestFalse(TEXT("An unaccepted offer cannot start"), Agent.Start(Pilot, 101));
    TestTrue(TEXT("Pilot accepts the offer"), Agent.Accept(Pilot, 101));
    TestFalse(TEXT("An unrelated mission does not start the assignment"), Agent.Start(Pilot, 102));
    TestTrue(TEXT("The simulation starts the assigned mission"), Agent.Start(Pilot, 101));
    Agent.Interrupt(Pilot, 101);
    TestTrue(TEXT("Interrupted mission can be relaunched"), Agent.Start(Pilot, 101));
    TestTrue(TEXT("Debrief resolves the assignment"), Agent.Resolve(Pilot, 101, true));
    TestFalse(TEXT("Duplicate debrief cannot resolve it again"), Agent.Resolve(Pilot, 101, true));
    TestTrue(TEXT("Completed training can be queued as a test replay"),Agent.QueueTestReplay(Pilot,101,TEXT("JanekFlightTraining")));
    TestTrue(TEXT("Replay queue is explicitly marked"),Agent.GetQueue(Pilot)[0].bTestReplay);
    TestFalse(TEXT("An occupied queue prevents a duplicate replay"),Agent.QueueTestReplay(Pilot,101,TEXT("JanekFlightTraining")));
    AgentAI RestoredReplay;
    TestTrue(TEXT("Test replay journal round trips"),RestoredReplay.RestoreJournal(Agent.SaveJournal()));
    TestTrue(TEXT("Restored replay retains no-reward marker"),RestoredReplay.GetQueue(Pilot)[0].bTestReplay);
    TestTrue(TEXT("Replay starts"),Agent.Start(Pilot,101));
    TestTrue(TEXT("Failed replay resolves without undoing previous success"),Agent.Resolve(Pilot,101,false));
    TestTrue(TEXT("Previous training completion is retained"),Agent.IsCompleted(Pilot,101));
    TestFalse(TEXT("Uncompleted lessons cannot be replayed"),Agent.QueueTestReplay(Pilot,102,TEXT("JanekFlightTraining")));
    const int32 Remaining[] = {102, 103};
    const auto* Next = Agent.Request(Pilot, Remaining);
    if (!TestNotNull(TEXT("Officer offers the next eligible lesson"), Next)) return false;
    TestEqual(TEXT("Completed lesson is left out of the next roster"), Next->MissionId, 102);
    TestTrue(TEXT("Pilot declines a pending offer"), Agent.Decline(Pilot, 102));
    const auto* Alternate = Agent.Request(Pilot, Remaining);
    if (!TestNotNull(TEXT("Officer can offer another lesson"), Alternate)) return false;
    TestEqual(TEXT("Declined lesson is skipped"), Alternate->MissionId, 103);
    TestTrue(TEXT("A second lesson can be queued without starting the first"), Agent.Accept(Pilot, 103));
    const int32 MoreEligible[] = {103, 104};
    const auto* FurtherOffer = Agent.Request(Pilot, MoreEligible);
    if (!TestNotNull(TEXT("An occupied queue still allows a separate offer"), FurtherOffer)) return false;
    TestEqual(TEXT("A queued lesson is not offered twice"), FurtherOffer->MissionId, 104);
    TestTrue(TEXT("Accept appends the new lesson"), Agent.Accept(Pilot, 104));
    TestEqual(TEXT("Two accepted lessons are visible in the queue"), Agent.GetQueue(Pilot).Num(), 2);
    TestFalse(TEXT("A later lesson cannot start ahead of the queue"), Agent.Start(Pilot, 104));
    TestTrue(TEXT("The head lesson starts"), Agent.Start(Pilot, 103));
    TestTrue(TEXT("Completing the head advances the queue"), Agent.Resolve(Pilot, 103, true));
    TestTrue(TEXT("The next lesson can now start"), Agent.Start(Pilot, 104));

    const FName Local(TEXT("Local")), Regional(TEXT("Regional"));
    const int32 LocalMissions[] = {105};
    const int32 RegionalMissions[] = {910008};
    TestNotNull(TEXT("Local offer is available"), Agent.Request(Pilot, LocalMissions, Local));
    TestNotNull(TEXT("Regional offer is available"), Agent.Request(Pilot, RegionalMissions, Regional));
    TestNotNull(TEXT("Requesting another agent preserves the first offer"), Agent.FindOffer(Pilot, Local));
    TestTrue(TEXT("Accepting the regional offer queues it"), Agent.Accept(Pilot, 910008));
    TestEqual(TEXT("The queue keeps the issuing agent"), Agent.GetQueue(Pilot).Last().Issuer, Regional);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FFreeFlightStartTest, "Starshatter.Mission.FreeFlightStart",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FFreeFlightStartTest::RunTest(const FString&)
{
    FString Definition;
    const FString Path = FPaths::ProjectContentDir() / TEXT("GameData/Campaigns/01/Scenes/FreeFlight.def");
    if (!TestTrue(TEXT("Free Mode has a staged mission definition"), FFileHelper::LoadFileToString(Definition, *Path)))
        return false;

    auto* Ships = LoadObject<UDataTable>(nullptr, TEXT("/Game/Game/DT_ShipDesign.DT_ShipDesign"));
    if (!TestNotNull(TEXT("Ship design table"), Ships)) return false;
    const bool bNeedsWeaponRegistry = WeaponDesignRegistry::GetAll().IsEmpty();
    const bool bNeedsShipRegistry = ShipDesignRegistry::GetAll().IsEmpty();
    ON_SCOPE_EXIT
    {
        if (bNeedsShipRegistry) ShipDesignRegistry::Clear();
        if (bNeedsWeaponRegistry) WeaponDesignRegistry::Clear();
    };
    if (bNeedsWeaponRegistry)
    {
        auto* Weapons = NewObject<UStarshatterWeaponDesignSubsystem>(NewObject<UGameInstance>());
        Weapons->LoadWeaponDesigns();
        if (!TestNotNull(TEXT("Runtime JSON catalog supplies carrier weapons"), WeaponDesignRegistry::Find(FName(TEXT("Vanguard")))))
            return false;
    }

    const TSet<FName> FreeFlightDesigns = {
        TEXT("Orion"), TEXT("Falcon"), TEXT("Station1"), TEXT("Mining Station"),
        TEXT("Asteroid Base"), TEXT("Processing Facility")
    };
    if (bNeedsShipRegistry)
        Ships->ForeachRow<FShipDesign>(TEXT("Free Mode test"),
            [&FreeFlightDesigns](FName Name, const FShipDesign& Row)
            {
                const FName Design = Row.ShipName.IsEmpty() ? Name : FName(*Row.ShipName);
                if (FreeFlightDesigns.Contains(Design)) ShipDesignRegistry::RegisterDesign(Design, Row);
            });

    Mission Model(Mission::FreeFlightId);
    if (!TestTrue(TEXT("Free Mode definition passes the mission parser"), Model.ParseMission(TCHAR_TO_UTF8(*Definition))))
        return false;
    const auto* Pilot = Model.GetPlayer();
    if (!TestNotNull(TEXT("Free Mode has a player"), Pilot)) return false;
    TestTrue(TEXT("Free Mode is distinct from the training missions"), Model.IsFreeFlight());
    TestEqual(TEXT("Pilot starts in Janek orbit"), FString(UTF8_TO_TCHAR(Model.GetRegion())), FString(TEXT("Janek")));
    TestEqual(TEXT("Pilot starts in the Janus system"), FString(UTF8_TO_TCHAR(Model.GetSystem())), FString(TEXT("Janus")));
    TestTrue(TEXT("Pilot starts on a carrier deck with flying start enabled or disabled"), Pilot->IsAlert());
    const auto* Squadron = Model.FindElement(Pilot->GetSquadron());
    if (!TestNotNull(TEXT("The player's squadron is aboard a carrier"), Squadron)) return false;
    TestEqual(TEXT("Carrier is Archon as in T1"), FString(UTF8_TO_TCHAR(Squadron->GetCarrier().data())), FString(TEXT("Archon")));
    TestTrue(TEXT("The free scene has no training script"), !Model.GetScriptName()[0] && Model.GetEvents().isEmpty());
    FString TestDefinition;
    if (!TestTrue(TEXT("Test Mission has its own staged definition"), FFileHelper::LoadFileToString(TestDefinition,
        *(FPaths::ProjectContentDir() / TEXT("GameData/Tests/JanekTest.def"))))) return false;
    Mission TestModel(Mission::TestFlightId);
    if (!TestTrue(TEXT("Test scene passes the mission parser"), TestModel.ParseMission(TCHAR_TO_UTF8(*TestDefinition)))) return false;
    TestTrue(TEXT("Test scene is distinct from real training"), TestModel.IsTestFlight() && TestModel.IsFreeFlight());
    TestNotNull(TEXT("Test scene has the player fighter"), TestModel.GetPlayer());
    TestEqual(TEXT("Only carrier, squadron provisioning and player are authored"), TestModel.GetElements().size(), 3);
    TestTrue(TEXT("Test environment has no instructor script or events"), !TestModel.GetScriptName()[0] && TestModel.GetEvents().isEmpty());
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FAgentCatalogTest, "Starshatter.Mission.AgentCatalog",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FAgentCatalogTest::RunTest(const FString&)
{
    auto* Catalog = NewObject<UAgentCatalogSubsystem>(NewObject<UGameInstance>());
    Catalog->LoadCatalog();
    const auto& Records = Catalog->GetCatalog();
    auto ForEachAgent = [&Records](const TCHAR*, TFunctionRef<void(FName, const FS_AgentCatalogRow&)> Visit)
    { for (const auto& Pair : Records) Visit(Pair.Key, Pair.Value); };
    TestTrue(TEXT("At least forty direct JSON agent records"), Records.Num() >= 40);
    TSet<FString> Systems;
    TSet<int32> Levels;
    for (const auto& Pair : Records)
    {
        Systems.Add(Pair.Value.System);
        Levels.Add(Pair.Value.Level);
    }
    TestTrue(TEXT("Agents are distributed across at least twenty systems"), Systems.Num() >= 20);
    TestEqual(TEXT("All five levels are represented"), Levels.Num(), 5);

    PlayerCharacter Pilot;
    auto* PreviousPilot = PlayerCharacter::GetCurrentPlayer();
    ON_SCOPE_EXIT { PlayerCharacter::SetCurrentPlayer(PreviousPilot); };
    PlayerCharacter::SetCurrentPlayer(&Pilot);
    Pilot.SetEmpire(EEMPIRE_NAME::Terellian);
    const FName AlliancePilotKey = AgentAI::PilotKey(Pilot);
    TestTrue(TEXT("Alliance pilots can contact Alliance training"), Catalog->CanContact(TEXT("JanekFlightTraining")));
    TestFalse(TEXT("Alliance pilots cannot contact Marakan agents"), Catalog->CanContact(TEXT("MarakCommand")));
    TestFalse(TEXT("Alliance pilots cannot contact Dantari agents"), Catalog->CanContact(TEXT("FoothillCommand")));
    Pilot.SetEmpire(EEMPIRE_NAME::Marakan);
    TestTrue(TEXT("Marakan pilots can contact Marakan agents"), Catalog->CanContact(TEXT("MarakCommand")));
    TestFalse(TEXT("Marakan pilots cannot contact Alliance training"), Catalog->CanContact(TEXT("JanekFlightTraining")));
    TestNotEqual(TEXT("Changing faction cannot reuse the old queue"), AgentAI::PilotKey(Pilot), AlliancePilotKey);
    Pilot.SetEmpire(EEMPIRE_NAME::Dantari);
    TestTrue(TEXT("Dantari pilots can contact Dantari agents"), Catalog->CanContact(TEXT("FoothillCommand")));
    TestFalse(TEXT("Dantari pilots cannot contact Marakan agents"), Catalog->CanContact(TEXT("MarakCommand")));
    auto* Scenarios = LoadObject<UDataTable>(nullptr, TEXT("/Game/Game/DT_SingleMissions.DT_SingleMissions"));
    if (!TestNotNull(TEXT("Standalone scenarios are available"), Scenarios)) return false;
    TestEqual(TEXT("Expanded single mission pool has forty sorties"),Scenarios->GetRowMap().Num(),40);
    TSet<int32> RoutedMissions;
    ForEachAgent(TEXT("Collect agent routing"),[&RoutedMissions](FName,const FS_AgentCatalogRow& Row)
    { if(Row.Class!=EAgentClass::Training) for(int32 Id:Row.MissionIds)RoutedMissions.Add(Id); });
    Scenarios->ForeachRow<FS_CampaignMission>(TEXT("No orphan singles"),[this,&RoutedMissions](FName,const FS_CampaignMission& Row)
    { TestTrue(FString::Printf(TEXT("Mission %d has a local officer"),Row.Id),RoutedMissions.Contains(Row.Id)); });
    const auto* Instructor=Catalog->FindAgent(TEXT("JanekFlightTraining"));
    TestTrue(TEXT("Same instructor introduces all seven lessons"),Instructor && Instructor->MissionIds.Num()==7 && Instructor->MissionIds.Contains(107));
    ForEachAgent(TEXT("Verify regional agent postings"),
        [this, Scenarios](FName Key, const FS_AgentCatalogRow& Agent)
        {
            if (Agent.Class == EAgentClass::Training)
            {
                TestEqual(TEXT("Training officer is aboard Archon"), Agent.Station, FString(TEXT("Archon")));
                return;
            }
            for (const int32 Id : Agent.MissionIds)
            {
                bool bHasMission = false;
                Scenarios->ForeachRow<FS_CampaignMission>(TEXT("Find approved mission in the region"),
                    [&Agent, &bHasMission, Id](FName, const FS_CampaignMission& Scenario)
                    {
                        bHasMission |= Scenario.Id == Id && Scenario.StoryCampaignId <= 0 &&
                            Scenario.MissionSystem.Equals(Agent.System, ESearchCase::IgnoreCase);
                    });
                TestTrue(FString::Printf(TEXT("%s has approved mission %d in its posting"), *Key.ToString(), Id), bHasMission);
            }
        });
    return true;
}


IMPLEMENT_SIMPLE_AUTOMATION_TEST(FAgentLocalSortieTest, "Starshatter.Mission.LocalAgentSorties",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FAgentLocalSortieTest::RunTest(const FString&)
{
    auto* Scenarios=LoadObject<UDataTable>(nullptr,TEXT("/Game/Game/DT_SingleMissions.DT_SingleMissions"));
    auto* Ships=LoadObject<UDataTable>(nullptr,TEXT("/Game/Game/DT_ShipDesign.DT_ShipDesign"));
    if(!TestNotNull(TEXT("Local scenarios"),Scenarios)||!TestNotNull(TEXT("Hull designs"),Ships))return false;
    const bool bPopulateShips=ShipDesignRegistry::GetAll().IsEmpty();
    const bool bPopulateWeapons=WeaponDesignRegistry::GetAll().IsEmpty();
    ON_SCOPE_EXIT {if(bPopulateShips)ShipDesignRegistry::Clear();if(bPopulateWeapons)WeaponDesignRegistry::Clear();};
    if(bPopulateWeapons)NewObject<UStarshatterWeaponDesignSubsystem>(NewObject<UGameInstance>())->LoadWeaponDesigns();
    if(bPopulateShips)Ships->ForeachRow<FShipDesign>(TEXT("Sortie hulls"),[](FName Key,const FShipDesign& Row)
    { ShipDesignRegistry::RegisterDesign(Row.ShipName.IsEmpty()?Key:FName(*Row.ShipName),Row); });
    int32 Count=0;
    Scenarios->ForeachRow<FS_CampaignMission>(TEXT("Load local sorties"),[this,&Count](FName,const FS_CampaignMission& Row)
    {
        if(Row.Id<11)return;
        Mission Model(Mission::RegionalAgentMissionOffset+Row.Id);
        TestTrue(FString::Printf(TEXT("Sortie %d provisions successfully"),Row.Id),Model.LoadFromCampaignMissionData(Row));
        TestNotNull(TEXT("Player fighter is provisioned"),Model.GetPlayer());
        TestTrue(TEXT("Sortie has friendly and hostile elements"),Model.GetElements().size()>=4);
        TestTrue(TEXT("Sortie has objective update events"),!Model.GetEvents().isEmpty());
        ++Count;
    });
    TestEqual(TEXT("Thirty new local sorties validated"),Count,30);
    return true;
}

#endif
