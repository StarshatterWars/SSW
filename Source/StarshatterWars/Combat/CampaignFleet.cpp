#include "CampaignFleet.h"
#include "Game.h"
#include "Campaign.h"
#include "Combatant.h"
#include "CombatGroup.h"
#include "CombatUnit.h"
#include "Ship.h"
#include "ShipDesign.h"
#include "Sim.h"
#include "SimRegion.h"
#include "StarSystem.h"
#include "OwnedHullCondition.h"
#include "HullRegistry.h"
#include "Mission.h"
#include "MissionElement.h"
#include "Instruction.h"
#include "ShipDesignRegistry.h"
#include "Drive.h"
#include "Power.h"

FString FCampaignFleet::UnitKey(const CombatUnit& Unit)
{
 const auto* Group=Unit.GetCombatGroup();if(!Group)return {};
 int32 Index=INDEX_NONE;auto& Units=const_cast<CombatGroup*>(Group)->GetUnits();
 for(int32 I=0;I<Units.size();++I)if(Units[I]==&Unit){Index=I;break;}
 if(Index==INDEX_NONE)return {};
 FString Path=FString::FromInt(Index);
 for(auto* Parent=const_cast<CombatGroup*>(Group);Parent;Parent=Parent->GetParent())
     Path=FString::Printf(TEXT("%d:%d/"),int32(Parent->GetType()),Parent->GetID())+Path;
 return FString::Printf(TEXT("%d/"),Unit.GetIFF())+Path;
}
namespace
{
void VisitGroup(CombatGroup* Group,TFunctionRef<void(CombatUnit&)> Visit)
{
 if(!Group)return;
 for(int32 I=0;I<Group->GetUnits().size();++I)if(Group->GetUnits()[I])Visit(*Group->GetUnits()[I]);
 for(int32 I=0;I<Group->GetComponents().size();++I)VisitGroup(Group->GetComponents()[I],Visit);
}
void VisitUnits(Campaign& Story,TFunctionRef<void(CombatUnit&)> Visit)
{
 for(int32 I=0;I<Story.GetCombatants().size();++I)if(Story.GetCombatants()[I])VisitGroup(Story.GetCombatants()[I]->GetForce(),Visit);
}
}
void FCampaignFleet::Prepare(Campaign& Story)
{
 auto& Fleet=Story.GetPersistentFleet();
 if(Fleet.bInitialized && !Fleet.bDirty)return;
 if(!Fleet.bInitialized)
 {
  if(Story.GetCombatants().size()==0)return;
  Fleet.CampaignId=Story.GetCampaignId();
  VisitUnits(Story,[&](CombatUnit& Unit)
  {
   if(Unit.GetShipClass()==int32(CLASSIFICATION::MINE))return;
   const FString Key=UnitKey(Unit);if(Key.IsEmpty())return;
   auto& Slots=Fleet.Units.FindOrAdd(Key).HullIds;
   Fleet.LastAppliedUnavailable.Add(Key,Unit.DeadCount());
   for(int32 Index=0;Index<Unit.GetCount();++Index)
   {
    FCampaignFleetHull Row;Row.HullId=FHullRegistry::Allocate();Row.UnitKey=Key;Row.Slot=Index;
    Row.Name=UTF8_TO_TCHAR(Unit.GetName().data());Row.Design=FName(UTF8_TO_TCHAR(Unit.GetDesignName().data()));
    Unit.GetDesign();Row.Position=Unit.GetLocation();Row.bStatic=Unit.IsStatic() || Unit.IsGroundUnit();Row.Faction=Unit.GetIFF();Row.Region=UTF8_TO_TCHAR(Unit.GetRegion().data());
    if(auto* Group=Unit.GetCombatGroup()){Row.Assignment=UTF8_TO_TCHAR(Group->GetName().data());Row.System=UTF8_TO_TCHAR(Group->GetAssignedSystem().data());if(Row.Region.IsEmpty())Row.Region=UTF8_TO_TCHAR(Group->GetRegion().data());}
    Row.bHero=Row.Faction==1 && (Row.Name.Equals(TEXT("Orion"),ESearchCase::IgnoreCase) || Row.Name.Equals(TEXT("CV6-Orion"),ESearchCase::IgnoreCase) || Row.Name.Equals(TEXT("Archon"),ESearchCase::IgnoreCase));
    Row.bDestroyed=Index>=Unit.GetCount()-Unit.DeadCount();if(Row.bHero && Row.bDestroyed)Fleet.bHeroLost=true;
    if(const auto* Design=Unit.GetDesign();Design && Design->integrity>0 && Unit.GetSustainedDamage()>0)
    {Row.Condition.bHasCondition=true;Row.Condition.HullCondition=FMath::Clamp(float(1.-Unit.GetSustainedDamage()/Design->integrity),0.f,1.f);}
    // Already quoted warehouse hulls retain their pre-existing physical VIN.
    if(auto* Simulation=Sim::GetSim())
    {
        const FSerializedHullStock* Match=nullptr;
        for(const auto& Pair:Simulation->ReadEconomy().SerializedHulls)
        {
            const auto& Stock=Pair.Value;
            if(Stock.Design==Row.Design && Stock.Slot==Index && Stock.Squadron.Equals(Row.Assignment,ESearchCase::IgnoreCase))
            {if(Match){Match=nullptr;break;}Match=&Stock;}
        }
        if(Match && !Fleet.Hulls.Contains(Match->HullId)){Row.HullId=Match->HullId;Row.bTransferred=Match->SoldReceipt.IsValid();}
    }
    if(Unit.GetCount()==1)if(auto* Simulation=Sim::GetSim())
    {
        Ship* Match=nullptr;bool Ambiguous=false;
        for(int32 R=0;R<Simulation->GetRegions().size();++R)
            for(int32 V=0;V<Simulation->GetRegions()[R]->GetShips().size();++V)
                if(auto* Vessel=Simulation->GetRegions()[R]->GetShips()[V];Vessel && Vessel->Design() &&
                   FName(UTF8_TO_TCHAR(Vessel->Design()->name))==Row.Design && Vessel->GetIFF()==Row.Faction &&
                   (Vessel->GetCombatUnit()==&Unit || Row.Name.Equals(UTF8_TO_TCHAR(Vessel->GetName()),ESearchCase::IgnoreCase)))
                {if(Match){Ambiguous=true;break;}Match=Vessel;}
        if(Match && !Ambiguous && !Fleet.Hulls.Contains(Match->GetHullId()))
        {
            Row.HullId=Match->GetHullId();Row.Position=OtherHand(Match->GetLocation());FOwnedHullCondition::Capture(*Match,Row.Condition);
            if(auto* Region=Match->GetRegion()){Row.Region=UTF8_TO_TCHAR(Region->GetName());if(auto* System=Region->GetSystem())Row.System=UTF8_TO_TCHAR(System->GetName());}
            Row.bDestroyed=Match->IsDead() || Match->IsDying();if(Row.bHero && Row.bDestroyed)Fleet.bHeroLost=true;
        }
    }
    Slots.Add(Row.HullId);Fleet.Hulls.Add(Row.HullId,MoveTemp(Row));
   }
  });
  Fleet.bInitialized=true;
 }
 if(Fleet.CampaignId!=Story.GetCampaignId())return;
 if(auto* Simulation=Sim::GetSim())for(auto& Pair:Fleet.Hulls)
     if(const auto* Stock=Simulation->ReadEconomy().SerializedHulls.Find(Pair.Key);Stock && Stock->SoldReceipt.IsValid())Pair.Value.bTransferred=true;

 VisitUnits(Story,[&](CombatUnit& Unit)
 {
  if(Unit.GetShipClass()==int32(CLASSIFICATION::MINE)){Unit.SetDeadCount(Fleet.ClearedMinefields.Contains(UnitKey(Unit))?Unit.GetCount():0);return;}
  const auto* Slots=Fleet.Units.Find(UnitKey(Unit));if(!Slots)return;
  int32 Recorded=0;for(const auto Id:Slots->HullIds)if(const auto* Hull=Fleet.Hulls.Find(Id);Hull && Hull->bDestroyed && !Hull->bTransferred)++Recorded;
  int32 ExtraLoss=FMath::Max(0,Unit.DeadCount()-FMath::Max(Recorded,Fleet.LastAppliedUnavailable.FindRef(UnitKey(Unit))));
  for(const auto Id:Slots->HullIds)if(auto* Row=Fleet.Hulls.Find(Id);Row && CanMove(*Row) && ExtraLoss>0)
  {Row->bDestroyed=true;--ExtraLoss;if(Row->bHero)Fleet.bHeroLost=true;}
  int32 Dead=0,ReadyHulls=0,Away=0;double Damage=0;
  for(const auto Id:Slots->HullIds)if(const auto* Row=Fleet.Hulls.Find(Id))
  {if(Row->bTransferred){++Away;continue;}if(Row->bDestroyed)++Dead;else {if(CanMove(*Row))++ReadyHulls;if(Row->Condition.bHasCondition && Unit.GetDesign())Damage=FMath::Max(Damage,(1.-Row->Condition.HullCondition)*Unit.GetDesign()->integrity);}}
  Unit.SetDeadCount(Dead);Unit.SetAssignedAway(Away);Unit.SetReadyCount(ReadyHulls);Fleet.LastAppliedUnavailable.Add(UnitKey(Unit),Dead);Unit.SetSustainedDamage(Damage);
 });
 Fleet.bDirty=false;
 if(Fleet.bHeroLost)Story.SetCampaignStatus(ECampaignStatus::FAILED);
}
FGuid FCampaignFleet::Resolve(Campaign& Story,const CombatUnit* Unit,int32 Index,bool MobileOnly)
{
 if(!Unit || Index<0)return {};Prepare(Story);
 const auto& Fleet=Story.GetPersistentFleet();const auto* Slots=Fleet.Units.Find(UnitKey(*Unit));if(!Slots)return {};
 for(int32 Pass=0;Pass<(MobileOnly?1:2);++Pass)
  for(const auto Id:Slots->HullIds)if(const auto* Row=Fleet.Hulls.Find(Id);Row && !Row->bDestroyed && !Row->bTransferred && (Pass==0?CanMove(*Row):!CanMove(*Row)))
      if(Index--==0)return Id;
 return {};
}
void FCampaignFleet::Apply(Campaign& Story,Ship& Vessel)
{
 const auto* Row=Story.GetPersistentFleet().Hulls.Find(Vessel.GetHullId());
 if(Row && !Row->bDestroyed)
 {
  FOwnedHullCondition::Apply(Vessel,Row->Condition);if(Row->bHero)Vessel.SetInvulnerable(false);
  if(!CanMove(*Row)){Vessel.SetThrottle(0);Vessel.SetThrottleRequest(0);Vessel.SetVelocity(FVector::ZeroVector);Vessel.SetAngularVelocity(FVector::ZeroVector);}
 }
}
void FCampaignFleet::Destroy(Campaign& Story,FGuid Id)
{
 auto& Fleet=Story.GetPersistentFleet();auto* Row=Fleet.Hulls.Find(Id);if(!Row || Row->bDestroyed)return;
 Fleet.bDirty=true;Row->bDestroyed=true;Row->bRepairing=false;Row->Condition.HullCondition=0;
 if(Row->bHero){Fleet.bHeroLost=true;Story.SetCampaignStatus(ECampaignStatus::FAILED);}
}
void FCampaignFleet::Capture(Campaign& Story,Sim& Simulation)
{
 Prepare(Story);auto& Fleet=Story.GetPersistentFleet();
 VisitUnits(Story,[&](CombatUnit& Unit)
 {
  const auto* Slots=Fleet.Units.Find(UnitKey(Unit));if(!Slots)return;
  for(const auto Id:Slots->HullIds)if(auto* Hull=Fleet.Hulls.Find(Id);Hull && !Hull->bDestroyed && !Hull->bTransferred)
  {
   const FString Region=UTF8_TO_TCHAR(Unit.GetRegion().data());
   if(!Region.IsEmpty() && Region!=Hull->Region)
   {Hull->Region=Region;if(auto* Group=Unit.GetCombatGroup())Hull->System=UTF8_TO_TCHAR(Group->GetAssignedSystem().data());}
   Hull->Position=Unit.GetLocation();
  }
 });
 for(int32 R=0;R<Simulation.GetRegions().size();++R)
 {
  auto& Ships=Simulation.GetRegions()[R]->GetShips();
  for(int32 I=0;I<Ships.size();++I)if(auto* Vessel=Ships[I])if(auto* Row=Fleet.Hulls.Find(Vessel->GetHullId()))
  {
   if(Row->bDestroyed)continue;
   if(Vessel->IsDead() || Vessel->IsDying()){Destroy(Story,Vessel->GetHullId());continue;}
   FOwnedHullCondition::Capture(*Vessel,Row->Condition);
   Row->Name=UTF8_TO_TCHAR(Vessel->GetName());Row->Position=OtherHand(Vessel->GetLocation());
   Row->Region=UTF8_TO_TCHAR(Vessel->GetRegion()->GetName());
   if(auto* Unit=Vessel->GetCombatUnit())Unit->SetRegion(Vessel->GetRegion()->GetName());
   if(auto* System=Vessel->GetRegion()->GetSystem())Row->System=UTF8_TO_TCHAR(System->GetName());
  }
 }
 Fleet.bDirty=true;Prepare(Story);
}
bool FCampaignFleet::Ready(const FCampaignFleetState& Fleet,const FString& Unit)
{
 const auto* Slots=Fleet.Units.Find(Unit);if(!Slots)return true;
 for(const auto Id:Slots->HullIds)if(const auto* Hull=Fleet.Hulls.Find(Id);Hull && CanMove(*Hull))return true;
 return false;
}
bool FCampaignFleet::ClearMinefield(FCampaignFleetState& Fleet,const FString& Key,bool Designated,bool Successful)
{
 if(!Designated || !Successful || Key.IsEmpty() || Fleet.ClearedMinefields.Contains(Key))return false;
 Fleet.ClearedMinefields.Add(Key);Fleet.bDirty=true;return true;
}

