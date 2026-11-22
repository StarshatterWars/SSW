#if WITH_DEV_AUTOMATION_TESTS && WITH_EDITOR
#include "Misc/AutomationTest.h"
#include "Misc/ScopeExit.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "DynamicMissionAI.h"
#include "AgentAI.h"
#include "ShipyardAI.h"
#include "ShipDesignRegistry.h"
#include "Sim.h"
#include "Ship.h"
#include "SimRegion.h"
#include "StarSystem.h"
#include "OrbitalRegion.h"
#include "Mission.h"
#include "DataLoader.h"
#include "EconomyRuntime.h"
#include "TransactionAI.h"
#include "Serialization/JsonSerializer.h"
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDynamicFactionGateTest,"Starshatter.Agents.DynamicFactionIsolation",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FDynamicFactionGateTest::RunTest(const FString&)
{
 const auto Old=ShipDesignRegistry::GetAll();ON_SCOPE_EXIT{ShipDesignRegistry::Clear();for(const auto& Pair:Old)ShipDesignRegistry::RegisterDesign(Pair.Key,Pair.Value);};
 FShipDesign D;D.ShipName=TEXT("Faction fixture");D.ShipType=2;D.Category=EShipCategory::Fighter;D.ShipEmpire=EShipEmpire::All;D.BaseCost=100;D.CargoCapacity.Tonnes=1;ShipDesignRegistry::RegisterDesign(FName(TEXT("Faction fixture")),D);
 for(auto Empire:{EEMPIRE_NAME::Marakan,EEMPIRE_NAME::Dantari})
 {
  FS_PlayerGameInfo P;P.Rank=1;P.Empire=int32(Empire);
  FText Access;auto Commercial=D;Commercial.ShipEmpire=EShipEmpire::Neutral;
  TestTrue(TEXT("Neutral commercial hulls remain available across empire boundaries"),ShipyardAI::Eligible(P,Commercial,Access));
  Commercial.ShipEmpire=EShipEmpire::Terellian;TestFalse(TEXT("Neutral access does not unlock foreign military hulls"),ShipyardAI::Eligible(P,Commercial,Access));auto& H=P.HangarShips.AddDefaulted_GetRef();H.HullId=FGuid::NewGuid();H.Design=TEXT("Faction fixture");P.SelectedHangarHull=H.HullId;
  FDynamicAgentContext A;A.Empire=Empire;A.Id=FName(*StaticEnum<EEMPIRE_NAME>()->GetNameStringByValue(int64(Empire)));A.Level=5;A.System=TEXT("Solus");A.Region=TEXT("Jalah");A.Station=TEXT("Goliath");A.Pools={FName(*(A.Id.ToString()+TEXT("Security")))};FText Why;
  if(!TestTrue(TEXT("Own empire generates a faction-only assignment"),DynamicMissionAI::Generate(P,A,100000,Why)))return false;
  auto Offers=DynamicMissionAI::Offers(P,A.Id,100001);if(!TestEqual(TEXT("Faction offer visible"),Offers.Num(),1))return false;
  const int32 Id=Offers[0];TestTrue(TEXT("Faction job pays credits and promotion points"),DynamicMissionAI::Reward(P,Id)>0 && DynamicMissionAI::Points(P,Id)>0);
  P.Empire=int32(EEMPIRE_NAME::Terellian);TestFalse(TEXT("Copied offer cannot be accepted by rival empire"),DynamicMissionAI::CanUse(P,Id,Why));TestTrue(TEXT("Rival cannot see saved faction offer"),DynamicMissionAI::Offers(P,A.Id,100001).IsEmpty());
  TestFalse(TEXT("Rival cannot generate from officer"),DynamicMissionAI::Generate(P,A,100002,Why));
  A.Empire=EEMPIRE_NAME::Neutral;P.DynamicAssignmentsJson.Empty();TestFalse(TEXT("Neutral officer cannot launder faction-only policy"),DynamicMissionAI::Generate(P,A,100002,Why));
 }
 return true;
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FIndependentObjectiveFlowTest,"Starshatter.Agents.IndependentObjectivesEndToEnd",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FIndependentObjectiveFlowTest::RunTest(const FString&)
{
 const auto Old=ShipDesignRegistry::GetAll();ON_SCOPE_EXIT{ShipDesignRegistry::Clear();for(const auto& Pair:Old)ShipDesignRegistry::RegisterDesign(Pair.Key,Pair.Value);};
 for(const auto& Pair:TArray<TPair<FName,int32>>{{TEXT("Falcon"),2},{TEXT("Zodiac"),8},{TEXT("Prospector"),8},{TEXT("Cutlass"),2},{TEXT("Courier"),16},{TEXT("Fixture base"),32768}})
 {FShipDesign D;D.ShipName=Pair.Key.ToString();D.ShipType=Pair.Value;D.Category=Pair.Value==16?EShipCategory::Transport:Pair.Value==32768?EShipCategory::Station:Pair.Value==8?EShipCategory::Shuttle:EShipCategory::Fighter;D.ShipEmpire=EShipEmpire::All;D.BaseCost=100;D.NavSys.AddDefaulted();D.NavSys.Last().DesignName=TEXT("Flight");D.CargoCapacity.Tonnes=100;D.Mass=20;D.Scale=1;D.Integrity=1000;ShipDesignRegistry::RegisterDesign(Pair.Key,D);}
 const bool OwnLoader=!DataLoader::GetLoader();if(OwnLoader)DataLoader::Initialize();ON_SCOPE_EXIT{if(OwnLoader)DataLoader::Close();};TUniquePtr<Sim> Owned;
 if(!Sim::GetSim()){Owned=MakeUnique<Sim>(nullptr);}
 auto& S=*Sim::GetSim();auto* OldMission=S.mission;auto* OldRegion=S.active_region;auto* OldSystem=S.star_system;Mission Free(Mission::FreeFlightId);StarSystem System("Objective Janus",FVector::ZeroVector);OrbitalRegion Orbit(&System,"Objective Janek",0,1,1);auto* Region=new SimRegion(&S,&Orbit);S.regions.append(Region);S.mission=&Free;S.active_region=Region;S.star_system=&System;
 ON_SCOPE_EXIT{S.regions.remove(Region);S.mission=OldMission;S.active_region=OldRegion;S.star_system=OldSystem;delete Region;};
 auto* Pickup=S.CreateShip("Pickup","",ShipDesignRegistry::FindLegacy(FName(TEXT("Fixture base"))),"Objective Janek",FVector::ZeroVector,1);auto* Drop=S.CreateShip("Drop","",ShipDesignRegistry::FindLegacy(FName(TEXT("Fixture base"))),"Objective Janek",FVector(40000,0,0),1);
 if(!TestNotNull(TEXT("Pickup base"),Pickup) || !TestNotNull(TEXT("Delivery base"),Drop))return false;
 FString Text;FFileHelper::LoadFileToString(Text,*(FPaths::ProjectContentDir()/TEXT("GameData/Agents/dynamic_jobs.json")));TSharedPtr<FJsonObject> Rules;FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(Text),Rules);
 int32 Sequence=1000;
 for(const TCHAR* Key:{TEXT("LocalPatrol"),TEXT("RouteSweep"),TEXT("RaiderIntercept"),TEXT("ConvoyEscort"),TEXT("MiningCover"),TEXT("StrikeLeader"),TEXT("DispatchCourier"),TEXT("PassengerRecovery"),TEXT("OreSupply"),TEXT("ConcentrateRun")})
 {
  const auto* V=Rules->GetArrayField(TEXT("Jobs")).FindByPredicate([&](const auto& Value){return Value->AsObject()->GetStringField(TEXT("Id"))==Key;});if(!V)return false;auto R=(*V)->AsObject();const FString Kind=R->GetStringField(TEXT("Kind"));
  const bool Freight=Kind==TEXT("Freight"),Ore=Kind==TEXT("OreDelivery"),Courier=Kind==TEXT("Courier"),Rescue=Kind==TEXT("Rescue");
  const FName Design=Courier?FName(TEXT("Courier")):Ore?FName(TEXT("Prospector")):(Freight || Rescue)?FName(TEXT("Zodiac")):FName(TEXT("Falcon"));
  auto* Craft=S.CreateShip("Objective pilot","",ShipDesignRegistry::FindLegacy(Design),"Objective Janek",FVector::ZeroVector,1);if(!Craft)return false;Region->SetPlayerShip(Craft);
  ON_SCOPE_EXIT{Craft->SetCarrier(nullptr,nullptr);Region->SetPlayerShip(nullptr);S.DestroyShip(Craft);};
  FS_PlayerGameInfo P;P.Rank=1;P.Empire=int32(EEMPIRE_NAME::Terellian);P.Economy.Wallets.Add(FEconomyLedger::Pilot(),1000000);auto& H=P.HangarShips.AddDefaulted_GetRef();H.HullId=FGuid::NewGuid();H.Design=Design;P.SelectedHangarHull=H.HullId;
  const int32 Id=950000+(++Sequence);auto J=MakeShared<FJsonObject>();
  J->SetNumberField(TEXT("id"),Id);J->SetNumberField(TEXT("empire"),P.Empire);J->SetStringField(TEXT("agent"),TEXT("Objective officer"));J->SetStringField(TEXT("rule"),Key);J->SetStringField(TEXT("kind"),Kind);J->SetStringField(TEXT("title"),Key);J->SetStringField(TEXT("system"),TEXT("Objective Janus"));J->SetStringField(TEXT("region"),TEXT("Objective Janek"));J->SetStringField(TEXT("station"),TEXT("Drop"));
  for(const auto& N:TArray<TPair<FString,double>>{{TEXT("state"),0},{TEXT("elapsed"),0},{TEXT("stage"),0},{TEXT("credits"),5000},{TEXT("score"),25},{TEXT("seed"),1234},{TEXT("expires"),double(FDateTime::UtcNow().ToUnixTimestamp()+7200)}})J->SetNumberField(N.Key,N.Value);
  auto Root=MakeShared<FJsonObject>();Root->SetNumberField(TEXT("sequence"),Sequence);Root->SetArrayField(TEXT("jobs"),{MakeShared<FJsonValueObject>(J)});FJsonSerializer::Serialize(Root,TJsonWriterFactory<>::Create(&P.DynamicAssignmentsJson));
  AgentAI Journal;const FName Pilot(TEXT("Objective pilot"));const int32 Offer[]={Id};Journal.Request(Pilot,Offer,TEXT("Objective officer"));if(!TestTrue(FString(Key)+TEXT(" accepts"),Journal.Accept(Pilot,Id)))continue;
  FText Why;Craft->SetFlightPhase(EOPSMode::ACTIVE);Craft->SetCarrier(nullptr,nullptr);
  if(Courier || Freight){TestFalse(TEXT("Cargo cannot start from an empty in-flight pickup"),DynamicMissionAI::Start(S,P,Journal,Pilot,Id,Why));Craft->SetCarrier(Pickup,nullptr);Craft->SetFlightPhase(EOPSMode::DOCKED);}
  if(!TestTrue(FString(Key)+TEXT(" starts through real journal"),DynamicMissionAI::Start(S,P,Journal,Pilot,Id,Why))){AddError(Why.ToString());continue;}
  if(Freight || Ore)
  {
   const FName Item=DataTableUtils::MakeValidName(Ore?TEXT("magnetite"):TEXT("industrial concentrate"));const FName Source(TEXT("Objective Janus/Pickup/Output")),Dest(TEXT("Objective Janus/Drop/Input"));
   auto* I=FEconomyLedger::CreateInventory(P.Economy,Source,FEconomyRuntime::IndustryOwner(),TEXT("Objective Janus"),1000);I->AskPrices.Add(Item,10);
   I=FEconomyLedger::CreateInventory(P.Economy,Dest,FEconomyRuntime::IndustryOwner(),TEXT("Objective Janus"),1000);I->DesiredStock.Add(Item,100);
   if(Freight)FEconomyLedger::Deposit(P.Economy,Source,Item,100,.5);
   else {Craft->SyncCargoInventory(P.Economy);FEconomyLedger::Deposit(P.Economy,Craft->GetCargoInventoryKey(),Item,R->GetNumberField(TEXT("CargoTonnes")),1);}
   DynamicMissionAI::Advance(S,P,Journal,Pilot,1);
   TestEqual(TEXT("Source pickup cannot credit destination"),FEconomyLedger::Quantity(P.Economy,Dest,Item),0.0);
   if(Freight){TestEqual(TEXT("Correct mass loaded for half-tonne units"),FEconomyLedger::Quantity(P.Economy,Craft->GetCargoInventoryKey(),Item),40.0);TestTrue(TEXT("Freight collateral held until delivery"),TransactionAI::Balance(P.Economy)<1000000);}
   Craft->SetCarrier(Drop,nullptr);Craft->SetFlightPhase(EOPSMode::DOCKED);DynamicMissionAI::Advance(S,P,Journal,Pilot,1);
   TestEqual(TEXT("Unload credits exactly contracted mass"),FEconomyLedger::Quantity(P.Economy,Dest,Item),Freight?40.:10.);
  }
  else if(Courier || Rescue)
  {
   DynamicMissionAI::Advance(S,P,Journal,Pilot,1);
   if(Rescue){auto* Survivor=S.FindShip(TCHAR_TO_UTF8(*FString::Printf(TEXT("Job%d-Survivors"),Id)));if(TestNotNull(TEXT("Real rescue shuttle spawns"),Survivor)){Craft->MoveTo(Survivor->GetLocation());DynamicMissionAI::Advance(S,P,Journal,Pilot,1);}}
   TestTrue(TEXT("Package occupies physical cargo hold"),Craft->HasCourierPackage(FName(*FString::Printf(TEXT("Dynamic courier %d"),Id))));
   Craft->SetCarrier(nullptr,nullptr);Craft->SetFlightPhase(EOPSMode::ACTIVE);DynamicMissionAI::Advance(S,P,Journal,Pilot,1);TestFalse(TEXT("Travel alone is not delivery"),Journal.IsCompleted(Pilot,Id));
   Craft->SetCarrier(Drop,nullptr);Craft->SetFlightPhase(EOPSMode::DOCKED);DynamicMissionAI::Advance(S,P,Journal,Pilot,1);TestFalse(TEXT("Delivered sealed package is removed"),Craft->HasCourierPackage(FName(*FString::Printf(TEXT("Dynamic courier %d"),Id))));
  }
  else
  {
   Craft->SetFlightPhase(EOPSMode::ACTIVE);DynamicMissionAI::Advance(S,P,Journal,Pilot,1);
   const FString Prefix=FString::Printf(TEXT("Job%d-"),Id);TArray<Ship*> Enemies;Ship* Ward=nullptr;
   for(ListIter<Ship> Ships=Region->GetShips();++Ships;)if(FString(UTF8_TO_TCHAR(Ships->GetName())).StartsWith(Prefix)){if(Ships->GetIFF()!=Craft->GetIFF())Enemies.Add(Ships.value());else Ward=Ships.value();}
   TestTrue(TEXT("Combat objectives create actual hostiles"),Enemies.Num()>=2);for(auto* Enemy:Enemies)S.DestroyShip(Enemy);
   if(Ward && Kind==TEXT("Escort"))Ward->MoveTo(FVector(25000,1000,7000));DynamicMissionAI::Advance(S,P,Journal,Pilot,1);
  }
  TestTrue(FString(Key)+TEXT(" completes through simulation objective handler"),Journal.IsCompleted(Pilot,Id));const int64 Paid=TransactionAI::Balance(P.Economy);TestTrue(TEXT("Completion credits paid and bond refunded"),Paid>1000000);TestEqual(TEXT("Promotion points awarded"),P.PlayerPoints,25);
  DynamicMissionAI::Advance(S,P,Journal,Pilot,100);TestEqual(TEXT("Repeated tick cannot duplicate payment"),TransactionAI::Balance(P.Economy),Paid);TestEqual(TEXT("Repeated tick cannot duplicate points"),P.PlayerPoints,25);
 }
 return true;
}
#endif
