#if WITH_DEV_AUTOMATION_TESTS && WITH_EDITOR
#include "CampaignFleetTransit.h"
#include "Misc/AutomationTest.h"
#include "Misc/ScopeExit.h"
#include "Sim.h"
#include "SimRegion.h"
#include "SimElement.h"
#include "Ship.h"
#include "ShipDesign.h"
#include "QuantumDrive.h"
#include "FlightDeck.h"
#include "Hangar.h"
#include "LocalGameStateSave.h"
#include "StarSystem.h"
#include "OrbitalBody.h"
#include "OrbitalRegion.h"
#include "Mission.h"
#include "DataLoader.h"
namespace
{
class FDeploymentSim:public Sim
{public:FDeploymentSim():Sim(nullptr){}using Sim::mission;using Sim::active_region;using Sim::star_system;};
class FDeploymentDrive:public QuantumDrive
{public:FDeploymentDrive():QuantumDrive(QUANTUM,100,10){}using QuantumDrive::Jump;};
class FDeploymentShip:public Ship
{
public:
 FDeploymentShip(const char* Name,ShipDesign* Design):Ship(Name,"",Design,1,0,nullptr,false){}
 FDeploymentDrive* AddDrive(){auto* Drive=new FDeploymentDrive;Drive->SetShip(this);Drive->SetPowerOn();quantum_drive=Drive;quantum_drives.append(Drive);systems.append(Drive);return Drive;}
};
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCampaignFleetTransitTest,"Starshatter.Agents.OrionGroupMeridianDeployment",
 EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FCampaignFleetTransitTest::RunTest(const FString&)
{
 if(Sim::GetSim()){AddError(TEXT("Fleet deployment test requires an idle simulation"));return false;}
 const bool OwnLoader=!DataLoader::GetLoader();if(OwnLoader)DataLoader::Initialize();ON_SCOPE_EXIT{if(OwnLoader)DataLoader::Close();};
 auto Owned=MakeUnique<FDeploymentSim>();auto& S=*Owned;
 StarSystem Janus("Janus",FVector::ZeroVector),Solus("Solus",FVector(1e12,0,0));
 OrbitalBody Janek(&Janus,"Janek",Orbital::PLANET,6e24,5400000,0),Meridian(&Solus,"Meridian",Orbital::PLANET,6e24,5400000,0);
 OrbitalRegion SourceMeta(&Janus,"Janek",0,1000000,5700000,&Janek),TargetMeta(&Solus,"Meridian",0,1000000,5700000,&Meridian);
 auto* Source=new SimRegion(&S,&SourceMeta);auto* Target=new SimRegion(&S,&TargetMeta);S.GetRegions().append(Source);S.GetRegions().append(Target);
 S.active_region=Source;S.star_system=&Janus;Mission Free(Mission::FreeFlightId);S.mission=&Free;
 ON_SCOPE_EXIT{S.GetRegions().remove(Source);S.GetRegions().remove(Target);S.active_region=nullptr;S.star_system=nullptr;S.mission=nullptr;delete Source;delete Target;};
 ShipDesign Capital;Capital.type=static_cast<int>(CLASSIFICATION::CARRIER);Capital.integrity=1000;Capital.radius=200;Capital.scale=1;
 ShipDesign Fighter;Fighter.type=static_cast<int>(CLASSIFICATION::FIGHTER);Fighter.integrity=100;Fighter.radius=20;Fighter.scale=1;
 auto* Orion=new FDeploymentShip("CV6-Orion",&Capital);auto* Escort=new FDeploymentShip("Orion Escort",&Capital);
 auto* Other=new FDeploymentShip("Unrelated group",&Capital);auto* PlayerCraft=new Ship("Alpha 1","",&Fighter,1,0,nullptr,false);
 Source->InsertObject(Orion);Source->InsertObject(Escort);Source->InsertObject(Other);Source->InsertObject(PlayerCraft);Source->SetPlayerShip(PlayerCraft);
 Orion->MoveTo(FVector(100,200,300));Escort->MoveTo(FVector(1100,200,300));Other->MoveTo(FVector(5000,200,300));
 auto* Command=S.CreateElement("Orion Group",1);Command->AddShip(Orion);
 auto* Escorts=S.CreateElement("Orion Escorts",1);Escorts->SetCommander(Command);Escorts->AddShip(Escort);
 auto* Outsiders=S.CreateElement("Other group",1);Outsiders->AddShip(Other);
 auto* CarrierDrive=Orion->AddDrive();auto* EscortDrive=Escort->AddDrive();Other->AddDrive();
 auto* Deck=new FlightDeck;Deck->SetCarrier(Orion);Deck->AddSlot(FVector(0,50,100),static_cast<uint32>(CLASSIFICATION::FIGHTER));Orion->GetFlightDecks().append(Deck);
 OrbitalRegion GroundMeta(&Janus,"Janek Station",0,1000000,5400000,&Janek);
 ShipDesign BaseDesign;BaseDesign.type=static_cast<int>(CLASSIFICATION::STATION);BaseDesign.integrity=1000;BaseDesign.radius=200;BaseDesign.scale=1;
 auto* Ground=new SimRegion(&S,&GroundMeta);S.GetRegions().append(Ground);
 ON_SCOPE_EXIT{S.GetRegions().remove(Ground);delete Ground;};
 auto* Base=new FDeploymentShip("Janek Station",&BaseDesign);Ground->InsertObject(Base);
 auto* BaseDeck=new FlightDeck;BaseDeck->SetCarrier(Base);BaseDeck->AddSlot(FVector::ZeroVector,static_cast<uint32>(CLASSIFICATION::FIGHTER));Base->GetFlightDecks().append(BaseDeck);
 Base->InitializeRuntimeHangarFromDesign();
 auto* BaseInventory=Base->GetHangar();
 if(!TestNotNull(TEXT("Janek has a real inventory observing the fighter"),BaseInventory))return false;
 BaseInventory->CreateSquadron(Text("Trainees"),nullptr,&Fighter,1,1,nullptr,0,1);
 TestTrue(TEXT("Janek inventory registers its live fighter"),BaseInventory->Stow(PlayerCraft));
 Ground->TransferObject(PlayerCraft);S.active_region=Ground;
 int GroundSlot=-1;if(!TestTrue(TEXT("Pilot begins stranded at Janek Station"),BaseDeck->Spot(PlayerCraft,GroundSlot)))return false;
 FText TransferReason;
 PlayerCraft->SetFlightPhase(EOPSMode::ACTIVE);
 TestFalse(TEXT("Return shuttle requires landing first"),CampaignFleetTransit::BoardCarrier(S,*Orion,TransferReason));
 PlayerCraft->SetFlightPhase(EOPSMode::ALERT);
 TestTrue(TEXT("Return shuttle boards the same pilot and craft on Orion"),CampaignFleetTransit::BoardCarrier(S,*Orion,TransferReason));
 TestTrue(TEXT("Return transfer activates the orbital sector"),S.GetActiveRegion()==Source && PlayerCraft->GetRegion()==Source && S.GetPlayerShip()==PlayerCraft);
 TestTrue(TEXT("Janek deck releases the transferred craft"),BaseDeck->GetShip(GroundSlot)==nullptr);
 int Squadron=-1,Slot=-1;
 auto* Inventory=Orion->GetHangar();
 TestTrue(TEXT("Transferred fighter has a real launchable inventory slot"),Inventory && Inventory->FindSquadronAndSlot(PlayerCraft,Squadron,Slot) &&
     Inventory->GetState(Inventory->GetSlot(Squadron,Slot))==Hangar::ALERT);
 TestTrue(TEXT("Repeated transfer does not duplicate the player"),CampaignFleetTransit::BoardCarrier(S,*Orion,TransferReason));
 TestTrue(TEXT("TDY shuttle can deliver a docked pilot to the surface base"),CampaignFleetTransit::BoardCarrier(S,*Base,TransferReason));
 TestTrue(TEXT("TDY arrives at Janek without starting the fleet jump"),PlayerCraft->GetRegion()==Ground && CarrierDrive->ActiveState()==QuantumDrive::ACTIVE_READY);
 TestTrue(TEXT("TDY return to Orion is an independent shuttle leg"),CampaignFleetTransit::BoardCarrier(S,*Orion,TransferReason));
 auto* Checkpoint=NewObject<ULocalUniverseState>();
 FLocalWorldStateCodec::Capture(S,*Checkpoint);
 Checkpoint->MissionName=TEXT("Janek Station"); // Older entry shell title before the shuttle return.
 TestTrue(TEXT("Fresh surface entry stages the pilot at Janek"),CampaignFleetTransit::BoardCarrier(S,*Base,TransferReason));
 TestTrue(TEXT("Carrier checkpoint restores across the free-flight entry shells"),FLocalWorldStateCodec::Restore(S,*Checkpoint));
 TestTrue(TEXT("Start Game keeps the pilot after restoring it to Orion"),S.GetPlayerShip()==PlayerCraft && S.GetActiveRegion()==Source && PlayerCraft->GetCarrier()==Orion);
 for(int32 Pass=0;Pass<3;++Pass)
 {
     TestTrue(TEXT("Repeated menu resumes restore the same carrier checkpoint"),FLocalWorldStateCodec::Restore(S,*Checkpoint));
     TestTrue(TEXT("Repeated resumes do not lose the active pilot"),S.GetPlayerShip()==PlayerCraft && S.GetActiveRegion()==Source);
 }
 Squadron=-1;Slot=-1;
 TestTrue(TEXT("Resumed fighter retains a usable Orion hangar slot"),Inventory->FindSquadronAndSlot(PlayerCraft,Squadron,Slot) && Inventory->GetState(Inventory->GetSlot(Squadron,Slot))==Hangar::ALERT);
 const FVector Offset=Escort->GetLocation()-Orion->GetLocation();
 TestEqual(TEXT("Only Orion and its assigned escort are selected"),CampaignFleetTransit::Group(*Orion).Num(),2);
 FText Reason;
 TestTrue(TEXT("Campaign deployment engages group drives"),CampaignFleetTransit::Start(S,TEXT("Solus"),TEXT("Meridian"),Reason));
 TestFalse(TEXT("Deployment has a pilot-facing message"),Reason.IsEmpty());
 CarrierDrive->Jump();EscortDrive->Jump();S.ResolveHyperList();
 TestTrue(TEXT("Orion reaches Meridian"),Orion->GetRegion()==Target);
 TestTrue(TEXT("Assigned escort reaches Meridian"),Escort->GetRegion()==Target);
 TestTrue(TEXT("Docked pilot travels with Orion"),PlayerCraft->GetRegion()==Target && S.GetPlayerShip()==PlayerCraft);
 TestTrue(TEXT("Unrelated friendly group remains in Janus"),Other->GetRegion()==Source);
 TestTrue(TEXT("Escort formation offset survives the jump"),(Escort->GetLocation()-Orion->GetLocation()).Equals(Offset,.01));
 TestTrue(TEXT("Pilot active system becomes Solus"),S.GetActiveRegion()==Target);
 auto* MeridianCheckpoint=NewObject<ULocalUniverseState>();
 FLocalWorldStateCodec::Capture(S,*MeridianCheckpoint);
 Source->TransferObject(PlayerCraft);Source->TransferObject(Orion);S.ActivateRegion(Source);
 TestTrue(TEXT("Meridian career resumes from fresh Janus entry"),FLocalWorldStateCodec::Restore(S,*MeridianCheckpoint));
 TestTrue(TEXT("Meridian resume keeps carrier and pilot in the destination sector"),S.GetActiveRegion()==Target &&
     S.GetPlayerShip()==PlayerCraft && PlayerCraft->GetRegion()==Target && Orion->GetRegion()==Target && PlayerCraft->GetCarrier()==Orion);
 // Match the packaged crash: the abandoned base is destroyed before the fighter.
 Ground->GetShips().remove(Base);Base->Destroy();delete Base;
 PlayerCraft->Notify();
 AddInfo(TEXT("Transferred fighter notification completed safely after Janek destruction."));
 return true;
}
#endif
