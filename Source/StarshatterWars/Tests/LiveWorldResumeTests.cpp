#if WITH_DEV_AUTOMATION_TESTS && WITH_EDITOR
#include "Misc/AutomationTest.h"
#include "Misc/ScopeExit.h"
#include "LocalGameStateSave.h"
#include "Kismet/GameplayStatics.h"
#include "Sim.h"
#include "SimRegion.h"
#include "Ship.h"
#include "ShipDesignRegistry.h"
#include "Mission.h"
#include "Instruction.h"
#include "StarSystem.h"
#include "OrbitalRegion.h"
#include "DataLoader.h"
#include "Game.h"
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FLiveWorldResumeTest,"Starshatter.Persistence.LiveMissionWorldDiskResume",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FLiveWorldResumeTest::RunTest(const FString&)
{
 const auto Old=ShipDesignRegistry::GetAll();ON_SCOPE_EXIT{ShipDesignRegistry::Clear();for(const auto& Pair:Old)ShipDesignRegistry::RegisterDesign(Pair.Key,Pair.Value);};
 FShipDesign Design;Design.ShipName=TEXT("Resume fixture hull");Design.Integrity=1000;Design.Mass=20;Design.Scale=1;Design.ShipType=2;ShipDesignRegistry::RegisterDesign(FName(TEXT("Resume fixture hull")),Design);
 const bool OwnLoader=!DataLoader::GetLoader();if(OwnLoader)DataLoader::Initialize();ON_SCOPE_EXIT{if(OwnLoader)DataLoader::Close();};
 TUniquePtr<Sim> Owned;if(!Sim::GetSim()){AddExpectedError(TEXT("ERROR: could notxparse 'Explosions.def'"),EAutomationExpectedErrorFlags::Contains,1);Owned=MakeUnique<Sim>(nullptr);}
 auto& S=*Sim::GetSim();auto* OldMission=S.mission;auto* OldRegion=S.active_region;auto* OldSystem=S.star_system;
 Mission Assignment(101);Assignment.SetName("Resume mission fixture");StarSystem System("Resume Janus",FVector::ZeroVector);OrbitalRegion Meta(&System,"Resume sector",0,1,1);
 auto* Region=new SimRegion(&S,&Meta);S.regions.append(Region);S.mission=&Assignment;S.star_system=&System;S.active_region=Region;
 ON_SCOPE_EXIT{S.regions.remove(Region);S.active_region=OldRegion;S.star_system=OldSystem;S.mission=OldMission;delete Region;};
 auto* Pilot=S.CreateShip("Resume pilot","",ShipDesignRegistry::FindLegacy(FName(TEXT("Resume fixture hull"))),"Resume sector",FVector(100,200,300),1);
 if(!TestNotNull(TEXT("Live native pilot"),Pilot))return false;Region->SetPlayerShip(Pilot);Pilot->SetIntegrity(640);Pilot->SetVelocity(FVector(20,30,40));Pilot->SetThrottle(35);
 const FVector Destination(4000,5000,6000);auto* Nav=new Instruction(Region,OtherHand(Destination));Nav->SetSpeed(250);Nav->SetWeaponsFree(0);Pilot->AddNavPoint(Nav);
 auto* Save=NewObject<ULocalUniverseState>();FLocalWorldStateCodec::Capture(S,*Save);if(!TestEqual(TEXT("Live pilot captured"),Save->Ships.Num(),1))return false;
 const FGuid Cargo=Save->Ships[0].CargoId;const FVector Original=Pilot->GetLocation();
 const FString Slot=TEXT("AutomationLiveResume-")+FGuid::NewGuid().ToString(EGuidFormats::Digits);ON_SCOPE_EXIT{UGameplayStatics::DeleteGameInSlot(Slot,0);};
 TestTrue(TEXT("World saved to disk"),UGameplayStatics::SaveGameToSlot(Save,Slot,0));auto* Reload=Cast<ULocalUniverseState>(UGameplayStatics::LoadGameFromSlot(Slot,0));if(!TestNotNull(TEXT("World reloaded"),Reload))return false;
 Pilot->MoveTo(FVector(999,999,999));Pilot->SetIntegrity(1000);Pilot->SetVelocity(FVector::ZeroVector);Pilot->ClearFlightPlan();
 TestTrue(TEXT("Live mission restores through the production codec"),FLocalWorldStateCodec::Restore(S,*Reload));
 TestTrue(TEXT("Position resumes"),Pilot->GetLocation().Equals(Original));TestTrue(TEXT("Velocity resumes"),Pilot->GetVelocity().Equals(FVector(20,30,40)));TestEqual(TEXT("Damage resumes"),Pilot->GetIntegrity(),640.0);
 TestEqual(TEXT("Rebuilt navigation survives restart"),Pilot->GetFlightPlan().size(),1);if(Pilot->GetFlightPlan().size())TestTrue(TEXT("Edited waypoint retains exact coordinates"),Pilot->GetFlightPlan()[0]->GetLocation().Equals(Destination));
 auto* Verify=NewObject<ULocalUniverseState>();FLocalWorldStateCodec::Capture(S,*Verify);TestEqual(TEXT("Cargo identity is not regenerated on restore"),Verify->Ships[0].CargoId,Cargo);
 Assignment.SetIdentity(102);TestFalse(TEXT("Different assignment cannot consume stale world save"),FLocalWorldStateCodec::Restore(S,*Reload));return true;
}
#endif
