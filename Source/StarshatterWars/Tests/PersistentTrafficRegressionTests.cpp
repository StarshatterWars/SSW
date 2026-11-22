#if WITH_DEV_AUTOMATION_TESTS && WITH_EDITOR
#include "CommerceCharges.h"
#include "Misc/AutomationTest.h"
#include "Misc/ScopeExit.h"
#include "Misc/SecureHash.h"
#include "LocalGameStateSubsystem.h"
#include "Kismet/GameplayStatics.h"
#include "TrafficAI.h"
#include "Engine/GameInstance.h"
#include "EconomyRuntime.h"
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FAlternateBuyerTest,"Starshatter.Traffic.AlternateBuyerAtomicSettlement",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FAlternateBuyerTest::RunTest(const FString&)
{
 FEconomyState E;FGalaxyTrafficState T;const FName Owner=FEconomyRuntime::IndustryOwner(),Ore(TEXT("ore"));
 FGalaxyTrafficEndpoint A;A.Id=TEXT("Janus/Mine");A.Sector=TEXT("Janus/Belt");A.System=TEXT("Janus");A.Station=TEXT("Mine");
 auto B=A;B.Id=TEXT("Janus/Closed");B.Station=TEXT("Closed");B.Sector=TEXT("Janus/Orbit");
 auto C=B;C.Id=TEXT("Janus/Buyer");C.Station=TEXT("Buyer");
 const FName Source(TEXT("Janus/Mine/Output")),Old(TEXT("Janus/Closed/Input")),New(TEXT("Janus/Buyer/Input"));
 for(FName Key:{Source,Old,New})FEconomyLedger::CreateInventory(E,Key,Owner,TEXT("Janus"),100);
 FEconomyLedger::Deposit(E,Source,Ore,20,1);E.Inventories[Source].TradingOwner=TEXT("Seller");E.Inventories[Source].AskPrices.Add(Ore,10);
 for(FName Key:{Old,New}){auto& I=E.Inventories[Key];I.TradingOwner=Key;I.DesiredStock.Add(Ore,20);I.BidPrices.Add(Ore,20);E.Wallets.Add(Key,1000);}
 const auto Id=TrafficAI::Schedule(T,E,A,B,{A.Sector,B.Sector},Source,Old,Ore,8,1);if(!TestTrue(TEXT("Real pickup reserves stock and escrow"),Id.IsValid()))return false;
 auto& J=T.Journeys[0];TrafficAI::Advance(J,E,10,false,false,10,1);TrafficAI::Advance(J,E,10,false,false,10,1);
 E.Inventories[Old].bOperational=false;TrafficAI::Advance(J,E,60,false,false,10,1);
 TMap<FName,FGalaxyTrafficSector> Graph;FGalaxyTrafficSector N;N.Id=A.Sector;N.Neighbors={B.Sector};Graph.Add(N.Id,N);N.Id=B.Sector;N.Neighbors={A.Sector};Graph.Add(N.Id,N);
 TMap<FName,FGalaxyTrafficEndpoint> Points;for(const auto& P:{A,B,C})Points.Add(P.Id,P);
 TestEqual(TEXT("Available buyer wins before return-to-origin"),TrafficAI::RecoverBlocked(T,E,Graph,Points),1);
 TestFalse(TEXT("Replacement is a delivery, not a return"),J.bReturning);TestEqual(TEXT("Replacement destination"),J.DestinationInventory,New);
 TestEqual(TEXT("First buyer refunded"),E.Wallets.FindRef(Old),1000LL);TestEqual(TEXT("Second buyer escrows same supplier price"),E.Wallets.FindRef(New),920LL);
 TestEqual(TEXT("Rerouting alone credits no cargo"),FEconomyLedger::Quantity(E,New,Ore),0.0);TestEqual(TEXT("Supplier remains unpaid until unload"),E.Wallets.FindRef(TEXT("Seller")),0LL);
 for(int32 I=0;I<5;++I)TrafficAI::Advance(J,E,100,false,false,10,1);
 TestEqual(TEXT("New buyer receives cargo once"),FEconomyLedger::Quantity(E,New,Ore),8.0);TestEqual(TEXT("Supplier settled once"),E.Wallets.FindRef(TEXT("Seller")),CommerceCharges::Quote(80,ECommercePurpose::Purchase).Base);
 TrafficAI::Advance(J,E,100,false,false,10,1);TestEqual(TEXT("Retry cannot duplicate payout"),E.Wallets.FindRef(TEXT("Seller")),CommerceCharges::Quote(80,ECommercePurpose::Purchase).Base);
 return true;
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPatrolPolicyTest,"Starshatter.Traffic.FighterPatrolPolicyAndPersistence",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FPatrolPolicyTest::RunTest(const FString&)
{
 TestTrue(TEXT("Free flight permits regional patrols"),TrafficAI::PatrolAllowed(true,false,false,false));
 TestFalse(TEXT("Training protected"),TrafficAI::PatrolAllowed(true,true,false,false));TestFalse(TEXT("Authored missions protected"),TrafficAI::PatrolAllowed(false,false,false,false));TestFalse(TEXT("Match networking protected"),TrafficAI::PatrolAllowed(true,false,true,false));TestFalse(TEXT("Cutscenes protected"),TrafficAI::PatrolAllowed(true,false,false,true));
 FGalaxyTrafficState T;TArray<FPatrolHull> Hulls{{TEXT("Falcon"),1},{TEXT("Raptor"),2},{TEXT("Cutlass"),3}};TSet<int32> Seen;
 for(int32 I=0;I<180;++I){FGalaxyTrafficSector S;S.Id=FName(*FString::Printf(TEXT("System/Sector%d"),I));S.Affiliation=I%2+1;auto Id=TrafficAI::SchedulePatrol(T,S,Hulls,200);if(!TestTrue(TEXT("Native patrol record scheduled without UE actor"),Id.IsValid()))return false;Seen.Add(T.Journeys.Last().PatrolIFF);TestFalse(TEXT("Same sector cannot duplicate active patrol"),TrafficAI::SchedulePatrol(T,S,Hulls,200).IsValid());}
 for(int32 IFF:{0,1,2,3})TestTrue(TEXT("Regional pool includes neutral, Alliance, Marakan and pirate contacts"),Seen.Contains(IFF));
 auto& P=T.Journeys[0];P.HullFraction=.4;P.PatrolAmmo={7,2};auto* Save=NewObject<ULocalPilotState>();Save->Pilot.Traffic=T;TArray<uint8> Bytes;TestTrue(TEXT("Patrol state serializes"),UGameplayStatics::SaveGameToMemory(Save,Bytes));auto* Reload=Cast<ULocalPilotState>(UGameplayStatics::LoadGameFromMemory(Bytes));if(!TestNotNull(TEXT("Patrol reload"),Reload))return false;
 const auto& R=Reload->Pilot.Traffic.Journeys[0];TestEqual(TEXT("Identity retained"),R.Id,P.Id);TestEqual(TEXT("Damage retained"),R.HullFraction,.4);TestEqual(TEXT("Ammunition retained"),R.PatrolAmmo[0],7);return true;
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCheckpointInterruptionTest,"Starshatter.Persistence.InterruptedAndExitDuringAutosave",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FCheckpointInterruptionTest::RunTest(const FString&)
{
 auto* Instance=NewObject<UGameInstance>();auto* Service=NewObject<ULocalGameStateSubsystem>(Instance);const FString Profile=TEXT("Automation-")+FGuid::NewGuid().ToString(EGuidFormats::Digits);TArray<FString> Cleanup;
 const FString Manifest=TEXT("State_")+FMD5::HashAnsiString(*Profile)+TEXT("_manifest");Cleanup.Add(Manifest);Cleanup.Add(Manifest+TEXT("_previous"));
 ON_SCOPE_EXIT{for(const auto& Slot:Cleanup)UGameplayStatics::DeleteGameInSlot(Slot,0);};
 auto Prepare=[&](int64 Balance)
 {
  Service->PendingManifest=NewObject<ULocalStateManifest>(Service);auto& M=*Service->PendingManifest;M.Generation=FGuid::NewGuid();M.ProfileSlot=Profile;
  auto* P=NewObject<ULocalPilotState>(Service);auto* E=NewObject<ULocalEconomyState>(Service);auto* K=NewObject<ULocalSkillState>(Service);auto* W=NewObject<ULocalUniverseState>(Service);E->Economy.Wallets.Add(FEconomyLedger::Pilot(),Balance);W->MissionId=101;
  Service->PendingAreas={P,E,K,W};const TCHAR* Names[]={TEXT("pilot"),TEXT("economy"),TEXT("skills"),TEXT("universe")};for(int32 I=0;I<4;++I){auto* Area=Service->PendingAreas[I].Get();Area->ProfileSlot=Profile;Area->Generation=M.Generation;M.Slots.Add(TEXT("State_")+FMD5::HashAnsiString(*Profile)+TEXT("_")+M.Generation.ToString(EGuidFormats::Digits)+TEXT("_")+Names[I]);Cleanup.Add(M.Slots[I]);}Service->AreaIndex=0;
 };
 Prepare(50000);TestTrue(TEXT("Initial full generation saved"),Service->FlushPendingSynchronously());
 Prepare(90000);const FString Interrupted=Service->PendingManifest->Slots[0];TestTrue(TEXT("Only first autosave area reaches disk"),UGameplayStatics::SaveGameToSlot(Service->PendingAreas[0],Interrupted,0));
 FS_PlayerGameInfo Pilot;ULocalUniverseState* World=nullptr;TestTrue(TEXT("Restart ignores uncommitted partial generation"),ULocalGameStateSubsystem::ReadCheckpoint(Pilot,Profile,0,World));TestEqual(TEXT("Old complete balance retained"),Pilot.Economy.Wallets.FindRef(FEconomyLedger::Pilot()),50000LL);
 Service->bSaving=true;Service->bIOPending=true;AddExpectedError(TEXT("Area save failed"),EAutomationExpectedErrorFlags::Contains,1);Service->OnAreaSaved(Interrupted,0,false);TestFalse(TEXT("Failed area ends autosave"),Service->bSaving);
 Prepare(60000);Service->bExiting=true;TestTrue(TEXT("Exit writes a fresh complete generation during interrupted autosave"),Service->FlushPendingSynchronously());const int32 Before=Service->AreaIndex;Service->OnAreaSaved(Interrupted,0,true);TestEqual(TEXT("Late old callback cannot change exit generation"),Service->AreaIndex,Before);
 Pilot={};TestTrue(TEXT("Exit generation reloads"),ULocalGameStateSubsystem::ReadCheckpoint(Pilot,Profile,0,World));TestEqual(TEXT("Fresh exit state wins"),Pilot.Economy.Wallets.FindRef(FEconomyLedger::Pilot()),60000LL);
 UGameplayStatics::DeleteGameInSlot(Service->PendingManifest->Slots[1],0);Pilot={};TestTrue(TEXT("Missing newest area falls back to previous complete generation"),ULocalGameStateSubsystem::ReadCheckpoint(Pilot,Profile,0,World));TestEqual(TEXT("Fallback is coherent"),Pilot.Economy.Wallets.FindRef(FEconomyLedger::Pilot()),50000LL);
 return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FLostFreightRecoveryTest,"Starshatter.Traffic.LostFreightRefundAndRedispatch",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FLostFreightRecoveryTest::RunTest(const FString&)
{
 FEconomyState State;FGalaxyTrafficState Traffic;const FName Owner=FEconomyRuntime::IndustryOwner(),Item(TEXT("Ore")),Source(TEXT("Janus/Mine/Output")),Drop(TEXT("Solus/Works/Input"));
 auto* Store=FEconomyLedger::CreateInventory(State,Source,Owner,TEXT("Janus"),100);Store->TradingOwner=TEXT("Seller");Store->AskPrices.Add(Item,10);FEconomyLedger::Deposit(State,Source,Item,20,1);
 Store=FEconomyLedger::CreateInventory(State,Drop,Owner,TEXT("Solus"),100);Store->TradingOwner=TEXT("Buyer");Store->BidPrices.Add(Item,20);State.Wallets.Add(TEXT("Buyer"),1000);
 FGalaxyTrafficEndpoint A;A.Id=TEXT("Mine");A.System=TEXT("Janus");A.Sector=TEXT("Janus/Belt");auto B=A;B.Id=TEXT("Works");B.System=TEXT("Solus");B.Sector=TEXT("Solus/Orbit");
 if(!TestTrue(TEXT("Freight dispatched"),TrafficAI::Schedule(Traffic,State,A,B,{A.Sector,B.Sector},Source,Drop,Item,8,1).IsValid()))return false;
 auto& Lost=Traffic.Journeys[0];TrafficAI::Advance(Lost,State,10,false,false,10,1);TrafficAI::Advance(Lost,State,10,false,false,10,1);
 TestEqual(TEXT("Cargo physically aboard before loss"),FEconomyLedger::Quantity(State,Lost.Hold,Item),8.0);TrafficAI::Lose(Lost,State);TrafficAI::Lose(Lost,State);
 TestEqual(TEXT("Loss refunds escrow once"),State.Wallets.FindRef(TEXT("Buyer")),1000LL);TestEqual(TEXT("Undelivered supplier remains unpaid"),State.Wallets.FindRef(TEXT("Seller")),0LL);
 TestEqual(TEXT("Lost cargo is destroyed, not returned or delivered"),FEconomyLedger::Quantity(State,Source,Item),12.0);TestEqual(TEXT("Destination receives nothing on loss"),FEconomyLedger::Quantity(State,Drop,Item),0.0);TestEqual(TEXT("Lost cargo hold emptied"),FEconomyLedger::Quantity(State,Lost.Hold,Item),0.0);
 TestTrue(TEXT("New delivery uses remaining real stock"),TrafficAI::Schedule(Traffic,State,A,B,{A.Sector,B.Sector},Source,Drop,Item,8,1).IsValid());
 for(int32 I=0;I<7;++I)TrafficAI::Advance(Traffic.Journeys.Last(),State,100,false,false,10,1);
 TestEqual(TEXT("Replacement delivery arrives once"),FEconomyLedger::Quantity(State,Drop,Item),8.0);TestEqual(TEXT("Supplier paid only for successful delivery"),State.Wallets.FindRef(TEXT("Seller")),CommerceCharges::Quote(80,ECommercePurpose::Purchase).Base);TestEqual(TEXT("Destroyed cargo never respawned"),FEconomyLedger::Quantity(State,Source,Item),4.0);
 return true;
}

#endif
