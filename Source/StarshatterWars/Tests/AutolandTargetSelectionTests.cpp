#if WITH_DEV_AUTOMATION_TESTS && WITH_EDITOR
#include "Misc/AutomationTest.h"
#include "Misc/ScopeExit.h"
#include "MissionBriefingDlg.h"
#include "Ship.h"
#include "ShipDesign.h"
#include "Sim.h"
#include "SimRegion.h"
#include "SimElement.h"
#include "FlightDeck.h"
#include "ShipActor.h"
#include "DataLoader.h"
#include "Engine/World.h"
#include "Engine/GameInstance.h"
#include "UObject/StrongObjectPtr.h"
namespace
{
class FRecoverySelectionShip:public Ship
{
public:
 FRecoverySelectionShip(const char* Name,ShipDesign* Design):Ship(Name,"",Design,1,0,nullptr,false){}
 void AutoLandForFixture(bool Value){bAutoLand=Value;}
};
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FAutolandTargetSelectionTest,"Starshatter.Carrier.Recovery.ManualTargetsDuringAutoland",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FAutolandTargetSelectionTest::RunTest(const FString&)
{
 const bool OwnLoader=!DataLoader::GetLoader();if(OwnLoader)DataLoader::Initialize();ON_SCOPE_EXIT{if(OwnLoader)DataLoader::Close();};
 TUniquePtr<Sim> Owned;if(!Sim::GetSim()){AddExpectedError(TEXT("ERROR: could notxparse 'Explosions.def'"),EAutomationExpectedErrorFlags::Contains,1);Owned=MakeUnique<Sim>(nullptr);}
 auto& Simulation=*Sim::GetSim();auto* Previous=Simulation.active_region;
 auto* Region=new SimRegion(&Simulation,"Selection recovery fixture",SimRegion::REAL_SPACE);Simulation.regions.append(Region);Simulation.active_region=Region;
 ShipDesign Design;SimElement Element("Selection fixture",1);
 auto* RecoveryPilot=new FRecoverySelectionShip("Pilot",&Design);auto* Base=new Ship("Recovery base","",&Design,1,0,nullptr,false);
 Region->InsertObject(RecoveryPilot);Region->InsertObject(Base);Region->SetPlayerShip(RecoveryPilot);Element.AddShip(RecoveryPilot);RecoveryPilot->SetElement(&Element);
 auto Deck=MakeUnique<FlightDeck>();Deck->SetCarrier(Base);auto Arrival=MakeUnique<InboundSlot>(RecoveryPilot,Deck.Get(),0,0);Arrival->Clear();RecoveryPilot->SetFlightPhase(EOPSMode::ACTIVE);RecoveryPilot->SetInbound(Arrival.Get());RecoveryPilot->AutoLandForFixture(true);
 auto* World=UWorld::CreateWorld(EWorldType::Game,false);World->SetGameInstance(NewObject<UGameInstance>());
 TStrongObjectPtr<UMissionBriefingDlg> Panel(NewObject<UMissionBriefingDlg>(World));Panel->bLiveMissionStarted=true;Panel->bRecoveryTargetSelected=true;
 ON_SCOPE_EXIT{Panel->bLiveMissionStarted=false;Panel.Reset();World->DestroyWorld(false);RecoveryPilot->SetInbound(nullptr);RecoveryPilot->SetCarrier(nullptr,nullptr);Arrival.Reset();Deck.Reset();Simulation.regions.remove(Region);Simulation.active_region=Previous;delete Region;};
 auto* BaseVisual=World->SpawnActor<AShipActor>();BaseVisual->BindRuntimeShip(Base);
 for(const char* Name:{"Fighter before last","Last fighter"})
 {
  auto* Fighter=new Ship(Name,"",&Design,1,0,nullptr,false);Region->InsertObject(Fighter);auto* Visual=World->SpawnActor<AShipActor>();Visual->BindRuntimeShip(Fighter);
  // The base has already been selected by clearance. Reproduce the pilot's next manual contact selection.
  Panel->SelectedMissionTarget=Visual;RecoveryPilot->LockTarget(Fighter);
  for(int32 Tick=0;Tick<3;++Tick)Panel->SyncMissionNavigationTarget();
  TestEqual(TEXT("Manual fighter selection survives landing updates"),Panel->GetSelectedMissionContact(),static_cast<AActor*>(Visual));
  TestEqual(TEXT("Weapons/sensor target remains the selected fighter"),RecoveryPilot->GetTarget(),static_cast<SimObject*>(Fighter));
  TestTrue(TEXT("Landing focus is initialized once, not stolen every tick"),Panel->bRecoveryTargetSelected);
  TestTrue(TEXT("Inspecting another contact does not cancel autoland"),RecoveryPilot->IsAutoLandEngaged());
  TestTrue(TEXT("Landing keeps its original assigned deck"),RecoveryPilot->GetInbound()==Arrival.Get());
 }
 return true;
}
#endif