FGuid FCampaignFleet::ResolveSlot(Campaign& Story,const CombatUnit* Unit,int32 Slot)
{
 if(!Unit || Slot<0)return {};Prepare(Story);auto& Fleet=Story.GetPersistentFleet();
 const auto* Slots=Fleet.Units.Find(UnitKey(*Unit));if(!Slots || !Slots->HullIds.IsValidIndex(Slot))return {};
 const auto* Hull=Fleet.Hulls.Find(Slots->HullIds[Slot]);return Hull && CanMove(*Hull)?Hull->HullId:FGuid();
}

bool FCampaignFleet::CanMove(const FCampaignFleetHull& Hull)
{
 if(Hull.bDestroyed || Hull.bTransferred)return false;
 if(Hull.bStatic)return true;
 if(!Hull.Condition.bHasCondition)return true;
 if(!FMath::IsFinite(Hull.Condition.HullCondition) || Hull.Condition.HullCondition<.35f)return false;
 bool HasDrive=false,HasPower=false;float DriveHealth=0,PowerHealth=0;
 for(const auto& System:Hull.Condition.SystemConditions)
 {
  const FString Name=System.SystemName.ToString();const float Health=FMath::IsFinite(System.Availability)?System.Availability:0;
  if(Name.Contains(TEXT("engine")) || Name.Contains(TEXT("drive")) || Name.Contains(TEXT("thruster"))){HasDrive=true;DriveHealth=FMath::Max(DriveHealth,Health);}
  if(Name.Contains(TEXT("reactor")) || Name.Contains(TEXT("power"))){HasPower=true;PowerHealth=FMath::Max(PowerHealth,Health);}
 }
 if((HasDrive && DriveHealth<.25f) || (HasPower && PowerHealth<.25f))return false;
 return true;
}
bool FCampaignFleet::MissionReady(Campaign& Story,Mission& Assignment)
{
 const auto& Fleet=Story.GetPersistentFleet();if(Fleet.bHeroLost)return false;
 auto* Player=Assignment.GetPlayer();
 for(int32 I=0;I<Assignment.GetElements().size();++I)
 {
  auto* Element=Assignment.GetElements()[I];if(!Element || !Element->GetCombatUnit())continue;
  const auto* Slots=Fleet.Units.Find(UnitKey(*Element->GetCombatUnit()));if(!Slots)continue;
  bool Required=Element==Player || (Player && Element->GetName()==Player->GetCarrier());
  for(const auto Id:Slots->HullIds)if(const auto* Hull=Fleet.Hulls.Find(Id);Hull && Hull->bHero)Required=true;
  bool TargetRequired=false;
  if(Player)for(int32 O=0;O<Player->GetObjectives().size();++O)
   if(const auto* Objective=Player->GetObjectives()[O];Objective && Objective->GetPriority()==Instruction::PRIMARY && Element->GetName()==Objective->GetTargetName())
   {
    if(Objective->GetAction()==EInstruction::Escort || Objective->GetAction()==EInstruction::Defend)Required=true;
    if(Objective->GetAction()==EInstruction::Assault || Objective->GetAction()==EInstruction::Strike)TargetRequired=true;
   }
  if(Required)
  {
   int32 ReadyHulls=0;for(const auto Id:Slots->HullIds)if(const auto* Hull=Fleet.Hulls.Find(Id);Hull && CanMove(*Hull))++ReadyHulls;
   if(ReadyHulls<Element->Count())return false;
  }
  if(TargetRequired)
  {
   bool Alive=false;for(const auto Id:Slots->HullIds)if(const auto* Hull=Fleet.Hulls.Find(Id);Hull && !Hull->bDestroyed && !Hull->bTransferred)Alive=true;
   if(!Alive)return false;
  }
 }
 return true;
}

