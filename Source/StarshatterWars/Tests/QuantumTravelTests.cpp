#if WITH_DEV_AUTOMATION_TESTS && WITH_EDITOR
#include "Misc/AutomationTest.h"
#include "QuantumTravel.h"
#include "QuantumDrive.h"
#include "QuantumTravelPopup.h"
#include "TravelTransition.h"
#include "StarSystem.h"
#include "SimRegion.h"
#include "Game.h"
#include "ShipDesign.h"
#include "ShipDesignRegistry.h"
#include "Misc/ScopeExit.h"

namespace
{
    class FTravelRegionFixture : public SimRegion
    {
    public:
        FTravelRegionFixture(StarSystem* System, const char* Name, int RegionType = REAL_SPACE)
            : SimRegion(nullptr, Name, RegionType) { star_system = System; }
    };
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FQuantumTravelDestinationsTest,
    "Starshatter.Travel.Destinations", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FQuantumTravelDestinationsTest::RunTest(const FString& Parameters)
{
    StarSystem Solus("Solus", FVector::ZeroVector), Loris("Loris", FVector(1,2,3));
    FTravelRegionFixture Current(&Solus,"Current"), Local(&Solus,"Local"),
        Foreign(&Loris,"Foreign"), Air(&Solus,"Air",SimRegion::AIR_SPACE), Unknown(nullptr,"Unknown");
    TestTrue(TEXT("Local space region reachable"), QuantumTravel::IsDestination(&Current,&Local));
    TestFalse(TEXT("Current region excluded"), QuantumTravel::IsDestination(&Current,&Current));
    TestFalse(TEXT("Unlinked system excluded"), QuantumTravel::IsDestination(&Current,&Foreign));
    Current.GetLinks().append(&Foreign);
    TestTrue(TEXT("Authored cross-system link reachable"), QuantumTravel::IsDestination(&Current,&Foreign));
    Current.GetLinks().clear();
    TestFalse(TEXT("Removed link no longer reachable"), QuantumTravel::IsDestination(&Current,&Foreign));
    TestFalse(TEXT("Airspace excluded"), QuantumTravel::IsDestination(&Current,&Air));
    TestFalse(TEXT("Missing system excluded"), QuantumTravel::IsDestination(&Current,&Unknown));
    TestFalse(TEXT("Missing player region excluded"), QuantumTravel::IsDestination(nullptr,&Local));
    TestFalse(TEXT("Missing destination excluded"), QuantumTravel::IsDestination(&Current,nullptr));
    return true;
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FFarcasterCoordinatesTest,
    "Starshatter.Travel.FarcasterArrival", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FFarcasterCoordinatesTest::RunTest(const FString& Parameters)
{
    // Asymmetric nonzero axes catch both mirroring and Y/Z swaps.
    const FVector GateSimLocation(12345,-67890,4321);
    const FVector QueuedLocation = QuantumTravel::FarcasterMapLocation(GateSimLocation);
    TestEqual(TEXT("Queue uses map-space X"), QueuedLocation.X, -12345.0);
    TestEqual(TEXT("Queue uses map-space Y"), QueuedLocation.Y, 4321.0);
    TestEqual(TEXT("Queue uses map-space Z"), QueuedLocation.Z, -67890.0);
    TestTrue(TEXT("Simulation resolver arrives exactly at destination gate"), OtherHand(QueuedLocation).Equals(GateSimLocation));
    const FVector RiderOffset(700,1200,-80);
    TestTrue(TEXT("Rider formation offset preserved on arrival"), (OtherHand(QueuedLocation)+RiderOffset).Equals(GateSimLocation+RiderOffset));
    return true;
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTravelOwnershipTest,
    "Starshatter.Travel.RegionOwnership", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FTravelOwnershipTest::RunTest(const FString& Parameters)
{
    FShipDesign Row; Row.ShipName=TEXT("TravelFixture"); Row.ShipClass=TEXT("Fighter"); Row.Mass=20; Row.Integrity=100;
    const auto OriginalDesigns=ShipDesignRegistry::GetAll();
    ON_SCOPE_EXIT { ShipDesignRegistry::Clear(); for (const auto& Entry:OriginalDesigns) ShipDesignRegistry::RegisterDesign(Entry.Key,Entry.Value); };
    ShipDesignRegistry::RegisterDesign(FName(TEXT("TravelFixture")),Row);
    ShipDesign* Design=ShipDesignRegistry::FindLegacy(FName(TEXT("TravelFixture")));
    StarSystem System("Solus", FVector::ZeroVector);
    FTravelRegionFixture Source(&System,"Source"), Destination(&System,"Destination");
    Ship* Pilot = new Ship("Travel fixture","",Design,0,0,nullptr,false);
    Source.InsertObject(Pilot); Source.SetPlayerShip(Pilot); Source.AddSelection(Pilot);
    const FVector Location(200,300,400); Pilot->MoveTo(Location);
    Destination.TransferObject(Pilot);
    TestFalse(TEXT("Source releases ship ownership"), Source.GetShips().contains(Pilot));
    TestNull(TEXT("Source releases player ownership"), Source.GetPlayerShip());
    TestFalse(TEXT("Source selection releases traveler"), Source.IsSelected(Pilot));
    TestTrue(TEXT("Destination owns traveler"), Destination.GetShips().contains(Pilot));
    TestTrue(TEXT("Traveler points to destination"), Pilot->GetRegion()==&Destination);
    TestTrue(TEXT("Ownership transfer does not silently alter coordinates"), Pilot->GetLocation().Equals(Location));
    Destination.TransferObject(Pilot);
    TestEqual(TEXT("Repeated insertion never duplicates traveler"), Destination.GetShips().size(),1);
    Source.TransferObject(Pilot);
    TestTrue(TEXT("Return trip moves ownership back"), Source.GetShips().contains(Pilot) && Destination.GetShips().size()==0);
    Ship* Neighbor = new Ship("Nearby fixture","",Design,0,0,nullptr,false);
    Source.InsertObject(Neighbor); Neighbor->MoveTo(Location+FVector(1000,0,0));
    QuantumDrive Drive(QuantumDrive::QUANTUM,100,10);
    Drive.SetShip(Pilot); Drive.SetDestination(&Destination,FVector::ZeroVector);
    TestTrue(TEXT("Charged mounted drive engages"), Drive.Engage());
    TestEqual(TEXT("Legacy proximity delay adds five seconds"), Drive.JumpTime(),10.0);
    Drive.ExecFrame(10.1); Drive.ExecFrame(0.1); Drive.ExecFrame(0.1);
    TestEqual(TEXT("Legacy prewarp multiplier retained"), Pilot->GetWarpFactor(),1.5);
    Drive.CancelJump();
    TestEqual(TEXT("Cancel resets traveler warp"), Pilot->GetWarpFactor(),1.0);
    TestEqual(TEXT("Cancel discharges drive"), Drive.GetEnergy(),0.0);
    return true;
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FQuantumTravelSafetyTest,
    "Starshatter.Travel.UnmountedDriveAndUI", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FQuantumTravelSafetyTest::RunTest(const FString& Parameters)
{
    QuantumDrive Drive(QuantumDrive::QUANTUM,100,10);
    TestNull(TEXT("Unmounted drive has no ship"), Drive.GetShip());
    TestFalse(TEXT("Unmounted drive cannot engage"), Drive.Engage());
    Drive.CancelJump(); Drive.SetPowerOff(); Drive.ExecFrame(0.1);
    TestEqual(TEXT("Power off without ship is safe and ready"), Drive.ActiveState(), int(QuantumDrive::ACTIVE_READY));
    TestEqual(TEXT("Power off resets warp"), Drive.WarpFactor(), 1.0);
    QuantumDrive Copy(Drive);
    TestNull(TEXT("Copied design drive never inherits runtime ship pointer"), Copy.GetShip());
    TestFalse(TEXT("Missing pilot reports unavailable"), QuantumTravel::Readiness(nullptr).IsEmpty());
    FString Message;
    TestFalse(TEXT("Missing simulation/pilot cannot engage"), QuantumTravel::Engage(nullptr,nullptr,TEXT("Solus"),TEXT("Solus"),Message));
    // Construct full Slate hierarchy, including persistent button styles, with no live mission.
    auto Popup=SNew(SQuantumTravelPopup).ResolveShip([](){return static_cast<Ship*>(nullptr);}).CanOperate([](){return false;});
    Popup->SlatePrepass();
    auto Transition=SNew(STravelTransition);
    Transition->Update(0.016f,5000,false,TEXT("Solus")); Transition->SlatePrepass();
    return true;
}
#endif
