#if WITH_DEV_AUTOMATION_TESTS && WITH_EDITOR
#include "Misc/AutomationTest.h"
#include "ScopedTestWorldContext.h"
#include "Engine/World.h"
#include "Engine/GameInstance.h"
#include "MissionsDlg.h"
#include "MenuScreen.h"
#include "Widgets/Text/STextBlock.h"
#include "Misc/ScopeExit.h"
#include "Sim.h"
#include "SimElement.h"
#include "Mission.h"
#include "Ship.h"
#include "ShipDesignRegistry.h"
#include "Hangar.h"
#include "FlightDeck.h"
#include "DataLoader.h"
#include "GameStructs.h"
#include "PlayerCharacter.h"
#include "QuantumCameraPresentation.h"
#include "MissionHullAccessRules.h"
#include "OwnedHullLocation.h"
#include "TrainingShuttleRules.h"
#include "StarshatterGameDataSubsystem.h"
#include "SSWGameInstance.h"
#include "Misc/Paths.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FT3LaunchFeedbackTest, "Starshatter.Mission.T3LaunchFeedback",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FT3LaunchFeedbackTest::RunTest(const FString&)
{
    auto* World = UWorld::CreateWorld(EWorldType::EditorPreview, false);
    FScopedSSWTestWorldContext Context(World);
    World->SetGameInstance(NewObject<UGameInstance>());
    ON_SCOPE_EXIT { World->DestroyWorld(false); };

    auto* Panel = NewObject<UMissionsDlg>(World);
    Panel->Initialize();
    Panel->SetDetails(INVTEXT("Flight School"), FText::GetEmpty(), {}, {}, {});
    auto Root = Panel->TakeWidget();
    const FText Instruction = INVTEXT("Remain aboard Orion until transit completes, then land at Janek Station.");
    Panel->SetLaunchFeedback(Instruction);
    TFunction<bool(const TSharedRef<SWidget>&)> ContainsInstruction;
    ContainsInstruction = [&ContainsInstruction, &Instruction](const TSharedRef<SWidget>& Widget)
    {
        if (Widget->GetType() == FName(TEXT("STextBlock")) &&
            StaticCastSharedRef<STextBlock>(Widget)->GetText().EqualTo(Instruction))
            return Widget->GetVisibility() == EVisibility::Visible;
        auto* Children = Widget->GetChildren();
        for (int32 Index = 0; Index < Children->Num(); ++Index)
            if (ContainsInstruction(Children->GetChildAt(Index))) return true;
        return false;
    };
    TestTrue(TEXT("Transit instructions appear in the already-open briefing"), ContainsInstruction(Root));
    Panel->SetLaunchFeedback(FText::GetEmpty());
    TestFalse(TEXT("Old transit feedback clears without rebuilding the panel"), ContainsInstruction(Root));

    auto* Menu = NewObject<UMenuScreen>(World);
    FText Reason;
    TestFalse(TEXT("Unavailable mission catalog cannot launch T3"), Menu->OpenQueuedAssignment(103, true, &Reason));
    TestFalse(TEXT("A rejected launch supplies a visible reason"), Reason.IsEmpty());
    return true;
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTrainingFighterOwnershipTest, "Starshatter.Mission.TrainingFighterOwnership",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FTrainingFighterOwnershipTest::RunTest(const FString&)
{
    FS_PlayerGameInfo PilotState;
    TestNull(TEXT("Missing required fighter blocks start"), MissionHullAccessRules::FindAvailable(PilotState, TEXT("Stormhawk")));
    auto& Hull = PilotState.HangarShips.AddDefaulted_GetRef();
    Hull.HullId = FGuid::NewGuid(); Hull.Design = TEXT("Stormhawk");
    TestNotNull(TEXT("Owned mission fighter is available"), MissionHullAccessRules::FindAvailable(PilotState, TEXT("Stormhawk")));
    TestNull(TEXT("Owning Stormhawk does not satisfy a Falcon mission"), MissionHullAccessRules::FindAvailable(PilotState, TEXT("Falcon")));
    Hull.bLost = true;
    TestNull(TEXT("Lost fighter does not satisfy ownership"), MissionHullAccessRules::FindAvailable(PilotState, TEXT("Stormhawk")));
    Hull.bLost = false; Hull.bAssigned = true; PilotState.AssignedHangarHull = Hull.HullId;
    TestNull(TEXT("Advanced loaner cannot bypass ownership"), MissionHullAccessRules::FindAvailable(PilotState, TEXT("Stormhawk")));
    Hull.Design = TEXT("Falcon");
    TestNotNull(TEXT("Existing issued Falcon trainer remains available"), MissionHullAccessRules::FindAvailable(PilotState, TEXT("Falcon")));
    Hull.bAssigned = false; Hull.Design = TEXT("Orion");
    TestNotNull(TEXT("An owned campaign capital ship satisfies its authored hull requirement"), MissionHullAccessRules::FindAvailable(PilotState, TEXT("Orion")));
    TestNull(TEXT("Capital ship ownership does not satisfy a different story hull"), MissionHullAccessRules::FindAvailable(PilotState, TEXT("Berents")));
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FHangarStoredLocationsTest, "Starshatter.Hangar.StoredLocationsAndShuttleReturn",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FHangarStoredLocationsTest::RunTest(const FString&)
{
    FS_PlayerGameInfo PilotState; PilotState.Campaign = 1; PilotState.Empire = int32(EEMPIRE_NAME::Terellian);
    FPlayerHangarShip Fighter; Fighter.HullId = FGuid::NewGuid(); Fighter.Design = TEXT("Falcon");
    Fighter.Location = OwnedHullLocation::Station(TEXT("Janus"), TEXT("Janek Station"));
    FPlayerHangarShip Capital; Capital.HullId = FGuid::NewGuid(); Capital.Design = TEXT("Asher");
    Capital.Location = OwnedHullLocation::Station(TEXT("Janus"), TEXT("Janus Transfer Station"));
    PilotState.HangarShips = {Fighter, Capital};
    TestEqual(TEXT("Surface base tracks its stored fighter"), OwnedHullLocation::StoredAt(PilotState, Fighter.Location).Num(), 1);
    TestEqual(TEXT("Another base tracks its own stored ship"), OwnedHullLocation::StoredAt(PilotState, Capital.Location).Num(), 1);
    const FString LeftBehind = PilotState.HangarShips[0].Location;
    PilotState.HangarShips[1].Location = TEXT("In flight / Janus / Janek");
    TestEqual(TEXT("Flying another craft does not move the parked fighter"), PilotState.HangarShips[0].Location, LeftBehind);
    TestFalse(TEXT("Remote fighter cannot board at a different base"), OwnedHullLocation::IsAt(PilotState.HangarShips[0], OwnedHullLocation::Carrier(TEXT("Orion"))));
    FPlayerHangarShip Legacy; Legacy.HullId = FGuid::NewGuid(); Legacy.Location = TEXT("Janek");
    FEconomyTransaction Receipt; Receipt.Reference = Legacy.HullId; Receipt.Category = EWalletTransactionCategory::Shipyard;
    Receipt.To = TEXT("NPC/Janus/Orion/Squadron Sales"); PilotState.Economy.Journal.Add(Receipt);
    TestTrue(TEXT("Purchase receipt restores the old carrier storage location"), OwnedHullLocation::RecoverPurchase(PilotState, Legacy));
    TestEqual(TEXT("Carrier identity remains stable across its jumps"), Legacy.Location, OwnedHullLocation::Carrier(TEXT("Orion")));
    TestFalse(TEXT("Before T3, normal career entry is aboard Orion"), TrainingShuttleRules::AtJanek(PilotState));
    TestTrue(TEXT("Unfinished T3 may resume its live checkpoint"), TrainingShuttleRules::CanResumeSavedMission(PilotState,103));
    PilotState.MarkMissionCompleted(1,102);
    TestEqual(TEXT("After T2, docked pilot receives TDY to Janek"),TrainingShuttleRules::TransferStage(PilotState,true,true,true,false),1);
    TestEqual(TEXT("TDY is not offered again while already at Janek"),TrainingShuttleRules::TransferStage(PilotState,true,true,false,true),0);
    PilotState.MarkMissionCompleted(1, 103);
    TestFalse(TEXT("Completed T3 cannot reopen an older lesson checkpoint"), TrainingShuttleRules::CanResumeSavedMission(PilotState,103));
    TestTrue(TEXT("Unfinished T4 is still eligible at Janek"), TrainingShuttleRules::CanResumeSavedMission(PilotState,104));
    TestTrue(TEXT("After T3, pilot stays at Janek for T4"), TrainingShuttleRules::AtJanek(PilotState));
    PilotState.MarkMissionCompleted(1, 104);
    TestEqual(TEXT("After T4, return to Orion is separate from campaign acceptance"),TrainingShuttleRules::TransferStage(PilotState,true,true,false,true),2);
    TestEqual(TEXT("No return shuttle is offered after reaching Orion"),TrainingShuttleRules::TransferStage(PilotState,true,true,true,false),0);
    TestEqual(TEXT("A flying pilot cannot take the shuttle"),TrainingShuttleRules::TransferStage(PilotState,true,false,false,true),0);
    TestFalse(TEXT("Completed T4 cannot reopen the surface lesson after shuttle return"), TrainingShuttleRules::CanResumeSavedMission(PilotState,104));
    TestTrue(TEXT("Other assignment resume rules are unchanged"), TrainingShuttleRules::CanResumeSavedMission(PilotState,201));
    TestFalse(TEXT("After T4, shuttle returns the pilot to Orion"), TrainingShuttleRules::AtJanek(PilotState));
    auto* Data = NewObject<UStarshatterGameDataSubsystem>(NewObject<USSWGameInstance>());
    FS_CampaignMission Surface;
    TestTrue(TEXT("Between-lesson surface entry ingests directly from JSON"), Data->ReadLooseMission(FPaths::ProjectContentDir() / TEXT("GameData/Campaigns/01/Scenes/FreeFlightSurface.json"), Surface));
    TestEqual(TEXT("Surface intermission is career mode, not a replayed training mission"), Surface.Id, Mission::FreeFlightId);
    TestEqual(TEXT("Surface intermission stays at Janek Station"), Surface.MissionRegion, FString(TEXT("Janek Station")));

    return true;
}

namespace
{
class FTrainingBoardingSim : public Sim
{
public:
    FTrainingBoardingSim() : Sim(nullptr) {}
    using Sim::mission;
    using Sim::regions;
    using Sim::active_region;
};
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FT3OwnedStormhawkBoardingTest, "Starshatter.Mission.T3OwnedStormhawkBoarding",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FT3OwnedStormhawkBoardingTest::RunTest(const FString&)
{
    const auto DesignsBefore = ShipDesignRegistry::GetAll();
    ON_SCOPE_EXIT { ShipDesignRegistry::Clear(); for (const auto& Pair : DesignsBefore) ShipDesignRegistry::RegisterDesign(Pair.Key, Pair.Value, false); };
    const bool OwnLoader = !DataLoader::GetLoader();
    if (OwnLoader) DataLoader::Initialize();
    ON_SCOPE_EXIT { if (OwnLoader) DataLoader::Close(); };
    if (Sim::GetSim()) { AddError(TEXT("Boarding test requires an idle simulation")); return false; }
    
    auto Simulation = MakeUnique<FTrainingBoardingSim>();
    Mission Free(Mission::FreeFlightId);
    auto* Region = new SimRegion(Simulation.Get(), "Boarding sector", SimRegion::REAL_SPACE);
    Simulation->regions.append(Region); Simulation->active_region = Region; Simulation->mission = &Free;
    ON_SCOPE_EXIT { Simulation->mission = nullptr; Simulation->active_region = nullptr; Simulation->regions.remove(Region); delete Region; };
    FShipDesign Fighter;
    Fighter.ShipName = TEXT("Falcon"); Fighter.ShipType = int32(CLASSIFICATION::FIGHTER);
    Fighter.Category = EShipCategory::Fighter; Fighter.Integrity = 100; Fighter.Mass = 20; Fighter.Scale = 1;
    ShipDesignRegistry::RegisterDesign(TEXT("Falcon"), Fighter, false);
    Fighter.ShipName = TEXT("Stormhawk"); ShipDesignRegistry::RegisterDesign(TEXT("Stormhawk"), Fighter, false);
    FShipDesign Carrier; Carrier.ShipName = TEXT("BoardingCarrierFixture"); Carrier.ShipType = int32(CLASSIFICATION::CARRIER);
    Carrier.Mass = 1000; Carrier.Integrity = 10000; Carrier.Scale = 1;
    for (const TCHAR* Name : {TEXT("Falcon"), TEXT("Stormhawk")})
    { FShipSquadron Squadron; Squadron.Name = Name; Squadron.DesignName = Name; Squadron.Count = Squadron.Avail = 1; Carrier.Squadron.Add(Squadron); }
    ShipDesignRegistry::RegisterDesign(TEXT("BoardingCarrierFixture"), Carrier, false);
    auto* Base = Simulation->CreateShip("Orion", "", ShipDesignRegistry::FindLegacy(FName(TEXT("BoardingCarrierFixture"))), Region->GetName(), FVector::ZeroVector, 1);
    if (!TestNotNull(TEXT("Carrier created"), Base)) return false;
    Base->InitializeRuntimeHangarFromDesign();
    auto* Inventory = Base->GetHangar();
    if (!TestNotNull(TEXT("Hangar created"), Inventory)) return false;
    auto* Deck = new FlightDeck(); Deck->SetCarrier(Base); Deck->SetLaunchDeck();
    Deck->AddSlot(FVector::ZeroVector, uint32(CLASSIFICATION::FIGHTER));
    Deck->AddSlot(FVector(100, 0, 0), uint32(CLASSIFICATION::FIGHTER)); Base->GetFlightDecks().append(Deck);
    auto* Element = Simulation->CreateElement("Pilot", 1, int32(CLASSIFICATION::FIGHTER));
    Element->SetCarrier(Base); Element->SetPlayer(1);
    if (!TestTrue(TEXT("Falcon staged"), Inventory->GotoAlert(0, 0, Deck, Element, nullptr, false, true))) return false;
    auto* Original = Inventory->GetShip(Inventory->GetSlot(0, 0));
    if (!TestNotNull(TEXT("Original fighter"), Original)) return false;
    Region->SetPlayerShip(Original);
    FS_PlayerGameInfo PilotState; PilotState.Rank = 0;
    auto& Owned = PilotState.HangarShips.AddDefaulted_GetRef();
    Owned.HullId = FGuid::NewGuid(); Owned.Design = TEXT("Stormhawk"); Owned.Location = TEXT("Carrier / Orion"); Owned.bHasCondition = true; Owned.HullCondition = .8f;
    const FGuid Hull = Owned.HullId;
    TestTrue(TEXT("Purchased reserve removed from sale"), Inventory->TransferStoredReserve(1, 0));
    FText Reason;
    TestFalse(TEXT("Failed save cancels boarding"), Simulation->BoardOwnedTrainingFighter(PilotState, Hull, [] { return false; }, Reason));
    TestEqual(TEXT("Save failure retains original player"), Simulation->GetPlayerShip(), Original);
    TestEqual(TEXT("Save failure restores purchased reserve status"), Inventory->GetState(Inventory->GetSlot(1, 0)), int(Hangar::TRANSFERRED));
    TestFalse(TEXT("Save failure preserves selection"), PilotState.SelectedHangarHull.IsValid());
    const bool BoardedSuccessfully = Simulation->BoardOwnedTrainingFighter(PilotState, Hull, [] { return true; }, Reason);
    if (!TestTrue(FString::Printf(TEXT("Owned Stormhawk boards without reloading: %s"), *Reason.ToString()), BoardedSuccessfully)) return false;
    auto* Boarded = Simulation->GetPlayerShip();
    if (!TestNotNull(TEXT("Boarded fighter"), Boarded)) return false;
    TestTrue(TEXT("Pilot is in the purchased Stormhawk"), Boarded->Design() == ShipDesignRegistry::FindLegacy(FName(TEXT("Stormhawk"))));
    TestEqual(TEXT("New fighter remains aboard same carrier"), Boarded->GetCarrier(), Base);
    TestEqual(TEXT("New fighter awaits launch"), Boarded->GetFlightPhase(), EOPSMode::ALERT);
    TestEqual(TEXT("Purchased identity is assigned"), PilotState.AssignedHangarHull, Hull);
    TestEqual(TEXT("Purchased cargo identity is used"), Boarded->GetCargoInventoryKey(), FName(*(TEXT("Ship/") + Hull.ToString(EGuidFormats::Digits))));
    TestTrue(TEXT("Stored hull condition retained"), FMath::IsNearlyEqual(Boarded->GetIntegrity(), 80.0, .01));
    TestFalse(TEXT("Previous fighter is retired from live contacts"), Region->GetShips().contains(Original));
    TestEqual(TEXT("Previous fighter is returned to reserve"), Inventory->GetState(Inventory->GetSlot(0, 0)), int(Hangar::STORAGE));
    const int32 ShipCount = Region->GetNumShips();
    TestTrue(TEXT("Repeated request is idempotent"), Simulation->BoardOwnedTrainingFighter(PilotState, Hull, [] { return false; }, Reason));
    TestEqual(TEXT("Repeated request creates no duplicate hull"), Region->GetNumShips(), ShipCount);
    Base->SetWarp(100);
    TestEqual(TEXT("Launch-bay quantum presentation follows the carrier"), ResolveQuantumPresentationShip(Boarded), Base);
    TestEqual(TEXT("Carrier presentation does not change the parked fighter's simulation warp"), Boarded->GetWarpFactor(), 1.0);
    Boarded->SetFlightPhase(EOPSMode::ACTIVE);
    TestEqual(TEXT("Launched fighter stops inheriting carrier jump effects"), ResolveQuantumPresentationShip(Boarded), Boarded);
    TestFalse(TEXT("Aircraft cannot be changed in flight"), Simulation->BoardOwnedTrainingFighter(PilotState, Hull, [] { return true; }, Reason));
    return true;
}
#endif