FString FCampaignFleet::MinefieldKey(MissionElement& Element)
{
 const auto* Design=Element.GetShipDesign();
 if(!Design || Design->ShipType!=int32(CLASSIFICATION::MINE))return {};
 if(auto* Unit=Element.GetCombatUnit())return UnitKey(*Unit);
 return FString(UTF8_TO_TCHAR(Element.GetRegion().data()))+TEXT("/")+UTF8_TO_TCHAR(Element.GetName().data());
}
void FCampaignFleet::CommitObjectives(Campaign& Story,Mission& Assignment)
{
 if(!Assignment.IsComplete() || !Assignment.GetPlayer())return;
 auto& Objectives=Assignment.GetPlayer()->GetObjectives();
 for(int32 I=0;I<Objectives.size();++I)
 {
  const auto* Objective=Objectives[I];if(!Objective || Objective->GetStatus()!=INSTRUCTION_STATUS::COMPLETE || Objective->GetPriority()!=Instruction::PRIMARY)continue;
  const auto Action=Objective->GetAction();if(Action!=EInstruction::Strike && Action!=EInstruction::Assault && Action!=EInstruction::Sweep)continue;
  for(int32 E=0;E<Assignment.GetElements().size();++E)if(auto* Target=Assignment.GetElements()[E];Target && Target->GetName()==Objective->GetTargetName())
      ClearMinefield(Story.GetPersistentFleet(),MinefieldKey(*Target),true,true);
 }
}

