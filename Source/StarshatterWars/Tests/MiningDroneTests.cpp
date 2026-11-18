#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "MiningDroneAI.h"
#include "MiningDroneSaveTransaction.h"
#include "PlayerSaveGame.h"
#include "Kismet/GameplayStatics.h"

namespace
{
FMiningDroneBayDefinition DroneBay(){FMiningDroneBayDefinition Bay;Bay.ShipDesign=TEXT("Prospector");Bay.BayCount=2;Bay.Bandwidth=1;Bay.CapacityTonnes=.25;Bay.Speed=1000;Bay.ControlRange=1000;return Bay;}
FMiningDroneFrame DroneFrame(){FMiningDroneFrame Frame;Frame.bAuthority=true;Frame.bTargetValid=true;Frame.bTargetAssayed=true;Frame.TargetContact=7;Frame.TargetOre=TEXT("Magnetite");Frame.TargetPosition=FVector(200,0,0);Frame.TargetRadius=10;Frame.RemainingTonnes=1;return Frame;}
FMiningDroneFleetState DroneFleet(){FMiningDroneFleetState Fleet;for(int32 I=0;I<2;++I){FMiningDroneState Drone;Drone.Id=FGuid::NewGuid();Fleet.Drones.Add(Drone);}return Fleet;}
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMiningDroneControlTest,"Starshatter.Mining.Drones.AuthorityBandwidthAndAssay",EAutomationTestFlags_ApplicationContextMask|EAutomationTestFlags::EngineFilter)
bool FMiningDroneControlTest::RunTest(const FString&)
{
    auto Fleet=DroneFleet();const auto Bay=DroneBay();auto Frame=DroneFrame();FText Reason;
    TestTrue(TEXT("Authoritative active play allows drone simulation"),MiningDroneAI::Eligible(true,true,false,false,false));
    TestFalse(TEXT("Cinematics cannot provision or operate drones"),MiningDroneAI::Eligible(true,true,true,false,false));
    TestFalse(TEXT("Offline movie export cannot mutate drone economy"),MiningDroneAI::Eligible(true,true,false,true,false));
    TestFalse(TEXT("Paused world cannot progress mining"),MiningDroneAI::Eligible(true,true,false,false,true));
    TestFalse(TEXT("Menus and briefings cannot tick mining"),MiningDroneAI::Eligible(true,false,false,false,false));
    Frame.bAuthority=false;TestFalse(TEXT("Client cannot launch drones"),MiningDroneAI::Launch(Fleet,Fleet.Drones[0].Id,Bay,Frame,Reason));
    Frame.bAuthority=true;TestTrue(TEXT("Authoritative owner launches drone"),MiningDroneAI::Launch(Fleet,Fleet.Drones[0].Id,Bay,Frame,Reason));
    TestFalse(TEXT("Bandwidth limits deployed drones"),MiningDroneAI::Launch(Fleet,Fleet.Drones[1].Id,Bay,Frame,Reason));
    Frame.bTargetAssayed=false;TestFalse(TEXT("Unknown ore cannot be mined"),MiningDroneAI::Mine(Fleet.Drones[0],Bay,Frame,Reason));
    Frame.bTargetAssayed=true;Frame.TargetPosition=FVector(2000,0,0);TestFalse(TEXT("Control range gate enforced"),MiningDroneAI::Mine(Fleet.Drones[0],Bay,Frame,Reason));
    Frame.TargetPosition=FVector(200,0,0);TestTrue(TEXT("Assayed live deposit accepted"),MiningDroneAI::Mine(Fleet.Drones[0],Bay,Frame,Reason));
    double Ore=1;int32 ExtractCalls=0;Frame.bAuthority=false;
    MiningDroneAI::Tick(Fleet.Drones[0],Bay,Frame,1,.1,[&](double Amount){++ExtractCalls;Ore-=Amount;return Amount;},[](FName,double){return 0.0;});
    TestEqual(TEXT("Client tick cannot extract source ore"),ExtractCalls,0);TestEqual(TEXT("Source unchanged without authority"),Ore,1.0);
    return true;
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMiningDroneCargoTest,"Starshatter.Mining.Drones.ReturnOnlyCargoAndLoss",EAutomationTestFlags_ApplicationContextMask|EAutomationTestFlags::EngineFilter)
bool FMiningDroneCargoTest::RunTest(const FString&)
{
    auto Fleet=DroneFleet();const auto Bay=DroneBay();auto Frame=DroneFrame();FText Reason;auto& Drone=Fleet.Drones[0];double Ore=1,Hold=0;
    MiningDroneAI::Launch(Fleet,Drone.Id,Bay,Frame,Reason);MiningDroneAI::Mine(Drone,Bay,Frame,Reason);
    const auto Extract=[&](double Requested){const double Amount=FMath::Min(Ore,Requested);Ore-=Amount;return Amount;};
    const auto Unload=[&](FName,double Amount){Hold+=Amount;return Amount;};
    MiningDroneAI::Tick(Drone,Bay,Frame,1,.1,Extract,Unload);
    TestTrue(TEXT("Actual source ore moves into drone hold"),FMath::IsNearlyEqual(Drone.OreTonnes,.1));
    TestEqual(TEXT("Owner receives no ore while drone is at rock"),Hold,0.0);
    MiningDroneAI::Stop(Drone,Frame,Reason);TestEqual(TEXT("Stop preserves onboard ore without a delivery"),Hold,0.0);
    MiningDroneAI::Return(Drone,Frame,Reason);MiningDroneAI::Tick(Drone,Bay,Frame,1,.1,Extract,Unload);
    TestEqual(TEXT("Drone docks only after physical return"),Drone.State,EMiningDroneState::Docked);
    TestTrue(TEXT("Owner receives actual returned ore"),FMath::IsNearlyEqual(Hold,.1));
    TestEqual(TEXT("Dock clears drone cargo"),Drone.OreTonnes,0.0);
    TestFalse(TEXT("Docked drone cannot deliver twice"),MiningDroneAI::Dock(Drone,Frame,Unload,Reason));
    MiningDroneAI::Launch(Fleet,Drone.Id,Bay,Frame,Reason);MiningDroneAI::Mine(Drone,Bay,Frame,Reason);MiningDroneAI::Tick(Drone,Bay,Frame,1,.1,Extract,Unload);
    Frame.bOwnerAlive=false;MiningDroneAI::Tick(Drone,Bay,Frame,1,.1,Extract,Unload);
    TestEqual(TEXT("Owner loss marks deployed drone lost"),Drone.State,EMiningDroneState::Lost);
    TestEqual(TEXT("Lost drone's ore is destroyed"),Drone.OreTonnes,0.0);TestTrue(TEXT("Loss cannot credit owner hold"),FMath::IsNearlyEqual(Hold,.1));
    TestFalse(TEXT("Lost drone cannot be relaunched"),MiningDroneAI::Launch(Fleet,Drone.Id,Bay,Frame,Reason));
    auto* Save=NewObject<UPlayerSaveGame>();Save->PlayerInfo.Economy.MiningDrones.Add(TEXT("OwnedHull"),Fleet);TArray<uint8> Bytes;
    if(!TestTrue(TEXT("Drone losses and bay stock serialize with the owned hull"),UGameplayStatics::SaveGameToMemory(Save,Bytes)))return false;
    const auto* Loaded=Cast<UPlayerSaveGame>(UGameplayStatics::LoadGameFromMemory(Bytes));if(!TestNotNull(TEXT("Saved drone fleet reloads"),Loaded))return false;
    const auto& SavedDrone=Loaded->PlayerInfo.Economy.MiningDrones.FindChecked(TEXT("OwnedHull")).Drones[0];
    TestEqual(TEXT("Reload cannot replenish lost drones"),SavedDrone.State,EMiningDroneState::Lost);TestEqual(TEXT("Reload cannot recover lost drone ore"),SavedDrone.OreTonnes,0.0);
    return true;
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMiningDroneCapacityTest,"Starshatter.Mining.Drones.FullOwnerHoldAndDepositDepletion",EAutomationTestFlags_ApplicationContextMask|EAutomationTestFlags::EngineFilter)
bool FMiningDroneCapacityTest::RunTest(const FString&)
{
    auto Fleet=DroneFleet();const auto Bay=DroneBay();auto Frame=DroneFrame();FText Reason;auto& Drone=Fleet.Drones[0];double Source=.03,Hold=0;
    Frame.RemainingTonnes=Source;MiningDroneAI::Launch(Fleet,Drone.Id,Bay,Frame,Reason);MiningDroneAI::Mine(Drone,Bay,Frame,Reason);
    MiningDroneAI::Tick(Drone,Bay,Frame,1,100,[&](double Amount){Source-=Amount;return Amount;},[](FName,double){return 0.0;});
    TestTrue(TEXT("Extraction never exceeds remaining deposit"),FMath::IsNearlyZero(Source));TestTrue(TEXT("Drone stores only existing ore"),FMath::IsNearlyEqual(Drone.OreTonnes,.03));
    MiningDroneAI::Return(Drone,Frame,Reason);MiningDroneAI::Tick(Drone,Bay,Frame,1,.1,[](double){return 0.0;},[](FName,double){return 0.0;});
    TestEqual(TEXT("Full owner hold prevents docking/unload"),Drone.State,EMiningDroneState::Returning);TestTrue(TEXT("Cargo stays in waiting drone"),FMath::IsNearlyEqual(Drone.OreTonnes,.03));
    TestFalse(TEXT("Partial unload is not falsely reported as docked"),MiningDroneAI::Dock(Drone,Frame,[&](FName,double){Hold+=.01;return .01;},Reason));
    TestEqual(TEXT("Partial unload leaves drone returning"),Drone.State,EMiningDroneState::Returning);
    TestTrue(TEXT("Only actual unloaded tonnes enter owner hold"),FMath::IsNearlyEqual(Hold,.01));
    TestTrue(TEXT("Remaining ore stays aboard drone"),FMath::IsNearlyEqual(Drone.OreTonnes,.02));
    TestTrue(TEXT("Freed hold allows docking"),MiningDroneAI::Dock(Drone,Frame,[&](FName,double Amount){Hold+=Amount;return Amount;},Reason));
    TestTrue(TEXT("Delivery conserves extracted tonnes"),FMath::IsNearlyEqual(Hold,.03));
    return true;
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMiningDroneRollbackTest,"Starshatter.Mining.Drones.FailedSaveConservesAsteroidDroneAndOwnerMass",EAutomationTestFlags_ApplicationContextMask|EAutomationTestFlags::EngineFilter)
bool FMiningDroneRollbackTest::RunTest(const FString&)
{
    Asteroid Rock(11,FVector::ZeroVector,1000000);const double Original=Rock.GetResource().AmountTonnes;
    FEconomyState State;FEconomyLedger::CreateInventory(State,TEXT("Ship"),FEconomyLedger::Pilot(),TEXT("Janus"),1);
    auto Fleet=DroneFleet();Fleet.Drones[0].Ore=TEXT("Magnetite");Fleet.Drones[0].OreTonnes=.1;Fleet.Drones[0].State=EMiningDroneState::Returning;State.MiningDrones.Add(TEXT("Ship"),Fleet);
    const auto Before=State;const double Taken=Rock.ExtractOre(.2);TArray<TPair<Asteroid*,double>> Extracted;Extracted.Emplace(&Rock,Taken);
    State.MiningDrones.FindChecked(TEXT("Ship")).Drones[0].OreTonnes+=Taken;
    FEconomyLedger::Deposit(State,TEXT("Ship"),TEXT("Magnetite"),.1,1);State.MiningDrones.FindChecked(TEXT("Ship")).Drones[0].OreTonnes-=.1;
    int32 Saves=0,Applications=0;
    TestFalse(TEXT("Failed durable save rejects mining/unload commit"),MiningDroneSaveTransaction::Commit(State,Before,Extracted,[&](){++Saves;return false;},[&](){++Applications;}));
    TestEqual(TEXT("Only one save is attempted"),Saves,1);TestEqual(TEXT("Cargo presentation applied before save and after rollback"),Applications,2);
    TestTrue(TEXT("Actual asteroid mass restored"),FMath::IsNearlyEqual(Rock.GetResource().AmountTonnes,Original));
    TestTrue(TEXT("Drone's previous cargo restored"),FMath::IsNearlyEqual(State.MiningDrones.FindChecked(TEXT("Ship")).Drones[0].OreTonnes,.1));
    TestEqual(TEXT("Owner's uncommitted delivery removed"),FEconomyLedger::Quantity(State,TEXT("Ship"),TEXT("Magnetite")),0.0);
    TestEqual(TEXT("Rollback cannot create ore beyond original deposit"),Rock.ReturnExtractedOre(1000000),0.0);
    TestEqual(TEXT("Repeated rollback cannot mint source ore"),Rock.ReturnExtractedOre(Taken),0.0);
    return true;
}
#endif
