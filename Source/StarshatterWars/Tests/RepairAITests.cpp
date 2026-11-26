#if WITH_DEV_AUTOMATION_TESTS && WITH_EDITOR
#include "Misc/AutomationTest.h"
#include "RepairAI.h"
#include "NativeEconomyWorker.h"
#include "HAL/PlatformProcess.h"
#include "HAL/Event.h"
#include "Campaign.h"
#include "CombatUnit.h"
#include "CombatGroup.h"
#include "Combatant.h"
#include "ShipDesignRegistry.h"
#include "Misc/ScopeExit.h"
#include "CampaignFleet.h"
#include "CampaignSave.h"
#include "PlayerSaveGame.h"
#include "EconomyRuntime.h"
#include "TrafficAI.h"
#include "Kismet/GameplayStatics.h"
#include "UObject/StrongObjectPtr.h"
#include "Engine/DataTable.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
namespace
{
struct FRepairFixture
{
 FCampaignFleetState Fleet;
 FEconomyState Economy;
 FFleetRepairContext Context;
 FFleetRepairPolicy Policy;
 FGuid Id=FGuid::NewGuid();
 FRepairFixture()
 {
  Fleet.bInitialized=true;Fleet.CampaignId=2;Fleet.FleetId=FGuid::NewGuid();
  FCampaignFleetHull Hull;Hull.HullId=Id;Hull.Design=TEXT("Test fighter");Hull.Name=TEXT("Test One");Hull.Faction=1;Hull.System=TEXT("Janus");Hull.Region=TEXT("Janek");Hull.UnitKey=TEXT("Test squadron");
  Hull.Condition.bHasCondition=true;Hull.Condition.HullCondition=.2f;Hull.Condition.SavedWeaponAmmo.Add(1,5);
  auto& Engine=Hull.Condition.SystemConditions.AddDefaulted_GetRef();Engine.SystemName=TEXT("Main Drive");Engine.SystemIndex=0;Engine.Availability=0;
  auto& Reactor=Hull.Condition.SystemConditions.AddDefaulted_GetRef();Reactor.SystemName=TEXT("Fusion Reactor");Reactor.SystemIndex=1;Reactor.Availability=0;
  auto& Part=Hull.Condition.Components.AddDefaulted_GetRef();Part.Component=TEXT("Fuel injector");Part.Level=5;Part.bHasCondition=true;Part.ConditionPercent=0;
  Fleet.Hulls.Add(Id,Hull);Fleet.Units.FindOrAdd(Hull.UnitKey).HullIds.Add(Id);
  FRepairHullSpec Spec;Spec.Mass=8;Spec.bFighter=true;Context.Designs.Add(Hull.Design,Spec);
  Context.Now=1000;Context.SupplySystem=TEXT("Janus");Policy.RepairSecondsPerMassRoot=100;Policy.RebuildSecondsPerMassRoot=300;
  Policy.InitialParts=100;Policy.InitialAlloy=500;Policy.InitialKits=20;
  Economy.Wallets.Add(FEconomyLedger::Pilot(),5000);
 }
 FFleetRepairOrder& Job(){for(auto& Pair:Economy.FleetRepairOrders)if(Pair.Value.HullId==Id && !Pair.Value.IsFinished())return Pair.Value;return Economy.FleetRepairOrders.CreateIterator().Value();}
};
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRepairResourceTest,"Starshatter.Campaign.RepairAI.ResourcesAndRollback",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FRepairResourceTest::RunTest(const FString&)
{
 FRepairFixture F;FFleetRepairStep Step;
 TestFalse(TEXT("Failed persistence rejects work and debit"),RepairAI::Commit(F.Fleet,F.Economy,F.Context,F.Policy,[](){return false;},Step));
 TestEqual(TEXT("No unpaid order survives rollback"),F.Economy.FleetRepairOrders.Num(),0);
 TestEqual(TEXT("No supply grant escapes failed transaction"),F.Economy.Inventories.Num(),0);
 TestEqual(TEXT("Failure never heals the hull"),F.Fleet.Hulls.FindChecked(F.Id).Condition.HullCondition,.2f);
 TestTrue(TEXT("Valid work persists"),RepairAI::Commit(F.Fleet,F.Economy,F.Context,F.Policy,[](){return true;},Step));
 TestTrue(TEXT("Repair scheduled a later milestone"),Step.NextAt>F.Context.Now);
 const auto Depot=RepairAI::Depot(F.Fleet,1,TEXT("Janus"));const auto Parts=DataTableUtils::MakeValidName(TEXT("military spare parts"));
 TestEqual(TEXT("Finite initial load minus two tonnes of parts"),FEconomyLedger::Quantity(F.Economy,Depot,Parts),98.);
 RepairAI::Advance(F.Fleet,F.Economy,F.Context,F.Policy);
 TestEqual(TEXT("Duplicate wake never consumes twice"),FEconomyLedger::Quantity(F.Economy,Depot,Parts),98.);
 TestEqual(TEXT("Fleet does not charge pilot"),F.Economy.Wallets.FindRef(FEconomyLedger::Pilot()),5000LL);
 F.Economy.Inventories.Remove(Depot);RepairAI::Advance(F.Fleet,F.Economy,F.Context,F.Policy);
 TestEqual(TEXT("Recreating a lost depot cannot repeat initial grant"),FEconomyLedger::Quantity(F.Economy,Depot,Parts),0.);
 return true;
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRepairSurvivorTest,"Starshatter.Campaign.RepairAI.SurvivalAndSaveReconciliation",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FRepairSurvivorTest::RunTest(const FString&)
{
 FRepairFixture F;F.Fleet.FleetId.Invalidate();const auto BeforeFleet=F.Fleet;
 RepairAI::Advance(F.Fleet,F.Economy,F.Context,F.Policy);const FGuid Order=F.Job().Id;
 F.Context.Now+=F.Job().DurationSeconds*.25;RepairAI::Advance(F.Fleet,F.Economy,F.Context,F.Policy);
 const auto& Hull=F.Fleet.Hulls.FindChecked(F.Id);
 TestTrue(TEXT("Quarter-repair reaches mobility thresholds"),FCampaignFleet::CanMove(Hull));
 TestEqual(TEXT("Repair retains permanent hull identity"),Hull.HullId,F.Id);
 TestEqual(TEXT("Repair preserves component grade"),Hull.Condition.Components[0].Level,5);
 TestEqual(TEXT("Repair does not grant missiles"),Hull.Condition.SavedWeaponAmmo.FindRef(1),5);
 TStrongObjectPtr<UPlayerSaveGame> Save(NewObject<UPlayerSaveGame>());Save->PlayerInfo.Economy=F.Economy;TArray<uint8> Bytes;
 TestTrue(TEXT("Paid work saves"),UGameplayStatics::SaveGameToMemory(Save.Get(),Bytes));
 TStrongObjectPtr<UPlayerSaveGame> Reload(Cast<UPlayerSaveGame>(UGameplayStatics::LoadGameFromMemory(Bytes)));if(!TestNotNull(TEXT("Paid work reloads"),Reload.Get()))return false;
 auto OldFleet=BeforeFleet;auto ReloadedEconomy=Reload->PlayerInfo.Economy;
 RepairAI::Advance(OldFleet,ReloadedEconomy,F.Context,F.Policy);
 TestEqual(TEXT("Durable work reconciles an older campaign save"),OldFleet.Hulls.FindChecked(F.Id).Condition.HullCondition,Hull.Condition.HullCondition);
 TestEqual(TEXT("Reconciliation retains one work order"),ReloadedEconomy.FleetRepairOrders.Num(),1);
 F.Context.Now+=10000;RepairAI::Advance(F.Fleet,F.Economy,F.Context,F.Policy);
 TestEqual(TEXT("Elapsed campaign time finishes work"),F.Economy.FleetRepairOrders.FindChecked(Order).State,EFleetRepairState::Completed);
 TestEqual(TEXT("Completed repairs restore hull"),F.Fleet.Hulls.FindChecked(F.Id).Condition.HullCondition,1.f);
 TestEqual(TEXT("Completed repairs restore component"),F.Fleet.Hulls.FindChecked(F.Id).Condition.Components[0].ConditionPercent,100.f);
 return true;
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRepairReplacementTest,"Starshatter.Campaign.RepairAI.ReplacementIdentityAndShipyard",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FRepairReplacementTest::RunTest(const FString&)
{
 FRepairFixture F;F.Fleet.Hulls.FindChecked(F.Id).bDestroyed=true;F.Fleet.LastAppliedUnavailable.Add(TEXT("Test squadron"),1);
 RepairAI::Advance(F.Fleet,F.Economy,F.Context,F.Policy);
 TestEqual(TEXT("No shipyard means no replacement"),F.Job().State,EFleetRepairState::Shipyard);
 TestFalse(TEXT("Waiting for yard consumes no resources"),F.Job().bResourcesConsumed);
 const FName Yard=TEXT("Janus/Shipworks/Output");auto* Store=FEconomyLedger::CreateInventory(F.Economy,Yard,FEconomyRuntime::IndustryOwner(),TEXT("Janus"),10000);Store->bHullStorage=Store->bSupplyOnly=true;
 RepairAI::Advance(F.Fleet,F.Economy,F.Context,F.Policy);TestFalse(TEXT("An unauthorised foreign yard cannot rebuild"),F.Job().bResourcesConsumed);
 F.Context.AuthorizedShipyards.Add(Yard);RepairAI::Advance(F.Fleet,F.Economy,F.Context,F.Policy);
 const auto OrderId=F.Job().Id,Replacement=F.Job().ReplacementId;
 TestTrue(TEXT("Operational allied yard starts construction"),F.Job().bResourcesConsumed);
 TestTrue(TEXT("Replacement VIN allocated before work persists"),Replacement.IsValid() && Replacement!=F.Id);
 F.Context.Now+=10000;RepairAI::Advance(F.Fleet,F.Economy,F.Context,F.Policy);
 TestTrue(TEXT("Original hull remains destroyed"),F.Fleet.Hulls.FindChecked(F.Id).bDestroyed);
 TestEqual(TEXT("Only active physical slot is replaced"),F.Fleet.Units.FindChecked(TEXT("Test squadron")).HullIds[0],Replacement);
 if(!TestTrue(TEXT("New physical hull exists"),F.Fleet.Hulls.Contains(Replacement)))return false;
 const auto& NewHull=F.Fleet.Hulls.FindChecked(Replacement);
 TestEqual(TEXT("Replacement history names destroyed original"),NewHull.Replaces,F.Id);
 TestEqual(TEXT("Fleet replacement uses Level III MILSPEC"),NewHull.Condition.Components[0].Level,3);
 TestTrue(TEXT("Replacement can move"),FCampaignFleet::CanMove(NewHull));
 RepairAI::Advance(F.Fleet,F.Economy,F.Context,F.Policy);
 TestEqual(TEXT("Repeated completion cannot duplicate hull"),F.Fleet.Hulls.Num(),2);
 TestEqual(TEXT("Original strategic loss retained until native readiness reconciliation"),F.Fleet.LastAppliedUnavailable.FindRef(TEXT("Test squadron")),1);
 TestEqual(TEXT("Completed construction remains durable"),F.Economy.FleetRepairOrders.FindChecked(OrderId).State,EFleetRepairState::Completed);
 return true;
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRepairPauseTest,"Starshatter.Campaign.RepairAI.DeploymentDamageAndClock",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FRepairPauseTest::RunTest(const FString&)
{
 FRepairFixture F;RepairAI::Advance(F.Fleet,F.Economy,F.Context,F.Policy);const auto Order=F.Job().Id;
 F.Context.Deployed.Add(F.Id);F.Context.Now+=50;RepairAI::Advance(F.Fleet,F.Economy,F.Context,F.Policy);
 TestEqual(TEXT("Deployment suspends repair"),F.Job().State,EFleetRepairState::Paused);
 F.Context.Now+=5000;RepairAI::Advance(F.Fleet,F.Economy,F.Context,F.Policy);
 TestEqual(TEXT("Flying does not accumulate repair time"),F.Job().WorkedSeconds,0.);
 F.Context.Deployed.Reset();RepairAI::Advance(F.Fleet,F.Economy,F.Context,F.Policy);
 F.Context.Now-=100;RepairAI::Advance(F.Fleet,F.Economy,F.Context,F.Policy);
 TestEqual(TEXT("Clock rollback grants no progress"),F.Job().WorkedSeconds,0.);
 F.Fleet.Hulls.FindChecked(F.Id).Condition.HullCondition=.1f;RepairAI::Advance(F.Fleet,F.Economy,F.Context,F.Policy);
 TestEqual(TEXT("New damage cancels stale work"),F.Economy.FleetRepairOrders.FindChecked(Order).State,EFleetRepairState::Cancelled);
 TestEqual(TEXT("No stale baseline heals new damage"),F.Fleet.Hulls.FindChecked(F.Id).Condition.HullCondition,.1f);
 F.Fleet.Hulls.FindChecked(F.Id).bHero=true;F.Fleet.Hulls.FindChecked(F.Id).bDestroyed=true;F.Fleet.bHeroLost=true;
 F.Context.Now+=10000;RepairAI::Advance(F.Fleet,F.Economy,F.Context,F.Policy);
 TestEqual(TEXT("Destroyed campaign hero cannot be rebuilt"),F.Fleet.Hulls.Num(),1);
 return true;
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRepairShortageTest,"Starshatter.Campaign.RepairAI.ShortagesAndPolicy",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FRepairShortageTest::RunTest(const FString&)
{
 FRepairFixture F;F.Policy.InitialParts=F.Policy.InitialAlloy=F.Policy.InitialKits=0;
 const auto Step=RepairAI::Advance(F.Fleet,F.Economy,F.Context,F.Policy);
 TestEqual(TEXT("Empty stores block work"),F.Job().State,EFleetRepairState::Supplies);
 TestTrue(TEXT("Missing resources are explained"),F.Job().Reason.Contains(TEXT("AWAITING SUPPLIES")));
 TestEqual(TEXT("Blocked work has no frame polling deadline"),Step.NextAt,0.);
 F.Context.Now+=100000;RepairAI::Advance(F.Fleet,F.Economy,F.Context,F.Policy);
 TestEqual(TEXT("Waiting consumes time without healing"),F.Fleet.Hulls.FindChecked(F.Id).Condition.HullCondition,.2f);
 const FName Depot=RepairAI::Depot(F.Fleet,1,TEXT("Janus"));
 const auto Materials=F.Job().Materials;for(const auto& Pair:Materials)FEconomyLedger::Deposit(F.Economy,Depot,Pair.Key,Pair.Value,1);
 RepairAI::Advance(F.Fleet,F.Economy,F.Context,F.Policy);TestEqual(TEXT("Delivered supplies release work"),F.Job().State,EFleetRepairState::Running);
 FString Json;FText Error;FFleetRepairPolicy Policy;
 TestTrue(TEXT("Shipped repair policy readable"),FFileHelper::LoadFileToString(Json,*(FPaths::ProjectContentDir()/TEXT("GameData/Production/fleet_repairs.json"))));
 TestTrue(TEXT("Shipped repair policy validates"),Policy.LoadJson(Json,Error));
 TestFalse(TEXT("Invalid policy is rejected"),Policy.LoadJson(TEXT("{}"),Error));
 return true;
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRepairOOBIntegrationTest,"Starshatter.Campaign.RepairAI.NativeRosterReplacement",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FRepairOOBIntegrationTest::RunTest(const FString&)
{
 const auto Old=ShipDesignRegistry::GetAll();ON_SCOPE_EXIT{ShipDesignRegistry::Clear();for(const auto& Pair:Old)ShipDesignRegistry::RegisterDesign(Pair.Key,Pair.Value);};
 FShipDesign Design;Design.ShipName=TEXT("Repair fixture");Design.ShipType=int32(CLASSIFICATION::FIGHTER);Design.Integrity=1000;Design.Mass=8;ShipDesignRegistry::RegisterDesign(FName(TEXT("Repair fixture")),Design);
 Campaign Story(2,"Repair fixture",true);auto* Group=new CombatGroup(ECOMBATGROUP_TYPE::DESTROYER_SQUADRON,887,"Repair test squadron",1,EIntel::KNOWN,EEMPIRE_NAME::Terellian);
 auto* Unit=new CombatUnit("Repair test","",int32(CLASSIFICATION::FIGHTER),"Repair fixture",1,1);Unit->SetCombatGroup(Group);Unit->SetRegion("Janek");Group->GetUnits().append(Unit);Group->SetAssignedSystem("Janus");Story.GetCombatants().append(new Combatant("Repair alliance",Group));
 FCampaignFleet::Prepare(Story);const FString Key=FCampaignFleet::UnitKey(*Unit);const FGuid Original=FCampaignFleet::Resolve(Story,Unit,0);
 FCampaignFleet::Destroy(Story,Original);FCampaignFleet::Prepare(Story);TestEqual(TEXT("Native roster initially records loss"),Unit->DeadCount(),1);
 FRepairFixture F;F.Context.Designs.Reset();FRepairHullSpec Spec;Spec.Mass=8;Spec.bFighter=true;F.Context.Designs.Add(TEXT("Repair fixture"),Spec);
 auto* Yard=FEconomyLedger::CreateInventory(F.Economy,TEXT("Janus/Test shipyard/Output"),FEconomyRuntime::IndustryOwner(),TEXT("Janus"),10000);Yard->bHullStorage=Yard->bSupplyOnly=true;F.Context.AuthorizedShipyards.Add(TEXT("Janus/Test shipyard/Output"));
 auto& Fleet=Story.GetPersistentFleet();RepairAI::Advance(Fleet,F.Economy,F.Context,F.Policy);F.Context.Now+=10000;RepairAI::Advance(Fleet,F.Economy,F.Context,F.Policy);FCampaignFleet::Prepare(Story);
 const FGuid Replacement=Fleet.Units.FindChecked(Key).HullIds[0];
 TestTrue(TEXT("Native fleet slot gets new physical identity"),Replacement!=Original);
 TestTrue(TEXT("Original history remains destroyed"),Fleet.Hulls.FindChecked(Original).bDestroyed);
 TestEqual(TEXT("Strategic loss reconciles without killing replacement"),Unit->DeadCount(),0);
 TestEqual(TEXT("Mission generation sees restored fighter complement"),Unit->ReadyCount(),1);
 TestEqual(TEXT("Mission resolver returns replacement identity"),FCampaignFleet::Resolve(Story,Unit,0),Replacement);
 return true;
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRepairSupplyRouteTest,"Starshatter.Campaign.RepairAI.DepotsAndFreight",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FRepairSupplyRouteTest::RunTest(const FString&)
{
 FRepairFixture F;RepairAI::Advance(F.Fleet,F.Economy,F.Context,F.Policy);
 const FName Home=RepairAI::Depot(F.Fleet,1,TEXT("Janus")),Parts=DataTableUtils::MakeValidName(TEXT("military spare parts"));
 const double Remaining=FEconomyLedger::Quantity(F.Economy,Home,Parts);
 F.Context.SupplySystem=TEXT("Solus");RepairAI::Advance(F.Fleet,F.Economy,F.Context,F.Policy);
 const FName Away=RepairAI::Depot(F.Fleet,1,TEXT("Solus"));
 TestEqual(TEXT("New system does not mint a second fleet allocation"),FEconomyLedger::Quantity(F.Economy,Away,Parts),0.);
 TestEqual(TEXT("Supplies do not teleport to another system"),FEconomyLedger::Quantity(F.Economy,Home,Parts),Remaining);
 F.Economy.Inventories.FindChecked(Home).DesiredStock.Add(Parts,Remaining+10);
 const FName Source=TEXT("Janus/Parts Factory/Output");auto* Store=FEconomyLedger::CreateInventory(F.Economy,Source,FEconomyRuntime::IndustryOwner(),TEXT("Janus"),1000);Store->TradingOwner=TEXT("Factory business");FEconomyLedger::Deposit(F.Economy,Source,Parts,10,1);
 FGalaxyTrafficEndpoint From;From.Id=TEXT("Parts Factory");From.Station=TEXT("Parts Factory");From.System=TEXT("Janus");From.Sector=TEXT("Janus/Factory");
 FGalaxyTrafficEndpoint To;To.Id=TEXT("Janus Transfer");To.Station=TEXT("Janus Transfer Station");To.System=TEXT("Janus");To.Sector=TEXT("Janus/Transfer");
 FGalaxyTrafficSector A;A.Id=From.Sector;A.System=TEXT("Janus");A.Neighbors.Add(To.Sector);FGalaxyTrafficSector B;B.Id=To.Sector;B.System=TEXT("Janus");B.Neighbors.Add(From.Sector);
 TMap<FName,FGalaxyTrafficSector> Graph;Graph.Add(A.Id,A);Graph.Add(B.Id,B);TMap<FName,FGalaxyTrafficEndpoint> Endpoints;Endpoints.Add(From.Id,From);Endpoints.Add(To.Id,To);
 FGalaxyTrafficState Traffic;TrafficAI::Dispatch(Traffic,F.Economy,Graph,Endpoints,A.Id,1);
 auto* Journey=Traffic.Journeys.FindByPredicate([&](const auto& J){return J.DestinationInventory==Home;});
 if(!TestNotNull(TEXT("Existing freight AI schedules fleet resupply"),Journey))return false;
 TestEqual(TEXT("Booking freight does not credit repair stores"),FEconomyLedger::Quantity(F.Economy,Home,Parts),Remaining);
 for(int32 I=0;I<8 && !Journey->IsFinished();++I)TrafficAI::Advance(*Journey,F.Economy,1000,false,true,10,10);
 TestEqual(TEXT("Actual unloading replenishes finite fleet supplies"),FEconomyLedger::Quantity(F.Economy,Home,Parts),Remaining+10);
 return true;
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FFleetRepairWorkerTest,"Starshatter.Domains.Repair.WorkerIsolationRollbackAndStaleFleet",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FFleetRepairWorkerTest::RunTest(const FString&)
{
 FRepairFixture F;auto Worker=MakeUnique<FNativeEconomyWorker>();FEvent* Done=FPlatformProcess::GetSynchEventFromPool(false);
 ON_SCOPE_EXIT{Worker.Reset();FPlatformProcess::ReturnSynchEventToPool(Done);};
 const auto Calculate=[&]()->TUniquePtr<FNativeEconomyResult>
 {
  FNativeEconomyInput Input;Input.State=F.Economy;Input.Profile=TEXT("Repair/session1");Input.Revision=1;
  Input.Repair=MakeUnique<FNativeFleetRepairCommand>();Input.Repair->Fleet=F.Fleet;Input.Repair->Context=F.Context;Input.Repair->Policy=F.Policy;
  if(!Worker->Submit(MoveTemp(Input),[Done](){Done->Trigger();}) || !Done->Wait(10000))return nullptr;return Worker->TakeResult();
 };
 auto Result=Calculate();if(!TestNotNull(TEXT("Native repair result"),Result.Get()))return false;
 TestTrue(TEXT("Repair executes off game thread"),Result->bOffGameThread);TestTrue(TEXT("Repair work proposed"),Result->Repair && Result->Repair->Step.bChanged);
 TestTrue(TEXT("Live supplies unchanged before acceptance"),F.Economy.FleetRepairOrders.IsEmpty());TestFalse(TEXT("Live hull not yet repairing"),F.Fleet.Hulls.FindChecked(F.Id).bRepairing);
 FText Why;
 TestFalse(TEXT("Generic commit cannot apply half a repair"),FNativeEconomyWorker::Commit(MoveTemp(*Result),F.Economy,1,TEXT("Repair/session1")));
 TestFalse(TEXT("Wrong profile rejected"),FNativeEconomyWorker::CommitRepair(MoveTemp(*Result),F.Economy,F.Fleet,1,TEXT("Repair/session2"),[]{return true;},Why));
 TestFalse(TEXT("Changed supplies rejected"),FNativeEconomyWorker::CommitRepair(MoveTemp(*Result),F.Economy,F.Fleet,2,TEXT("Repair/session1"),[]{return true;},Why));
 F.Fleet.Hulls.FindChecked(F.Id).bDestroyed=true;
 TestFalse(TEXT("Hull destruction rejects old result"),FNativeEconomyWorker::CommitRepair(MoveTemp(*Result),F.Economy,F.Fleet,1,TEXT("Repair/session1"),[]{return true;},Why));F.Fleet.Hulls.FindChecked(F.Id).bDestroyed=false;
 TestFalse(TEXT("Failed save rejects repair"),FNativeEconomyWorker::CommitRepair(MoveTemp(*Result),F.Economy,F.Fleet,1,TEXT("Repair/session1"),[]{return false;},Why));
 TestTrue(TEXT("Failed save rolls back work and supply seed"),F.Economy.FleetRepairOrders.IsEmpty() && F.Economy.Inventories.IsEmpty());TestFalse(TEXT("Failed save never heals fleet"),F.Fleet.Hulls.FindChecked(F.Id).bRepairing);
 Result=Calculate();if(!Result)return false;
 TestTrue(TEXT("Saved work updates fleet and economy together"),FNativeEconomyWorker::CommitRepair(MoveTemp(*Result),F.Economy,F.Fleet,1,TEXT("Repair/session1"),[]{return true;},Why));
 TestTrue(TEXT("Repairing state projected after saved debit"),F.Fleet.Hulls.FindChecked(F.Id).bRepairing);TestEqual(TEXT("Only one work order"),F.Economy.FleetRepairOrders.Num(),1);
 TestFalse(TEXT("Result consumed once"),FNativeEconomyWorker::CommitRepair(MoveTemp(*Result),F.Economy,F.Fleet,1,TEXT("Repair/session1"),[]{return true;},Why));
 TestEqual(TEXT("Pilot not charged for fleet repairs"),F.Economy.Wallets.FindRef(FEconomyLedger::Pilot()),5000LL);
 return true;
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FFleetRepairWorkerCompletionTest,"Starshatter.Domains.Repair.CompletionAndDeployedHull",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FFleetRepairWorkerCompletionTest::RunTest(const FString&)
{
 FRepairFixture F;FNativeEconomyInput In;In.State=F.Economy;In.Profile=TEXT("repair");In.Revision=1;In.Repair=MakeUnique<FNativeFleetRepairCommand>();
 In.Repair->Fleet=F.Fleet;In.Repair->Context=F.Context;In.Repair->Policy=F.Policy;
 auto Started=FNativeEconomyWorker::Calculate(MoveTemp(In));FText Why;TestTrue(TEXT("Start saved"),FNativeEconomyWorker::CommitRepair(MoveTemp(Started),F.Economy,F.Fleet,1,TEXT("repair"),[]{return true;},Why));
 F.Context.Now+=100000;FNativeEconomyInput Finish;Finish.State=F.Economy;Finish.Profile=TEXT("repair");Finish.Revision=1;Finish.Repair=MakeUnique<FNativeFleetRepairCommand>();
 Finish.Repair->Fleet=F.Fleet;Finish.Repair->Context=F.Context;Finish.Repair->Policy=F.Policy;
 auto Finished=FNativeEconomyWorker::Calculate(MoveTemp(Finish));TestTrue(TEXT("Completion saved"),FNativeEconomyWorker::CommitRepair(MoveTemp(Finished),F.Economy,F.Fleet,1,TEXT("repair"),[]{return true;},Why));
 const auto& Hull=F.Fleet.Hulls.FindChecked(F.Id);TestEqual(TEXT("Hull restored"),Hull.Condition.HullCondition,1.f);
 TestEqual(TEXT("Missiles not granted"),Hull.Condition.SavedWeaponAmmo.FindRef(1),5);TestEqual(TEXT("Component quality preserved"),Hull.Condition.Components[0].Level,5);
 FRepairFixture Deployed;Deployed.Context.Deployed.Add(Deployed.Id);FNativeEconomyInput Blocked;Blocked.State=Deployed.Economy;Blocked.Repair=MakeUnique<FNativeFleetRepairCommand>();
 Blocked.Repair->Fleet=Deployed.Fleet;Blocked.Repair->Context=Deployed.Context;Blocked.Repair->Policy=Deployed.Policy;
 auto Skipped=FNativeEconomyWorker::Calculate(MoveTemp(Blocked));TestFalse(TEXT("Deployed hull not healed"),Skipped.Repair->Fleet.Hulls.FindChecked(Deployed.Id).bRepairing);
 return true;
}
#endif