bool FCampaignFleet::CanMove(Ship& Vessel)
{
 if(Vessel.IsDead() || Vessel.IsDying() || !Vessel.Design() || Vessel.GetIntegrity()<.35*Vessel.Design()->integrity)return false;
 double Drive=0,Power=0;
 for(int32 I=0;I<Vessel.GetDrives().size();++I)if(Vessel.GetDrives()[I])Drive=FMath::Max(Drive,Vessel.GetDrives()[I]->GetAvailability());
 for(int32 I=0;I<Vessel.GetReactors().size();++I)if(Vessel.GetReactors()[I])Power=FMath::Max(Power,Vessel.GetReactors()[I]->GetAvailability());
 return (Vessel.GetDrives().isEmpty() || Drive>=25.) && (Vessel.GetReactors().isEmpty() || Power>=25.);
}

FString FCampaignFleet::Status(const FCampaignFleetHull& Hull)
{
 if(Hull.bTransferred)return TEXT("TRANSFERRED");
 if(Hull.bDestroyed)return TEXT("DESTROYED");
 if(Hull.bRepairing)return CanMove(Hull)?TEXT("REPAIRING"):TEXT("REPAIRING / IMMOBILIZED");
 if(!CanMove(Hull))return TEXT("IMMOBILIZED");
 if(Hull.Condition.bHasCondition && (Hull.Condition.HullCondition<.999f || Hull.Condition.SystemConditions.ContainsByPredicate([](const auto& System){return System.Availability<.999f;})))return TEXT("DAMAGED");
 return TEXT("READY");
}

