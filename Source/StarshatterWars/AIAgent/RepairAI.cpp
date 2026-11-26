#include "RepairAI.h"
#include "CampaignFleet.h"
#include "EconomyRuntime.h"
#include "SimDataEvents.h"
#include "Misc/SecureHash.h"
#include "Engine/DataTable.h"
#include "Serialization/JsonSerializer.h"

namespace
{
FName Parts(){return DataTableUtils::MakeValidName(TEXT("military spare parts"));}
FName Alloy(){return DataTableUtils::MakeValidName(TEXT("structural alloy"));}
FName Kits(){return DataTableUtils::MakeValidName(TEXT("repair kit"));}
void Restore(FPlayerHangarShip& Hull,double Progress,const FPlayerHangarShip& Baseline)
{
 Hull.bHasCondition=true;
 Hull.HullCondition=FMath::Lerp(Baseline.HullCondition,1.f,float(Progress));
 Hull.SystemConditions=Baseline.SystemConditions;
 for(auto& System:Hull.SystemConditions)
 {
  System.Availability=FMath::Lerp(System.Availability,1.f,float(Progress));
  System.Safety=FMath::Lerp(System.Safety,1.f,float(Progress));
  System.Stability=FMath::Lerp(System.Stability,1.f,float(Progress));
  System.SafetyOverload=0;System.NetAvailability=-1;
  System.Status=System.Availability>=.999f?3:System.Availability>=.5f?2:System.Availability>0?1:0;
 }
 Hull.Components=Baseline.Components;
 for(auto& Part:Hull.Components)
 {
  Part.bHasCondition=true;Part.ConditionPercent=FMath::Lerp(Part.ConditionPercent,100.f,float(Progress));
  Part.ConditionStatus=Part.ConditionPercent>=99.9f?3:Part.ConditionPercent>=50?2:Part.ConditionPercent>0?1:0;
  Part.MaintenanceSeconds=0;
 }
 Hull.RepairQueue.Reset();
 // Ordnance, skins and loadout remain the current hull's state, never a repair grant.
}
double Damage(const FPlayerHangarShip& Hull)
{
 if(!Hull.bHasCondition)return 0;
 double Missing=1.-FMath::Clamp(double(Hull.HullCondition),0.,1.);
 for(const auto& System:Hull.SystemConditions)Missing=FMath::Max(Missing,1.-FMath::Clamp(double(FMath::Min3(System.Availability,System.Safety,System.Stability)),0.,1.));
 for(const auto& Part:Hull.Components)if(Part.bHasCondition)Missing=FMath::Max(Missing,1.-FMath::Clamp(double(Part.ConditionPercent)/100.,0.,1.));
 return Missing;
}
bool NewDamage(const FPlayerHangarShip& Current,const FFleetRepairOrder& Job,double Applied)
{
 FPlayerHangarShip Expected=Job.Baseline;Restore(Expected,Applied,Job.Baseline);
 if(Current.HullCondition+.001f<Expected.HullCondition)return true;
 for(const auto& Old:Expected.SystemConditions)
  if(const auto* Now=Current.SystemConditions.FindByPredicate([&](const auto& S){return S.SystemIndex==Old.SystemIndex && S.SystemName==Old.SystemName;});Now && (Now->Availability+.001f<Old.Availability || Now->Safety+.001f<Old.Safety || Now->Stability+.001f<Old.Stability))return true;
 for(const auto& Old:Expected.Components)
  if(const auto* Now=Current.Components.FindByPredicate([&](const auto& S){return S.SystemIndex==Old.SystemIndex && S.ComponentIndex==Old.ComponentIndex;});Now && Now->ConditionPercent+.1f<Old.ConditionPercent)return true;
 return false;
}
void ApplyProgress(FCampaignFleetState& Fleet,FFleetRepairOrder& Job)
{
 const double Progress=Job.Progress();
 if(Progress<=Fleet.AppliedRepairProgress.FindRef(Job.Id))return;
 auto* Hull=Fleet.Hulls.Find(Job.HullId);if(!Hull || Hull->bTransferred)return;
 if(Job.bReplacement)
 {
  if(Progress<1 || !Hull->bDestroyed || Hull->bHero)return;
  auto* Unit=Fleet.Units.Find(Hull->UnitKey);
  if(!Unit || !Unit->HullIds.IsValidIndex(Hull->Slot) || Unit->HullIds[Hull->Slot]!=Hull->HullId)return;
  if(!Fleet.Hulls.Contains(Job.ReplacementId))
  {
   auto NewHull=*Hull;NewHull.HullId=Job.ReplacementId;NewHull.Replaces=Hull->HullId;
   NewHull.bDestroyed=false;NewHull.bRepairing=false;
   NewHull.Condition=Job.Baseline;NewHull.Condition.bLost=false;NewHull.Condition.StandardEquipmentLevel=3;
   Restore(NewHull.Condition,1,Job.Baseline);
   NewHull.Condition.HullId=NewHull.HullId;
   NewHull.Condition.SavedWeaponAmmo.Reset();NewHull.Condition.SavedWeaponTypes.Reset();
   for(auto& Part:NewHull.Condition.Components){Part.Level=3;Part.InstalledValue=-1;Part.ItemId=FGuid::NewGuid();Part.SpareCount=0;Part.JerryCount=0;}
   Unit->HullIds[NewHull.Slot]=NewHull.HullId;
   // Keep the previous applied loss until Prepare reconciles the native unit.
   Fleet.Hulls.Add(NewHull.HullId,MoveTemp(NewHull));
  }
 }
 else
 {
  if(Hull->bDestroyed)return;
  Restore(Hull->Condition,Progress,Job.Baseline);
 }
 Fleet.AppliedRepairProgress.Add(Job.Id,Progress);Fleet.bDirty=true;
}
FName FindYard(const FEconomyState& Economy,const FFleetRepairContext& Context,FName System)
{
 TArray<FName> Keys=Context.AuthorizedShipyards.Array();Keys.Sort(FNameLexicalLess());
 for(FName Key:Keys)if(const auto* Store=Economy.Inventories.Find(Key);Store && Store->bOperational && Store->bHullStorage && Store->bSupplyOnly && Store->System==System)return Key;
 return NAME_None;
}
}
bool FFleetRepairPolicy::LoadJson(const FString& Json,FText& Error)
{
 TSharedPtr<FJsonObject> Root;
 if(!FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(Json),Root) || !Root){Error=INVTEXT("Invalid fleet repair JSON.");return false;}
 FFleetRepairPolicy Candidate;double Version=0,TeamCount=0;
 const auto Read=[&](const TCHAR* Key,double& Value,double Maximum)
 {return Root->TryGetNumberField(Key,Value) && FMath::IsFinite(Value) && Value>=0 && Value<=Maximum;};
 if(!Root->TryGetNumberField(TEXT("schemaVersion"),Version) || Version!=1 || !Read(TEXT("teams"),TeamCount,16) || TeamCount<1 || TeamCount!=FMath::FloorToDouble(TeamCount) ||
 !Read(TEXT("repairSecondsPerMassRoot"),Candidate.RepairSecondsPerMassRoot,86400) || Candidate.RepairSecondsPerMassRoot<1 ||
 !Read(TEXT("rebuildSecondsPerMassRoot"),Candidate.RebuildSecondsPerMassRoot,604800) || Candidate.RebuildSecondsPerMassRoot<1 ||
 !Read(TEXT("partsPerMassRoot"),Candidate.PartsPerMassRoot,10000) || Candidate.PartsPerMassRoot<=0 ||
 !Read(TEXT("alloyPerMassRoot"),Candidate.AlloyPerMassRoot,10000) || Candidate.AlloyPerMassRoot<=0 ||
 !Read(TEXT("initialParts"),Candidate.InitialParts,100000) || !Read(TEXT("initialAlloy"),Candidate.InitialAlloy,100000) || !Read(TEXT("initialKits"),Candidate.InitialKits,100000))
 {Error=INVTEXT("Fleet repair policy has missing or invalid values.");return false;}
 Candidate.Teams=int32(TeamCount);*this=Candidate;return true;
}
bool RepairAI::NeedsRepair(const FPlayerHangarShip& Hull){return Damage(Hull)>.001;}
FName RepairAI::Depot(const FCampaignFleetState& Fleet,int32 Faction,FName System)
{return FName(*(TEXT("Fleet Repair/")+Fleet.FleetId.ToString(EGuidFormats::Digits)+TEXT("/")+FString::FromInt(Faction)+TEXT("/")+System.ToString()));}
FFleetRepairStep RepairAI::Advance(FCampaignFleetState& Fleet,FEconomyState& Economy,const FFleetRepairContext& Context,const FFleetRepairPolicy& Policy)
{
 FFleetRepairStep Result;
 if(!Fleet.bInitialized || Fleet.bHeroLost || !FMath::IsFinite(Context.Now) || Context.Now<0 || Context.SupplySystem.IsNone() || Context.AlliedFaction<=0)return Result;
 if(!Fleet.FleetId.IsValid())
 {
  // Deterministic migration: if the economy save succeeds before the first
  // campaign save, reloading that older campaign must locate the same orders.
  TArray<FGuid> Existing;Fleet.Hulls.GetKeys(Existing);if(Existing.IsEmpty())return Result;
  Existing.Sort([](const FGuid& A,const FGuid& B){return A.ToString()<B.ToString();});
  const FString Hash=FMD5::HashAnsiString(*(TEXT("FleetRepair/")+FString::FromInt(Fleet.CampaignId)+TEXT("/")+Existing[0].ToString()));
  FGuid::ParseExact(Hash,EGuidFormats::Digits,Fleet.FleetId);Result.bChanged=true;
 }
 const FName Grant(*(TEXT("Fleet Repair Grant/")+Fleet.FleetId.ToString(EGuidFormats::Digits)+TEXT("/")+FString::FromInt(Context.AlliedFaction)));
 const bool AlreadyGranted=Economy.InitializedMarkets.Contains(Grant);
 TSet<FName> Systems{Context.SupplySystem};
 for(const auto& Pair:Fleet.Hulls)if(Pair.Value.Faction==Context.AlliedFaction && !Pair.Value.bTransferred && !Pair.Value.System.IsEmpty())Systems.Add(FName(*Pair.Value.System));
 for(FName System:Systems)
 {
  const FName Supply=Depot(Fleet,Context.AlliedFaction,System);
  if(Economy.Inventories.Contains(Supply))continue;
  auto* Store=FEconomyLedger::CreateInventory(Economy,Supply,FEconomyRuntime::IndustryOwner(),System,1000000);
  if(!Store)continue;
  // A single authored campaign loadout. Other depots must be supplied by real
  // production/freight. Visiting another system never creates another grant.
  if(!AlreadyGranted && System==Context.SupplySystem)
  {
   FEconomyLedger::Deposit(Economy,Supply,Parts(),Policy.InitialParts,1);
   FEconomyLedger::Deposit(Economy,Supply,Alloy(),Policy.InitialAlloy,1);
   FEconomyLedger::Deposit(Economy,Supply,Kits(),Policy.InitialKits,1);
   Economy.InitializedMarkets.Add(Grant);
  }
  Store=Economy.Inventories.Find(Supply);Store->DesiredStock.Add(Parts(),Policy.InitialParts);Store->DesiredStock.Add(Alloy(),Policy.InitialAlloy);Store->DesiredStock.Add(Kits(),Policy.InitialKits);
  Result.bChanged=true;
 }
 TSet<FGuid> Active;TArray<FGuid> Orders;
 Economy.FleetRepairOrders.GetKeys(Orders);Orders.Sort([](const FGuid& A,const FGuid& B){return A.ToString()<B.ToString();});
 int32 Running=0;
 for(FGuid Id:Orders)
 {
  auto& Job=Economy.FleetRepairOrders.FindChecked(Id);if(Job.FleetId!=Fleet.FleetId || Job.State==EFleetRepairState::Cancelled)continue;
  const double Before=Fleet.AppliedRepairProgress.FindRef(Job.Id);ApplyProgress(Fleet,Job);
  if(Fleet.AppliedRepairProgress.FindRef(Job.Id)!=Before)Result.bChanged=true;
  if(Job.IsFinished())continue;
  auto* Hull=Fleet.Hulls.Find(Job.HullId);
  if(!Hull || Hull->bTransferred || (!Job.bReplacement && Hull->bDestroyed) || (Job.bReplacement && (!Hull->bDestroyed || Hull->bHero)))
  {Job.State=EFleetRepairState::Cancelled;Job.Reason=TEXT("HULL NO LONGER ELIGIBLE");Result.bChanged=true;continue;}
  if(!Job.bReplacement && NewDamage(Hull->Condition,Job,Fleet.AppliedRepairProgress.FindRef(Job.Id)))
  {Job.State=EFleetRepairState::Cancelled;Job.Reason=TEXT("NEW DAMAGE REQUIRES REASSESSMENT");Hull->bRepairing=false;Result.bChanged=true;continue;}
  Active.Add(Hull->HullId);
  const auto* ActiveDepot=Economy.Inventories.Find(Job.Depot);
  const bool DepotOffline=!ActiveDepot || !ActiveDepot->bOperational || ActiveDepot->Owner!=FEconomyRuntime::IndustryOwner();
  const bool Blocked=Context.Deployed.Contains(Hull->HullId) || FName(*Hull->System)!=Job.System || DepotOffline ||
      (Job.bReplacement && FindYard(Economy,Context,Job.System).IsNone());
  if(Blocked)
  {
   const FString Why=DepotOffline?TEXT("FLEET SUPPLY DEPOT OFFLINE"):Context.Deployed.Contains(Hull->HullId)?TEXT("DEPLOYED / REPAIRS SUSPENDED"):FName(*Hull->System)!=Job.System?TEXT("RETURN TO THE REPAIR DEPOT SYSTEM"):TEXT("RETURN TO AN OPERATIONAL ALLIED SHIPYARD");
   if(Job.State!=EFleetRepairState::Paused || Job.Reason!=Why){Job.State=EFleetRepairState::Paused;Job.Reason=Why;Job.LastAt=Context.Now;Result.bChanged=true;}
   Hull->bRepairing=false;continue;
  }
  if(Job.State==EFleetRepairState::Running)
  {
   const double Elapsed=FMath::Max(0.,Context.Now-Job.LastAt);
   if(Elapsed>0){Job.WorkedSeconds=FMath::Min(Job.DurationSeconds,Job.WorkedSeconds+Elapsed);Job.LastAt=Context.Now;Result.bChanged=true;ApplyProgress(Fleet,Job);}
   if(Job.Progress()>=1){Job.State=EFleetRepairState::Completed;Job.Reason=TEXT("COMPLETED");Hull=Fleet.Hulls.Find(Job.HullId);if(Hull)Hull->bRepairing=false;continue;}
   ++Running;
  }
 }
 // Stable priority prevents hash-map ordering from deciding who receives the last spare.
 TArray<FGuid> Hulls;
 for(const auto& Unit:Fleet.Units)for(FGuid Id:Unit.Value.HullIds)if(const auto* Hull=Fleet.Hulls.Find(Id);Hull && Hull->Faction==Context.AlliedFaction && !Hull->bTransferred && !Hull->bStatic)Hulls.Add(Id);
 Hulls.Sort([&](FGuid A,FGuid B)
 {
  const auto& First=Fleet.Hulls.FindChecked(A);const auto& Second=Fleet.Hulls.FindChecked(B);
  const int32 AP=First.bHero?0:First.bStoryRequired?1:Context.Designs.FindRef(First.Design).bFighter?3:2;
  const int32 BP=Second.bHero?0:Second.bStoryRequired?1:Context.Designs.FindRef(Second.Design).bFighter?3:2;
  return AP==BP?A.ToString()<B.ToString():AP<BP;
 });
 for(FGuid Id:Hulls)
 {
  auto& Hull=Fleet.Hulls.FindChecked(Id);const auto* Spec=Context.Designs.Find(Hull.Design);if(!Spec || !FMath::IsFinite(Spec->Mass) || Spec->Mass<=0 || Active.Contains(Id))continue;
  if(Hull.bDestroyed && (Hull.bHero || (!Spec->bFighter && !Hull.bStoryRequired)))continue;
  if(!Hull.bDestroyed && !NeedsRepair(Hull.Condition))continue;
  FFleetRepairOrder Job;Job.Id=FGuid::NewGuid();Job.FleetId=Fleet.FleetId;Job.HullId=Id;Job.bReplacement=Hull.bDestroyed;
  Job.ReplacementId=Job.bReplacement?FGuid::NewGuid():FGuid();Job.System=FName(*Hull.System);Job.Depot=Depot(Fleet,Hull.Faction,Job.System);Job.CreatedAt=Job.LastAt=Context.Now;Job.Baseline=Hull.Condition;Job.Reason=TEXT("WAITING FOR A FLEET REPAIR TEAM");
  if(!Job.Baseline.bHasCondition){Job.Baseline.bHasCondition=true;Job.Baseline.HullCondition=Hull.bDestroyed?0:1;}
  const double Scale=FMath::Pow(FMath::Max(1.,Spec->Mass),1./3.);
  const double Fraction=Job.bReplacement?1.:FMath::Max(.01,Damage(Job.Baseline));
  Job.DurationSeconds=FMath::Clamp(Scale*Fraction*(Job.bReplacement?Policy.RebuildSecondsPerMassRoot:Policy.RepairSecondsPerMassRoot),Job.bReplacement?600.:60.,604800.);
  Job.Materials.Add(Parts(),FMath::CeilToDouble(Job.bReplacement?FMath::Max(Scale*Policy.PartsPerMassRoot,Spec->Mass*.1):Scale*Fraction*Policy.PartsPerMassRoot));
  Job.Materials.Add(Alloy(),FMath::CeilToDouble(Job.bReplacement?FMath::Max(Scale*Policy.AlloyPerMassRoot,Spec->Mass*.75):Scale*Fraction*Policy.AlloyPerMassRoot));Job.Materials.Add(Kits(),1);
  Orders.Add(Job.Id);Economy.FleetRepairOrders.Add(Job.Id,MoveTemp(Job));Active.Add(Id);Result.bChanged=true;
 }
 for(FGuid Id:Orders)
 {
  auto& Job=Economy.FleetRepairOrders.FindChecked(Id);if(Job.FleetId!=Fleet.FleetId || Job.IsFinished())continue;
  auto* Hull=Fleet.Hulls.Find(Job.HullId);if(!Hull)continue;
  if(Job.State!=EFleetRepairState::Running)
  {
   if(Context.Deployed.Contains(Hull->HullId) || FName(*Hull->System)!=Job.System)continue;
   if(Job.bReplacement)
   {
    const FName Yard=FindYard(Economy,Context,Job.System);
    if(Yard.IsNone())
    {if(Job.State!=EFleetRepairState::Shipyard){Job.State=EFleetRepairState::Shipyard;Job.Reason=TEXT("AWAITING AN OPERATIONAL ALLIED SHIPYARD");Result.bChanged=true;}continue;}
    Job.Shipyard=Yard;
   }
   if(Running>=Policy.Teams){if(Job.State!=EFleetRepairState::Queued){Job.State=EFleetRepairState::Queued;Job.Reason=TEXT("WAITING FOR A FLEET REPAIR TEAM");Result.bChanged=true;}continue;}
   const auto* DepotStore=Economy.Inventories.Find(Job.Depot);
   if(!DepotStore || !DepotStore->bOperational || DepotStore->Owner!=FEconomyRuntime::IndustryOwner())
   {if(Job.State!=EFleetRepairState::Supplies || Job.Reason!=TEXT("FLEET SUPPLY DEPOT OFFLINE")){Job.State=EFleetRepairState::Supplies;Job.Reason=TEXT("FLEET SUPPLY DEPOT OFFLINE");Result.bChanged=true;}continue;}
   if(!Job.bResourcesConsumed)
   {
    FString Missing;
    for(const auto& Material:Job.Materials)if(FEconomyLedger::Available(Economy,Job.Depot,Material.Key)+UE_SMALL_NUMBER<Material.Value)
     Missing+=FString::Printf(TEXT("%s%s %.0f"),Missing.IsEmpty()?TEXT(""):TEXT(", "),*Material.Key.ToString(),Material.Value-FEconomyLedger::Available(Economy,Job.Depot,Material.Key));
    if(!Missing.IsEmpty())
    {const FString Why=TEXT("AWAITING SUPPLIES: ")+Missing;if(Job.State!=EFleetRepairState::Supplies || Job.Reason!=Why){Job.State=EFleetRepairState::Supplies;Job.Reason=Why;Result.bChanged=true;}continue;}
    // All requirements were preflighted. Candidate state is committed atomically by the host.
    const FEconomyInventory BeforeDebit=*DepotStore;bool Debited=true;
    for(const auto& Material:Job.Materials)
     if(!FEconomyLedger::Withdraw(Economy,FEconomyRuntime::IndustryOwner(),Job.Depot,Material.Key,Material.Value)){Debited=false;break;}
    if(!Debited){Economy.Inventories.FindChecked(Job.Depot)=BeforeDebit;Job.State=EFleetRepairState::Supplies;Job.Reason=TEXT("SUPPLY TRANSACTION BLOCKED");Result.bChanged=true;continue;}
    Job.bResourcesConsumed=true;
   }
   Job.State=EFleetRepairState::Running;Job.Reason=Job.bReplacement?TEXT("BUILDING REPLACEMENT HULL / MILSPEC III"):TEXT("FLEET REPAIRS IN PROGRESS");Job.LastAt=Context.Now;Hull->bRepairing=true;++Running;Result.bChanged=true;
  }
  const double Remaining=Job.DurationSeconds-Job.WorkedSeconds;
  const double Due=Context.Now+FMath::Min(Remaining,FMath::Max(1.,Job.DurationSeconds*.25));
  if(Result.NextAt==0 || Due<Result.NextAt)Result.NextAt=Due;
 }
 if(Result.bChanged)Fleet.bDirty=true;
 return Result;
}
bool RepairAI::Commit(FCampaignFleetState& Fleet,FEconomyState& Economy,const FFleetRepairContext& Context,const FFleetRepairPolicy& Policy,TFunctionRef<bool()> Persist,FFleetRepairStep& Result)
{
 auto CandidateFleet=Fleet;auto CandidateEconomy=Economy;
 FSimDataEvents::Suppress(true);Result=Advance(CandidateFleet,CandidateEconomy,Context,Policy);FSimDataEvents::Suppress(false);
 if(!Result.bChanged)return true;
 auto Before=MoveTemp(Economy);Economy=MoveTemp(CandidateEconomy);
 if(!Persist()){Economy=MoveTemp(Before);Result={};return false;}
 Fleet=MoveTemp(CandidateFleet);FSimDataEvents::Publish(Economy);return true;
}
FString RepairAI::Describe(const FCampaignFleetState& Fleet,const FEconomyState& Economy,FGuid Hull,double Now)
{
 const FFleetRepairOrder* Latest=nullptr;
 for(const auto& Pair:Economy.FleetRepairOrders)if(Pair.Value.FleetId==Fleet.FleetId && (Pair.Value.HullId==Hull || Pair.Value.ReplacementId==Hull) && (!Latest || Pair.Value.CreatedAt>Latest->CreatedAt))Latest=&Pair.Value;
 if(!Latest)return TEXT("NO FLEET WORK ORDER");
 if(Latest->State!=EFleetRepairState::Running)return Latest->Reason;
 const double Remaining=FMath::Max(0.,Latest->DurationSeconds-Latest->WorkedSeconds-FMath::Max(0.,Now-Latest->LastAt));
 return FString::Printf(TEXT("%s | %.0f%% | %.0f MIN REMAINING"),*Latest->Reason,Latest->Progress()*100,FMath::CeilToDouble(Remaining/60));
}
