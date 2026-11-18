#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "MiningAI.h"
#include "TransportAI.h"
#include "Ship.h"
#include "ShipDesign.h"
#include "SimRegion.h"
#include "SimShot.h"
#include "Asteroid.h"
#include "Farcaster.h"
#include "WeaponDesign.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FNPCMovementCleanupTest,"Starshatter.AI.IndustrialMovementCompletionAndCleanup",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FNPCMovementCleanupTest::RunTest(const FString&)
{
    ShipDesign Design;
    SimRegion Origin(nullptr,"Industrial origin",SimRegion::REAL_SPACE);
    SimRegion Unreachable(nullptr,"Unreachable destination",SimRegion::REAL_SPACE);
    auto* Hauler=new Ship("Movement lifecycle fixture","",&Design,0,0,nullptr,false);
    Origin.InsertObject(Hauler);
    Hauler->SetVelocity(FVector(50,0,0));Hauler->SetAngularVelocity(FVector(1,2,3));Hauler->SetThrottleRequest(70);Hauler->SetScriptedContactSpeed(350);
    TestTrue(TEXT("Arrival returns completion"),TransportAI::FlyToRegion(*Hauler,Origin,.1));
    TestEqual(TEXT("Arrival clears stale scripted speed"),Hauler->GetContactSpeed(),0.0);
    TestTrue(TEXT("Stationkeeping clears physical drift"),Hauler->GetVelocity().IsNearlyZero());
    TestTrue(TEXT("Stationkeeping clears residual angular motion"),Hauler->GetAngularVelocity().IsNearlyZero());
    TestEqual(TEXT("Arrival releases the old throttle request"),Hauler->GetThrottleRequest(),0.0);
    Hauler->SetVelocity(FVector(40,0,0));Hauler->SetScriptedContactSpeed(350);
    TestFalse(TEXT("Missing gate route cannot complete"),TransportAI::FlyToRegion(*Hauler,Unreachable,.1));
    TestEqual(TEXT("Failed route cannot retain a moving contact tag"),Hauler->GetContactSpeed(),0.0);
    TestTrue(TEXT("Failed route stops controlled motion"),Hauler->GetVelocity().IsNearlyZero());
    const FVector Before=Hauler->GetLocation();
    TestFalse(TEXT("Negative time cannot advance a route"),TransportAI::FlyToRegion(*Hauler,Unreachable,-1));
    TestTrue(TEXT("Invalid step preserves the position"),Before.Equals(Hauler->GetLocation()));
    Farcaster Gate(1,1);Gate.SetStartPoint(FVector::ZeroVector);Gate.SetApproachPoint(0,FVector(0,0,-1000));Gate.Orient(Hauler);
    const FVector GateAxis=(Gate.StartPoint()-Gate.ApproachPoint(0)).GetSafeNormal();
    FVector Side,Up; GateAxis.FindBestAxisVectors(Side,Up);
    TestFalse(TEXT("Facing gate while stationary cannot enter"),Gate.CanEnterAt(Gate.StartPoint(),GateAxis,FVector::ZeroVector));
    TestFalse(TEXT("Sideways actual velocity cannot enter despite correct heading"),Gate.CanEnterAt(Gate.StartPoint(),GateAxis,Side*50));
    TestTrue(TEXT("Heading and actual forward velocity satisfy native gate entry"),Gate.CanEnterAt(Gate.StartPoint(),GateAxis,GateAxis*50));
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMiningStationKeepPulseTest,"Starshatter.AI.StationKeepingMiningPulsesRetainAsteroidTarget",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FMiningStationKeepPulseTest::RunTest(const FString&)
{
    Asteroid::Initialize();
    SimRegion Region(nullptr,"Stationkeeping beam fixture",SimRegion::REAL_SPACE);
    auto* Rock=new Asteroid(5,FVector::ZeroVector,5e7);Region.InsertObject(Rock);
    const FVector HoldingPoint(-(Rock->GetRadius()+600),0,0);
    WeaponDesign Laser;Laser.name="Mining Laser";Laser.primary=true;Laser.beam=true;Laser.life=5;Laser.length=2500;
    for(int32 Pulse=0;Pulse<2;++Pulse)
    {
        // Navigation has already arrived at HoldingPoint on both pulses. Its
        // destination must never be reused as the laser's asteroid endpoint.
        const FVector End=MiningAI::BeamEndpoint(HoldingPoint,Rock->GetLocation(),2500);
        auto* Shot=new SimShot(HoldingPoint,Camera(),&Laser);
        Shot->SetBeamPoints(HoldingPoint,End);Region.InsertObject(Shot);Region.DamageShips();
        TestTrue(TEXT("Repeated stationary mining pulses still intersect the asteroid"),Shot->HasMiningContact());
        TestTrue(TEXT("Beam remains nonzero after capture of stationkeeping offset"),!Shot->GetLocation().Equals(HoldingPoint));
        TestEqual(TEXT("Stationkeeping beam remains noncombat"),Shot->Damage(),0.0);
        Shot->SetLife(0);
    }
    TestTrue(TEXT("Beam cannot exceed the authored mining range"),MiningAI::BeamEndpoint(FVector::ZeroVector,FVector(10000,0,0),2500).Equals(FVector(2500,0,0)));
    TestTrue(TEXT("Invalid range emits no ray"),MiningAI::BeamEndpoint(HoldingPoint,FVector::ZeroVector,-1).Equals(HoldingPoint));
    return true;
}
#endif