void FCampaignFleet::CaptureHull(Campaign& Story,Ship& Vessel)
{
 auto* Hull=Story.GetPersistentFleet().Hulls.Find(Vessel.GetHullId());if(!Hull || Hull->bDestroyed)return;
 if(Vessel.IsDead() || Vessel.IsDying()){Destroy(Story,Vessel.GetHullId());return;}
 Story.GetPersistentFleet().bDirty=true;FOwnedHullCondition::Capture(Vessel,Hull->Condition);
 Hull->Position=OtherHand(Vessel.GetLocation());
 if(auto* Region=Vessel.GetRegion()){Hull->Region=UTF8_TO_TCHAR(Region->GetName());if(auto* System=Region->GetSystem())Hull->System=UTF8_TO_TCHAR(System->GetName());}
}

void FCampaignFleet::RestoreLocations(Campaign& Story)
{
 auto& Fleet=Story.GetPersistentFleet();
 VisitUnits(Story,[&](CombatUnit& Unit)
 {
  const auto* Slots=Fleet.Units.Find(UnitKey(Unit));if(!Slots)return;
  for(const auto Id:Slots->HullIds)if(const auto* Hull=Fleet.Hulls.Find(Id);Hull && !Hull->bDestroyed && !Hull->bTransferred)
  {if(!Hull->Region.IsEmpty())Unit.RestoreFleetLocation(TCHAR_TO_UTF8(*Hull->Region),Hull->Position);if(auto* Group=Unit.GetCombatGroup();Group && !Hull->System.IsEmpty())Group->SetAssignedSystem(TCHAR_TO_UTF8(*Hull->System));break;}
 });
}
