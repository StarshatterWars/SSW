#include "DynamicMissionAI.h"
#include "AgentAI.h"
#include "ShipyardAI.h"
#include "ShipDesignRegistry.h"
#include "CombatGroupRegistry.h"
#include "TransactionAI.h"
#include "Sim.h"
#include "Ship.h"
#include "ShipDesign.h"
#include "EconomyRuntime.h"
#include "Weapon.h"
#include "WeaponGroup.h"
#include "SimRegion.h"
#include "StarSystem.h"
#include "Mission.h"
#include "Campaign.h"
#include "Instruction.h"
#include "Game.h"
#include "PlayerCharacter.h"
#include "JsonObjectConverter.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Misc/SecureHash.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"
namespace
{
using Object=TSharedPtr<FJsonObject>;
Object Read(const FS_PlayerGameInfo& P)
{
 if(P.DynamicAssignmentsJson.IsEmpty()){auto O=MakeShared<FJsonObject>();O->SetNumberField(TEXT("sequence"),0);O->SetArrayField(TEXT("jobs"),{});return O;}
 Object O;if(!FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(P.DynamicAssignmentsJson),O) || !O || !O->HasTypedField<EJson::Array>(TEXT("jobs")))return nullptr;return O;
}
void Write(FS_PlayerGameInfo& P,const Object& O){FJsonSerializer::Serialize(O.ToSharedRef(),TJsonWriterFactory<>::Create(&P.DynamicAssignmentsJson));}
TArray<Object> Rules()
{
 static const TArray<Object> All=[](){TArray<Object> Out;FString Text;Object Root;if(FFileHelper::LoadFileToString(Text,*(FPaths::ProjectContentDir()/TEXT("GameData/Agents/dynamic_jobs.json"))) && FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(Text),Root) && Root)for(const auto& V:Root->GetArrayField(TEXT("Jobs")))if(V->AsObject())Out.Add(V->AsObject());return Out;}();return All;
}
Object Find(const Object& Root,int32 Id){if(Root)for(const auto& V:Root->GetArrayField(TEXT("jobs"))){auto O=V->AsObject();if(O && O->GetIntegerField(TEXT("id"))==Id)return O;}return nullptr;}
Object Rule(const FString& Id){for(const auto& R:Rules())if(R->GetStringField(TEXT("Id"))==Id)return R;return nullptr;}
const FPlayerHangarShip* Hull(const FS_PlayerGameInfo& P,const Object& Rule,FText& Reason)
{
 if(!Rule)return nullptr;
 const FString Role=Rule->GetStringField(TEXT("HullRole"));const double Cargo=Rule->GetNumberField(TEXT("CargoTonnes"));const FPlayerHangarShip* First=nullptr;
 for(const auto& H:P.HangarShips)
 {
  if(H.bLost || H.bAssigned)continue;const auto* D=ShipDesignRegistry::Find(H.Design);FText Why;if(!D || !ShipyardAI::Eligible(P,*D,Why))continue;
  const bool Fighter=(D->ShipType&6)!=0,Shuttle=(D->ShipType&8)!=0;
  const bool Miner=H.Design.ToString().Contains(TEXT("Prospector")) || H.Design.ToString().Contains(TEXT("Rockhopper"));
  if(D->CargoCapacity.Tonnes+UE_SMALL_NUMBER<Cargo || (Role==TEXT("fighter") && !Fighter) || (Role==TEXT("shuttle") && !Shuttle) || (Role==TEXT("mining") && !Miner) || (Role==TEXT("freighter") && D->CargoCapacity.Tonnes<5))continue;
  if(H.HullId==P.SelectedHangarHull)return &H;if(!First)First=&H;
 }
 if(First)return First;
 Reason=FText::FromString(FString::Printf(TEXT("Hangar requirement: usable %s craft, at least %.2f t cargo capacity, with the required rank, empire and hull skills."),*Role,Cargo));return nullptr;
}
FGuid Receipt(int32 Id){FGuid G;FGuid::ParseExact(FMD5::HashAnsiString(*FString::Printf(TEXT("SSW Dynamic Job %d"),Id)),EGuidFormats::Digits,G);return G;}
FName Package(int32 Id){return FName(*FString::Printf(TEXT("Dynamic courier %d"),Id));}
FString DockName(Ship* P){return P && P->GetFlightPhase()<=EOPSMode::ALERT && P->GetCarrier()?FString(UTF8_TO_TCHAR(P->GetCarrier()->GetName())):FString();}
}
bool DynamicMissionAI::Generate(FS_PlayerGameInfo& P,const FDynamicAgentContext& A,int64 Now,FText& Reason)
{
 if(P.Rank<1 || A.Pools.IsEmpty()){Reason=INVTEXT("Complete training and earn your commission before taking independent work.");return false;}
 auto Root=Read(P);if(!Root){Reason=INVTEXT("Saved independent jobs could not be read. No jobs changed.");return false;}
 auto Jobs=Root->GetArrayField(TEXT("jobs"));
 for(const auto& V:Jobs){auto J=V->AsObject();if(J && J->GetStringField(TEXT("agent"))==A.Id.ToString() && J->GetIntegerField(TEXT("state"))<2 && J->GetNumberField(TEXT("expires"))>Now)return true;}
 TArray<Object> Pool;FText Missing;double Weight=0;
 for(const auto& R:Rules())
 {
  if(!A.Pools.Contains(FName(*R->GetStringField(TEXT("Pool")))) || P.Rank<R->GetIntegerField(TEXT("MinimumRank")) || A.Level<R->GetIntegerField(TEXT("MinimumAgentLevel")) || !Hull(P,R,Missing))continue;
  bool Cooldown=false;int32 Completed=0;
  for(const auto& V:Jobs){auto J=V->AsObject();if(J && J->GetStringField(TEXT("agent"))==A.Id.ToString() && J->GetStringField(TEXT("rule"))==R->GetStringField(TEXT("Id"))){if(J->GetNumberField(TEXT("created"))+R->GetNumberField(TEXT("CooldownSeconds"))>Now)Cooldown=true;if(J->GetIntegerField(TEXT("state"))==2)++Completed;}}
  if(Cooldown || (Completed>0 && !R->GetBoolField(TEXT("Repeatable"))))continue;
  Pool.Add(R);Weight+=R->GetNumberField(TEXT("Weight"));
 }
 if(Pool.IsEmpty()){Reason=Missing.IsEmpty()?INVTEXT("No independent jobs fit your hangar, rank or current agent cooldowns."):Missing;return false;}
 const int32 Sequence=Root->GetIntegerField(TEXT("sequence"))+1;if(Sequence>49999){Reason=INVTEXT("Independent job history is full.");return false;}
 FRandomStream Random(HashCombine(GetTypeHash(A.Id),uint32(Sequence)^uint32(Now/300)));double Pick=Random.FRandRange(0,Weight);Object Chosen=Pool.Last();for(const auto& R:Pool){Pick-=R->GetNumberField(TEXT("Weight"));if(Pick<=0){Chosen=R;break;}}
 int32 Prior=0;for(const auto& V:Jobs){auto J=V->AsObject();if(J && J->GetStringField(TEXT("agent"))==A.Id.ToString() && J->GetStringField(TEXT("rule"))==Chosen->GetStringField(TEXT("Id")) && J->GetIntegerField(TEXT("state"))==2)++Prior;}
 // Failed repeat rolls are stable within the same five-minute refresh window.
 if(Prior && Random.FRand()>Chosen->GetNumberField(TEXT("RepeatChance"))){Reason=INVTEXT("This agent has no new repeat assignment on this refresh.");return false;}
 auto J=MakeShared<FJsonObject>();J->SetNumberField(TEXT("id"),950000+Sequence);J->SetStringField(TEXT("agent"),A.Id.ToString());J->SetStringField(TEXT("rule"),Chosen->GetStringField(TEXT("Id")));J->SetStringField(TEXT("kind"),Chosen->GetStringField(TEXT("Kind")));J->SetStringField(TEXT("title"),A.System+TEXT(": ")+Chosen->GetStringField(TEXT("Title")));
 J->SetStringField(TEXT("system"),A.System);J->SetStringField(TEXT("region"),A.Region);J->SetStringField(TEXT("station"),A.Station);J->SetNumberField(TEXT("created"),double(Now));J->SetNumberField(TEXT("expires"),double(Now)+Chosen->GetNumberField(TEXT("OfferExpirySeconds")));J->SetNumberField(TEXT("state"),0);J->SetNumberField(TEXT("elapsed"),0);J->SetNumberField(TEXT("stage"),0);
 const int64 Credits=FMath::RoundToInt64(Chosen->GetNumberField(TEXT("BaseCredits"))*(1+Chosen->GetNumberField(TEXT("LevelCreditMultiplier"))*FMath::Max(0,A.Level-1)));
 J->SetNumberField(TEXT("credits"),double(Credits));J->SetNumberField(TEXT("score"),Chosen->GetNumberField(TEXT("CompletionScore")));J->SetNumberField(TEXT("seed"),Random.RandRange(1,MAX_int32));Jobs.Add(MakeShared<FJsonValueObject>(J));Root->SetArrayField(TEXT("jobs"),Jobs);Root->SetNumberField(TEXT("sequence"),Sequence);Write(P,Root);Reason=FText::GetEmpty();return true;
}
TArray<int32> DynamicMissionAI::Offers(const FS_PlayerGameInfo& P,FName Agent,int64 Now)
{TArray<int32> Out;auto Root=Read(P);if(Root)for(const auto& V:Root->GetArrayField(TEXT("jobs"))){auto J=V->AsObject();if(J && J->GetStringField(TEXT("agent"))==Agent.ToString() && J->GetIntegerField(TEXT("state"))<2 && J->GetNumberField(TEXT("expires"))>Now)Out.Add(J->GetIntegerField(TEXT("id")));}return Out;}
bool DynamicMissionAI::CanUse(const FS_PlayerGameInfo& P,int32 Id,FText& Reason)
{auto J=Find(Read(P),Id);if(!J || J->GetIntegerField(TEXT("state"))>=2){Reason=INVTEXT("This independent job is no longer available.");return false;}return Hull(P,Rule(J->GetStringField(TEXT("rule"))),Reason)!=nullptr;}
FText DynamicMissionAI::Title(const FS_PlayerGameInfo& P,int32 Id){auto J=Find(Read(P),Id);return J?FText::FromString(J->GetStringField(TEXT("title"))):FText::GetEmpty();}
int64 DynamicMissionAI::Reward(const FS_PlayerGameInfo& P,int32 Id){auto J=Find(Read(P),Id);return J?int64(J->GetNumberField(TEXT("credits"))):0;}
bool DynamicMissionAI::Preview(const FS_PlayerGameInfo& P,int32 Id,FS_CampaignMission& Out)
{
 auto J=Find(Read(P),Id);if(!J)return false;FString Text;
 if(!FFileHelper::LoadFileToString(Text,*(FPaths::ProjectContentDir()/TEXT("GameData/Campaigns/01/Scenes/FreeFlight.json"))) || !FJsonObjectConverter::JsonObjectStringToUStruct(Text,&Out))return false;
 Out.Id=Id;Out.MissionName=J->GetStringField(TEXT("title"));Out.StoryCampaignId=0;Out.DisplayType=EMissionDisplayType::PlayerMission;Out.Objective=FString::Printf(TEXT("Independent %s assignment in %s. Return to %s. Base payment: %lld cr. Use a qualifying owned craft; campaign progress is not required."),*J->GetStringField(TEXT("kind")),*J->GetStringField(TEXT("region")),*J->GetStringField(TEXT("station")),Reward(P,Id));FString Status;J->TryGetStringField(TEXT("status"),Status);if(!Status.IsEmpty())Out.Objective+=TEXT("\n\n")+Status;Out.Sitrep=Out.Objective;
 const FString Kind=J->GetStringField(TEXT("kind"));Out.MissionType=Kind==TEXT("Recon")?EMissionType::RECON:Kind==TEXT("Survey")?EMissionType::SCOUT:Kind==TEXT("Intercept")?EMissionType::INTERCEPT:Kind==TEXT("Escort")?EMissionType::ESCORT_FREIGHT:Kind==TEXT("Defend")?EMissionType::DEFEND:Kind==TEXT("Strike")?EMissionType::ASSAULT:Kind==TEXT("Sweep")?EMissionType::SWEEP:(Kind==TEXT("Freight") || Kind==TEXT("OreDelivery"))?EMissionType::CARGO:(Kind==TEXT("Courier") || Kind==TEXT("Rescue"))?EMissionType::TRANSPORT:EMissionType::PATROL;
 return true;
}
bool DynamicMissionAI::Start(Sim& S,FS_PlayerGameInfo& P,AgentAI& Journal,FName Pilot,int32 Id,FText& Reason)
{
 auto Root=Read(P);auto J=Find(Root,Id);auto* Craft=S.GetPlayerShip();if(!Craft || !J || !CanUse(P,Id,Reason))return false;
 const auto* Required=Hull(P,Rule(J->GetStringField(TEXT("rule"))),Reason);if(!Required || P.SelectedHangarHull!=Required->HullId || FName(UTF8_TO_TCHAR(Craft->DesignName()))!=Required->Design){Reason=INVTEXT("Dock and select the qualifying owned craft from your hangar first.");return false;}
 if(!Craft->GetRegion() || !Craft->GetRegion()->GetSystem() || J->GetStringField(TEXT("system"))!=UTF8_TO_TCHAR(Craft->GetRegion()->GetSystem()->GetName())){Reason=INVTEXT("Travel to the issuing agent's system first.");return false;}
 if(!Journal.Start(Pilot,Id))return false;
 J->SetNumberField(TEXT("state"),1);J->SetStringField(TEXT("hull"),Required->HullId.ToString(EGuidFormats::Digits));J->SetStringField(TEXT("startDock"),DockName(Craft));J->SetNumberField(TEXT("started"),double(FDateTime::UtcNow().ToUnixTimestamp()));Write(P,Root);Reason=FText::GetEmpty();return true;
}
bool DynamicMissionAI::Advance(Sim& S,FS_PlayerGameInfo& P,AgentAI& Journal,FName Pilot,double Seconds)
{
 if(!FMath::IsFinite(Seconds) || Seconds<=0 || !S.GetMission() || !S.GetMission()->IsFreeFlight() || S.IsNetworked())return false;
 auto* Craft=S.GetPlayerShip();if(!Craft || !Craft->GetRegion())return false;auto Root=Read(P);if(!Root)return false;bool Changed=false,Critical=false;
 for(const auto& Value:Root->GetArrayField(TEXT("jobs")))
 {
  auto J=Value->AsObject();if(!J || J->GetIntegerField(TEXT("state"))!=1)continue;
  const int32 Id=J->GetIntegerField(TEXT("id"));const AgentAI::FAssignment* Assignment=Journal.Find(Pilot);auto R=Rule(J->GetStringField(TEXT("rule")));if(!R)continue;
  const FString Prefix=FString::Printf(TEXT("Job%d-"),Id),Kind=J->GetStringField(TEXT("kind"));
  const bool Combat=Kind==TEXT("Patrol") || Kind==TEXT("Sweep") || Kind==TEXT("Intercept") || Kind==TEXT("Defend") || Kind==TEXT("Escort") || Kind==TEXT("Strike");
  const bool Survey=Kind==TEXT("Survey") || Kind==TEXT("Recon");const bool Sealed=Kind==TEXT("Courier") || Kind==TEXT("Rescue");
  const bool Freight=Kind==TEXT("Freight") || Kind==TEXT("OreDelivery");
  const FName Bond(*(TEXT("Dynamic cargo bond/")+FString::FromInt(Id)));
  if(Assignment && Assignment->MissionId==Id && Assignment->State==AgentAI::EState::Accepted)
  {
      FString HullText;FGuid SavedHull;if(J->TryGetStringField(TEXT("hull"),HullText) && FGuid::ParseExact(HullText,EGuidFormats::Digits,SavedHull) && SavedHull==P.SelectedHangarHull){Journal.Start(Pilot,Id);Assignment=Journal.Find(Pilot);}
  }
  const auto Release=[&]()
  {
   FString Text;FGuid Reservation;if(J->TryGetStringField(TEXT("reservation"),Text) && FGuid::Parse(Text,Reservation))FEconomyLedger::Release(P.Economy,FEconomyRuntime::IndustryOwner(),Reservation);
   Craft->RemoveCourierPackage(Package(Id));
   for(ListIter<SimRegion> Regions=S.GetRegions();++Regions;){TArray<Ship*> Cleanup;for(ListIter<Ship> Ships=Regions->GetShips();++Ships;)if(FString(UTF8_TO_TCHAR(Ships->GetName())).StartsWith(Prefix) && !Ships->HasNetworkPilot() && Ships.value()!=Craft)Cleanup.Add(Ships.value());for(auto* Ship:Cleanup)S.DestroyShip(Ship);}
  };
  if(!Assignment || Assignment->MissionId!=Id || Assignment->State!=AgentAI::EState::InProgress || Craft->IsDead() || Craft->IsDying())
  {Release();if(Assignment && Assignment->MissionId==Id)Journal.Resolve(Pilot,Id,false);J->SetNumberField(TEXT("state"),3);Changed=true;Critical=true;continue;}
  const int32 OldStage=J->GetIntegerField(TEXT("stage")),OldState=J->GetIntegerField(TEXT("state"));
  const double Elapsed=J->GetNumberField(TEXT("elapsed"))+Seconds;J->SetNumberField(TEXT("elapsed"),Elapsed);Changed=true;
  if(Elapsed>R->GetNumberField(TEXT("TimeLimitSeconds"))){Release();Journal.Resolve(Pilot,Id,false);J->SetNumberField(TEXT("state"),3);Critical=true;continue;}
  int32 Stage=J->GetIntegerField(TEXT("stage"));auto* Region=Craft->GetRegion();const FVector Here=Craft->GetLocation();bool Complete=false;TOptional<FEconomyState> Delivery;
  if(!J->HasField(TEXT("x")))
  {
   J->SetNumberField(TEXT("x"),Here.X);J->SetNumberField(TEXT("y"),Here.Y);J->SetNumberField(TEXT("z"),Here.Z);
   FString Destination=J->GetStringField(TEXT("station"));
   if(Destination==J->GetStringField(TEXT("startDock")) || !S.FindShip(TCHAR_TO_UTF8(*Destination)))
   {for(ListIter<Ship> Ships=Region->GetShips();++Ships;)if(Ships->IsStatic() && Ships->NumFlightDecks()>0 && Ships->GetIFF()==Craft->GetIFF() && FString(UTF8_TO_TCHAR(Ships->GetName()))!=J->GetStringField(TEXT("startDock"))){Destination=UTF8_TO_TCHAR(Ships->GetName());break;}}
   J->SetStringField(TEXT("destination"),Destination);
  }
  const FVector Origin(J->GetNumberField(TEXT("x")),J->GetNumberField(TEXT("y")),J->GetNumberField(TEXT("z")));
  if(Combat)
  {
   if(Stage==0 && Craft->GetFlightPhase()>EOPSMode::ALERT)
   {
    auto* Enemy=ShipDesignRegistry::FindLegacy(FName(TEXT("Cutlass")));if(!Enemy)continue;
    FRandomStream Random(J->GetIntegerField(TEXT("seed")));const int32 Count=2+Random.RandRange(0,2);int32 Spawned=0;
    for(int32 I=0;I<Count;++I){const FString Name=Prefix+FString::FromInt(I);auto* NPC=S.CreateShip(TCHAR_TO_UTF8(*Name),"",Enemy,Region->GetName(),OtherHand(Origin+FVector(18000+I*1500,2000+I*750,10000)),Craft->GetIFF()==3?1:3);if(!NPC)continue;auto* Attack=new Instruction(EInstruction::Intercept,Craft->GetName());Attack->SetWeaponsFree(1);NPC->AddNavPoint(Attack);++Spawned;}
    if(!Spawned)continue;J->SetNumberField(TEXT("enemyCount"),Spawned);
    if(Kind==TEXT("Escort") || Kind==TEXT("Defend"))
    {
     auto* Design=ShipDesignRegistry::FindLegacy(FName(TEXT("Courier")));const FString Name=Prefix+TEXT("Ward");
     auto* Ward=Design?S.CreateShip(TCHAR_TO_UTF8(*Name),"",Design,Region->GetName(),OtherHand(Origin+FVector(8000,0,2000)),Craft->GetIFF()):nullptr;
     if(!Ward)continue;auto* Route=new Instruction(Region,OtherHand(Origin+FVector(25000,1000,7000)));Route->SetSpeed(150);Route->SetWeaponsFree(0);Ward->AddNavPoint(Route);Ward->SetAutoNav(true);
    }
    J->SetNumberField(TEXT("stage"),1);J->SetStringField(TEXT("status"),TEXT("Defeat the raiders and protect any assigned transport."));
   }
   else if(Stage==1)
   {
    int32 Alive=0;for(int32 I=0;I<J->GetIntegerField(TEXT("enemyCount"));++I)if(auto* NPC=S.FindShip(TCHAR_TO_UTF8(*(Prefix+FString::FromInt(I))));NPC && !NPC->IsDead() && !NPC->IsDying())++Alive;
    auto* Ward=S.FindShip(TCHAR_TO_UTF8(*(Prefix+TEXT("Ward"))));
    if((Kind==TEXT("Escort") || Kind==TEXT("Defend")) && (!Ward || Ward->IsDead() || Ward->IsDying())){Release();Journal.Resolve(Pilot,Id,false);J->SetNumberField(TEXT("state"),3);Critical=true;continue;}
    Complete=Alive==0;if(Complete && Kind==TEXT("Escort"))Complete=FVector::DistSquared(Ward->GetLocation(),Origin+FVector(25000,1000,7000))<FMath::Square(2500.0);
   }
  }
  else if(Survey)
  {
   const int32 Count=Kind==TEXT("Recon")?4:3;const double Angle=Stage*2*PI/Count;const FVector Point=Origin+FVector(FMath::Cos(Angle)*18000,2000+Stage*750,FMath::Sin(Angle)*18000);
   int32 Shown=-1;J->TryGetNumberField(TEXT("navStage"),Shown);if(Shown!=Stage){Craft->SetAutoNav(false);Craft->ClearFlightPlan();auto* N=new Instruction(Region,OtherHand(Point));N->SetSpeed(250);N->SetWeaponsFree(0);Craft->AddNavPoint(N);J->SetNumberField(TEXT("navStage"),Stage);}
   J->SetStringField(TEXT("status"),FString::Printf(TEXT("Survey point %d/%d: range %.2f km; bearing %.1f degrees."),Stage+1,Count,FVector::Distance(Here,Point)/1000,(Point-Here).Rotation().Yaw));
   if(FVector::DistSquared(Here,Point)<FMath::Square(1800.0)){++Stage;J->SetNumberField(TEXT("stage"),Stage);Complete=Stage>=Count;}
  }
  else if(Sealed)
  {
   const FString Dock=DockName(Craft);const double Tonnes=R->GetNumberField(TEXT("CargoTonnes"));
   if(Stage==0)
   {
    if(Kind==TEXT("Rescue"))
    {
     const FString Name=Prefix+TEXT("Survivors");auto* Shuttle=S.FindShip(TCHAR_TO_UTF8(*Name));
     if(!Shuttle){auto* Design=ShipDesignRegistry::FindLegacy(FName(TEXT("Zodiac")));if(Design)Shuttle=S.CreateShip(TCHAR_TO_UTF8(*Name),"",Design,Region->GetName(),OtherHand(Origin+FVector(14000,0,8000)),0);if(Shuttle)Shuttle->StopScriptedMotion();}
     if(Shuttle)J->SetStringField(TEXT("status"),FString::Printf(TEXT("Recover survivors from %s within 750 m. Current range %.2f km."),*Name,FVector::Distance(Here,Shuttle->GetLocation())/1000));
     if(Shuttle && FVector::DistSquared(Here,Shuttle->GetLocation())<FMath::Square(750.0) && Craft->LoadCourierPackage(Package(Id),Tonnes)){J->SetNumberField(TEXT("stage"),1);S.DestroyShip(Shuttle);}
    }
    else if(!Dock.IsEmpty() && Dock==J->GetStringField(TEXT("startDock")) && Craft->LoadCourierPackage(Package(Id),Tonnes))J->SetNumberField(TEXT("stage"),1);
    else J->SetStringField(TEXT("status"),TEXT("Dock at the pickup carrier to load the sealed dispatch."));
   }
   else
   {
    J->SetStringField(TEXT("status"),TEXT("Deliver the sealed package to ")+J->GetStringField(TEXT("destination")));
    if(!Craft->HasCourierPackage(Package(Id))){Journal.Resolve(Pilot,Id,false);J->SetNumberField(TEXT("state"),3);Critical=true;continue;}
    if(Dock.IsEmpty())J->SetBoolField(TEXT("departed"),true);bool Departed=false;J->TryGetBoolField(TEXT("departed"),Departed);
    Complete=Departed && Dock==J->GetStringField(TEXT("destination"));
   }
  }
  else if(Freight)
  {
   const double Tonnes=R->GetNumberField(TEXT("CargoTonnes"));const FName Item=DataTableUtils::MakeValidName(Kind==TEXT("OreDelivery")?TEXT("magnetite"):J->GetStringField(TEXT("rule"))==TEXT("ShipyardSupply")?TEXT("structural alloy"):J->GetStringField(TEXT("rule"))==TEXT("MachineParts")?TEXT("machine parts"):TEXT("industrial concentrate"));
   const FName Hold=Craft->GetCargoInventoryKey();Craft->SyncCargoInventory();const FString Dock=DockName(Craft);FName Destination;
   for(const auto& Pair:P.Economy.Inventories)if(Pair.Value.Owner==FEconomyRuntime::IndustryOwner() && Pair.Value.bOperational && Pair.Key.ToString().StartsWith(J->GetStringField(TEXT("system"))+TEXT("/")) && Pair.Key.ToString().Contains(TEXT("/")+Dock+TEXT("/")) && Pair.Value.DesiredStock.Contains(Item)){Destination=Pair.Key;break;}
   double UnitMass=1;J->TryGetNumberField(TEXT("unitMass"),UnitMass);if(!FMath::IsFinite(UnitMass) || UnitMass<=0)continue;
   double Quantity=Tonnes/UnitMass;
   if(Kind==TEXT("Freight") && Stage==0 && !Dock.IsEmpty())
   {
    TArray<FName> Keys;P.Economy.Inventories.GetKeys(Keys);Keys.Sort([](FName A,FName B){return A.LexicalLess(B);});
    for(FName Key:Keys)if(Key.ToString().Contains(TEXT("/")+Dock+TEXT("/")) && P.Economy.Inventories[Key].Owner==FEconomyRuntime::IndustryOwner() && FEconomyLedger::Available(P.Economy,Key,Item)>=Quantity)
    {
     auto Candidate=P.Economy;const auto* Stock=Candidate.Inventories[Key].Items.FindByPredicate([&](const auto& Stack){return Stack.Type==Item;});if(!Stock || Stock->UnitTonnes<=0)continue;UnitMass=Stock->UnitTonnes;Quantity=Tonnes/UnitMass;
     const int64 Collateral=FMath::CeilToInt64(Quantity*FMath::Max(1LL,Candidate.Inventories[Key].AskPrices.FindRef(Item)));
     if(!FEconomyLedger::Pay(Candidate,FEconomyLedger::Pilot(),Bond,Collateral,FGuid::NewGuid(),TEXT("Independent freight cargo bond")) || !FEconomyLedger::Withdraw(Candidate,FEconomyRuntime::IndustryOwner(),Key,Item,Quantity) || !FEconomyLedger::Deposit(Candidate,Hold,Item,Quantity,UnitMass))continue;
     P.Economy=MoveTemp(Candidate);J->SetNumberField(TEXT("unitMass"),UnitMass);J->SetNumberField(TEXT("bond"),double(Collateral));J->SetStringField(TEXT("pickupInventory"),Key.ToString());J->SetNumberField(TEXT("stage"),1);Craft->ApplyCargoInventoryToManifest();break;
    }
   }
   const double Aboard=FEconomyLedger::Available(P.Economy,Hold,Item);
   J->SetStringField(TEXT("status"),FString::Printf(TEXT("Deliver %.2f t %s to a docked facility that needs it. Aboard %.2f t. Freight pickup requires source stock and a refundable cargo bond."),Tonnes,*Item.ToString(),Aboard*UnitMass));
   FString Pickup;J->TryGetStringField(TEXT("pickupInventory"),Pickup);
   if(!Dock.IsEmpty() && !Destination.IsNone() && Destination.ToString()!=Pickup && Aboard>=Quantity && (Kind==TEXT("OreDelivery") || Stage==1))
   {
    auto Candidate=P.Economy;if(FEconomyLedger::Withdraw(Candidate,FEconomyLedger::Pilot(),Hold,Item,Quantity) && FEconomyLedger::Deposit(Candidate,Destination,Item,Quantity,UnitMass)){Delivery=MoveTemp(Candidate);Complete=true;}
   }
  }
  if(Complete)
  {
   const int64 Base=int64(J->GetNumberField(TEXT("credits"))),Bonus=Elapsed<=R->GetNumberField(TEXT("TimeBonusSeconds"))?int64(R->GetNumberField(TEXT("TimeBonusCredits"))):0;
   const int64 Return=DockName(Craft).IsEmpty()?0:int64(R->GetNumberField(TEXT("RecoveryBonusCredits")));auto Candidate=Delivery?*Delivery:P.Economy;double BondValue=0;J->TryGetNumberField(TEXT("bond"),BondValue);
   if(BondValue>0 && !FEconomyLedger::Pay(Candidate,Bond,FEconomyLedger::Pilot(),int64(BondValue),FGuid::NewGuid(),TEXT("Independent freight cargo bond refund")))continue;
   const int64 Balance=TransactionAI::Balance(Candidate);if(Balance>MAX_int64-Base-Bonus-Return)continue;
   if(!TransactionAI::ReconcileExternalBalance(Candidate,FEconomyLedger::Pilot(),Balance+Base+Bonus+Return,FName(*J->GetStringField(TEXT("title"))),TEXT("Independent agent assignment"),EWalletTransactionCategory::MissionReward,Receipt(Id),Receipt(Id)))continue;
   if(!Journal.Resolve(Pilot,Id,true))continue;P.Economy=MoveTemp(Candidate);Craft->ApplyCargoInventoryToManifest();J->SetNumberField(TEXT("state"),2);P.MarkMissionCompleted(Campaign::SINGLE_MISSIONS,Id);
   if(auto* Character=PlayerCharacter::GetCurrentPlayer();Character && AgentAI::PilotKey(*Character)==Pilot){Character->AddPoints(J->GetIntegerField(TEXT("score")));P.PlayerPoints=Character->GetPoints();P.Rank=Character->GetRank();}else P.PlayerPoints=int32(FMath::Clamp(int64(P.PlayerPoints)+J->GetIntegerField(TEXT("score")),0LL,int64(MAX_int32)));
   Release();J->SetStringField(TEXT("status"),TEXT("Completed. Credits and score recorded."));Critical=true;
  }
  Critical|=OldStage!=J->GetIntegerField(TEXT("stage")) || OldState!=J->GetIntegerField(TEXT("state"));
 }
 if(Changed)Write(P,Root);return Critical;
}
