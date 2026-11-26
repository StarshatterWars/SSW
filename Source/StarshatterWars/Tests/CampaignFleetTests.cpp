#if WITH_DEV_AUTOMATION_TESTS && WITH_EDITOR
#include "Misc/AutomationTest.h"
#include "Misc/ScopeExit.h"
#include "CampaignFleet.h"
#include "Campaign.h"
#include "CampaignSave.h"
#include "Combatant.h"
#include "CombatGroup.h"
#include "CombatUnit.h"
#include "ShipDesignRegistry.h"
#include "Sim.h"
#include "SimRegion.h"
#include "StarSystem.h"
#include "OrbitalRegion.h"
#include "Ship.h"
#include "Mission.h"
#include "DataLoader.h"
#include "ForcesKnowledge.h"
#include "Kismet/GameplayStatics.h"
#include "UObject/StrongObjectPtr.h"
namespace
{
CombatUnit* AddFleetFixture(Campaign& Story)
{
 auto* Group=new CombatGroup(ECOMBATGROUP_TYPE::DESTROYER_SQUADRON,771,"Fixture squadron",1,EIntel::KNOWN,EEMPIRE_NAME::Terellian);
 auto* Unit=new CombatUnit("Udell","77",int32(CLASSIFICATION::DESTROYER),"Fleet test",3,1);
 Unit->SetCombatGroup(Group);Group->GetUnits().append(Unit);
 Story.GetCombatants().append(new Combatant("Fixture alliance",Group));return Unit;
}
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCampaignFleetPersistenceTest,"Starshatter.Campaign.PersistentFleet",
 EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FCampaignFleetPersistenceTest::RunTest(const FString&)
{
 const auto Old=ShipDesignRegistry::GetAll();ON_SCOPE_EXIT{ShipDesignRegistry::Clear();for(const auto& Pair:Old)ShipDesignRegistry::RegisterDesign(Pair.Key,Pair.Value);};
 FShipDesign Design;Design.ShipName=TEXT("Fleet test");Design.ShipType=int32(CLASSIFICATION::DESTROYER);Design.Integrity=1000;Design.Mass=200;ShipDesignRegistry::RegisterDesign(FName(TEXT("Fleet test")),Design);
 CombatUnit Partial("Partial squadron","",int32(CLASSIFICATION::FIGHTER),"Fleet test",12,1);Partial.SetReadyCount(2);
 TestEqual(TEXT("Small ready complement is not inflated to four craft"),Partial.AssignMission(),2);
 TestEqual(TEXT("Assigned physical fighters cannot be assigned again"),Partial.Available(),0);
 Partial.CompleteMission();TestEqual(TEXT("Returned flight restores only actual ready complement"),Partial.Available(),2);
 Campaign Story(2,"Fleet fixture",true);auto* Unit=AddFleetFixture(Story);FCampaignFleet::Prepare(Story);
 auto& Fleet=Story.GetPersistentFleet();const FString Key=FCampaignFleet::UnitKey(*Unit);
 TestTrue(TEXT("Seeded one time"),Fleet.bInitialized);TestEqual(TEXT("Every physical hull has an identity"),Fleet.Hulls.Num(),3);
 const auto Ids=Fleet.Units.FindChecked(Key).HullIds;TestTrue(TEXT("Physical hull IDs differ"),Ids[0]!=Ids[1] && Ids[1]!=Ids[2]);
 FCampaignFleet::Prepare(Story);TestTrue(TEXT("Repeated prepare preserves IDs"),Fleet.Units.FindChecked(Key).HullIds==Ids);
 auto& Damaged=Fleet.Hulls.FindChecked(Ids[1]);Damaged.Condition.bHasCondition=true;Damaged.Condition.HullCondition=.35f;
 auto& Reactor=Damaged.Condition.SystemConditions.AddDefaulted_GetRef();Reactor.SystemName=TEXT("Fusion Reactor");Reactor.Availability=0;
 TestFalse(TEXT("No functioning power means no movement"),FCampaignFleet::CanMove(Damaged));
 Fleet.bDirty=true;FCampaignFleet::Prepare(Story);TestEqual(TEXT("Disabled ship excluded from strategic live strength"),Unit->ReadyCount(),2);
 TestEqual(TEXT("Disabled is not destroyed"),Unit->DeadCount(),0);
 Reactor.Availability=.25f;TestTrue(TEXT("Enough repair releases mobility without requiring pristine hull"),FCampaignFleet::CanMove(Damaged));
 Fleet.bDirty=true;FCampaignFleet::Prepare(Story);TestEqual(TEXT("Repair readiness does not become a permanent loss"),Unit->ReadyCount(),3);
 FCampaignFleet::Destroy(Story,Ids[0]);FCampaignFleet::Prepare(Story);TestEqual(TEXT("One loss counted once"),Unit->DeadCount(),1);
 FCampaignFleet::Prepare(Story);TestEqual(TEXT("Repeated prepare cannot double-count loss"),Unit->DeadCount(),1);
 TestEqual(TEXT("Next mission resolves the surviving physical hull"),FCampaignFleet::Resolve(Story,Unit,0),Ids[1]);
 TestFalse(TEXT("Destroyed slot cannot launch"),FCampaignFleet::ResolveSlot(Story,Unit,0).IsValid());
 TestFalse(TEXT("Incidental mine destruction does not clear objective"),FCampaignFleet::ClearMinefield(Fleet,TEXT("Minefield C"),false,true));
 TestFalse(TEXT("Failed objective does not clear minefield"),FCampaignFleet::ClearMinefield(Fleet,TEXT("Minefield C"),true,false));
 TestTrue(TEXT("Successful designated destruction is permanent"),FCampaignFleet::ClearMinefield(Fleet,TEXT("Minefield C"),true,true));
 TStrongObjectPtr<UCampaignSave> Save(NewObject<UCampaignSave>());Save->CampaignIndex=2;Save->Fleet=Fleet;TArray<uint8> Bytes;
 TestTrue(TEXT("Fleet saves"),UGameplayStatics::SaveGameToMemory(Save.Get(),Bytes));
 TStrongObjectPtr<UCampaignSave> Reload(Cast<UCampaignSave>(UGameplayStatics::LoadGameFromMemory(Bytes)));if(!TestNotNull(TEXT("Fleet reloads"),Reload.Get()))return false;
 Campaign Reopened(2,"Reopened fixture",true);auto* ReopenedUnit=AddFleetFixture(Reopened);
 FCampaignFleet::Prepare(Reopened);Reload->RestoreProgress(&Reopened);FCampaignFleet::Prepare(Reopened);
 const auto& Restored=Reopened.GetPersistentFleet();TestTrue(TEXT("Reload replaces temporary seed with original IDs"),Restored.Units.FindChecked(FCampaignFleet::UnitKey(*ReopenedUnit)).HullIds==Ids);
 TestEqual(TEXT("Destroyed original remains dead"),ReopenedUnit->DeadCount(),1);
 TestEqual(TEXT("35 percent integrity persists"),Restored.Hulls.FindChecked(Ids[1]).Condition.HullCondition,.35f);
 TestEqual(TEXT("Reactor damage state persists"),Restored.Hulls.FindChecked(Ids[1]).Condition.SystemConditions[0].Availability,.25f);
 TestTrue(TEXT("Minefield objective persists"),Restored.ClearedMinefields.Contains(TEXT("Minefield C")));
 FForcesReport Ally;Ally.bAlliedOperationalAccess=true;Ally.Id=1;Ally.IFF=1;Ally.ObserverIFF=1;Ally.bReported=true;Ally.System=TEXT("Janus");Ally.Sector=TEXT("Janek");Ally.Status=TEXT("REPAIRING");Ally.HullId=Ids[1];Ally.Integrity=.35;
 auto Enemy=Ally;Enemy.bAlliedOperationalAccess=false;Enemy.Id=2;Enemy.IFF=2;Enemy.Status=TEXT("IMMOBILIZED");
 const auto Reports=FForcesKnowledge::Filter({Ally,Enemy},1,TEXT("Janus"),TEXT("Janek"),NAME_None,EForcesScope::Sector,1,{});
 if(TestEqual(TEXT("Knowledge still reports detected enemies"),Reports.Num(),2))
 {TestEqual(TEXT("Allied repair state visible"),Reports[0].Status,FString(TEXT("REPAIRING")));TestTrue(TEXT("Enemy operational state hidden in data projection"),Reports[1].Status.IsEmpty());TestFalse(TEXT("Enemy internal VIN hidden"),Reports[1].HullId.IsValid());TestEqual(TEXT("Enemy maintenance health hidden"),Reports[1].Integrity,0.0);}

 FCampaignFleet::Destroy(Reopened,Ids[2]);Reload->RestoreProgress(&Reopened);TestTrue(TEXT("Reopening a panel cannot restore an old fleet over newer losses"),Reopened.GetPersistentFleet().Hulls.FindChecked(Ids[2]).bDestroyed);
 return true;
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCampaignFleetLiveTest,"Starshatter.Campaign.PersistentFleetLiveCondition",
 EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FCampaignFleetLiveTest::RunTest(const FString&)
{
 const auto Old=ShipDesignRegistry::GetAll();ON_SCOPE_EXIT{ShipDesignRegistry::Clear();for(const auto& Pair:Old)ShipDesignRegistry::RegisterDesign(Pair.Key,Pair.Value);};
 FShipDesign Design;Design.ShipName=TEXT("Fleet test");Design.ShipType=int32(CLASSIFICATION::DESTROYER);Design.Integrity=1000;Design.Mass=200;Design.Scale=1;ShipDesignRegistry::RegisterDesign(FName(TEXT("Fleet test")),Design);
 const bool OwnLoader=!DataLoader::GetLoader();if(OwnLoader)DataLoader::Initialize();ON_SCOPE_EXIT{if(OwnLoader)DataLoader::Close();};
 TUniquePtr<Sim> Owned;if(!Sim::GetSim())Owned=MakeUnique<Sim>(nullptr);
 auto& Simulation=*Sim::GetSim();auto* OldMission=Simulation.mission;auto* OldRegion=Simulation.active_region;auto* OldSystem=Simulation.star_system;
 Mission Assignment(991);StarSystem System("Fleet fixture system",FVector::ZeroVector);OrbitalRegion Meta(&System,"Fleet fixture sector",0,1,1);
 auto* Region=new SimRegion(&Simulation,&Meta);Simulation.regions.append(Region);Simulation.mission=&Assignment;Simulation.star_system=&System;Simulation.active_region=Region;
 ON_SCOPE_EXIT{Simulation.regions.remove(Region);Simulation.active_region=OldRegion;Simulation.star_system=OldSystem;Simulation.mission=OldMission;delete Region;};
 Campaign Story(2,"Live fleet fixture",true);auto* Unit=AddFleetFixture(Story);FCampaignFleet::Prepare(Story);
 const auto Id=FCampaignFleet::Resolve(Story,Unit,0);
 auto* Vessel=Simulation.CreateShip("Udell","77",ShipDesignRegistry::FindLegacy(FName(TEXT("Fleet test"))),"Fleet fixture sector",FVector::ZeroVector,1,0,nullptr,Id);
 if(!TestNotNull(TEXT("Native ship provisions with fleet VIN"),Vessel))return false;
 Vessel->SetIntegrity(350);FCampaignFleet::Capture(Story,Simulation);
 auto& Row=Story.GetPersistentFleet().Hulls.FindChecked(Id);
 TestEqual(TEXT("Live hull damage captured independently of mission teardown"),Row.Condition.HullCondition,.35f);
 TestEqual(TEXT("Live region captured"),Row.Region,FString(TEXT("Fleet fixture sector")));
 Vessel->SetIntegrity(1000);FCampaignFleet::Apply(Story,*Vessel);
 TestEqual(TEXT("Mission provisioning cannot heal saved damage"),Vessel->GetIntegrity(),350.0);
 TestEqual(TEXT("Applying condition preserves immutable hull identity"),Vessel->GetHullId(),Id);
 Vessel->SetIntegrity(200);TestFalse(TEXT("Critically damaged live ship cannot mobilize"),FCampaignFleet::CanMove(*Vessel));
 Vessel->SetIntegrity(350);TestTrue(TEXT("Sufficient live repair restores mobility"),FCampaignFleet::CanMove(*Vessel));
 return true;
}
#endif
