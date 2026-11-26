#include "FleetRepairSubsystem.h"
#include "GalaxyTrafficSubsystem.h"
#include "CampaignFleet.h"
#include "Campaign.h"
#include "Sim.h"
#include "SimRegion.h"
#include "Mission.h"
#include "Ship.h"
#include "Hangar.h"
#include "ShipDesignRegistry.h"
#include "OwnedHullCondition.h"
#include "StarshatterPlayerSubsystem.h"
#include "TimerSubsystem.h"
#include "SSWGameInstance.h"
#include "ProductionCatalog.h"
#include "EconomyRuntime.h"
#include "SimDataEvents.h"
#include "FormattingUtils.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Engine/DataTable.h"

void UFleetRepairSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
 Super::Initialize(Collection);
 Collection.InitializeDependency<UStarshatterPlayerSubsystem>();
 Collection.InitializeDependency<UTimerSubsystem>();
 Collection.InitializeDependency<UGalaxyTrafficSubsystem>();
 FString Json;FText Error;
 bPolicyValid=FFileHelper::LoadFileToString(Json,*(FPaths::ProjectContentDir()/TEXT("GameData/Production/fleet_repairs.json"))) && Policy.LoadJson(Json,Error);
 if(!bPolicyValid)UE_LOG(LogTemp,Error,TEXT("[RepairAI] Repair scheduling disabled: invalid fleet_repairs.json. %s"),*Error.ToString());
 FleetHandle=FCampaignFleet::Changed().AddUObject(this,&UFleetRepairSubsystem::Request);
 EconomyHandle=FSimDataEvents::Changed().AddUObject(this,&UFleetRepairSubsystem::OnEconomyChanged);
}
void UFleetRepairSubsystem::Deinitialize()
{
 bStopping=true;
 FCampaignFleet::Changed().Remove(FleetHandle);FSimDataEvents::Changed().Remove(EconomyHandle);
 if(WakeHandle.IsValid()){FTSTicker::GetCoreTicker().RemoveTicker(WakeHandle);WakeHandle.Reset();}
 Super::Deinitialize();
}
void UFleetRepairSubsystem::Request()
{
 if(!bPolicyValid || bStopping)return;
 if(bProcessing){bWakeAgain=true;return;}
 if(WakeHandle.IsValid()){FTSTicker::GetCoreTicker().RemoveTicker(WakeHandle);WakeHandle.Reset();}
 WakeHandle=FTSTicker::GetCoreTicker().AddTicker(FTickerDelegate::CreateUObject(this,&UFleetRepairSubsystem::Process),.5f);
}
uint32 UFleetRepairSubsystem::SupplyFingerprint(const FEconomyState& State) const
{
 uint32 Result=0;const FName Parts=DataTableUtils::MakeValidName(TEXT("military spare parts")),Alloy=DataTableUtils::MakeValidName(TEXT("structural alloy")),Kits=DataTableUtils::MakeValidName(TEXT("repair kit"));
 for(const auto& Pair:State.Inventories)
  if(Pair.Key.ToString().StartsWith(TEXT("Fleet Repair/")) || (Pair.Value.bHullStorage && Pair.Value.bSupplyOnly))
  {
   uint32 Value=HashCombine(GetTypeHash(Pair.Key),GetTypeHash(Pair.Value.bOperational));Value=HashCombine(Value,GetTypeHash(Pair.Value.System));
   for(const auto& Stack:Pair.Value.Items)if(Stack.Type==Parts || Stack.Type==Alloy || Stack.Type==Kits)Value=HashCombine(Value,HashCombine(GetTypeHash(Stack.Type),GetTypeHash(Stack.Quantity)));
   Result^=Value;
  }
 return Result;
}
void UFleetRepairSubsystem::OnEconomyChanged(const FEconomyState& State)
{
 auto* Players=GetGameInstance()->GetSubsystem<UStarshatterPlayerSubsystem>();
 if(!Players || &State!=&Players->GetPlayerInfo().Economy)return;
 const uint32 Fingerprint=SupplyFingerprint(State);
 if(Fingerprint!=LastSupplyFingerprint){LastSupplyFingerprint=Fingerprint;Request();}
}
bool UFleetRepairSubsystem::Process(float)
{
 WakeHandle.Reset();
 auto* Game=Cast<USSWGameInstance>(GetGameInstance());auto* Players=Game?Game->GetSubsystem<UStarshatterPlayerSubsystem>():nullptr;
 auto* Timer=Game?Game->GetSubsystem<UTimerSubsystem>():nullptr;auto* Story=Campaign::GetCampaign();auto* Simulation=Sim::GetSim();
 if(!Game || !Players || !Players->HasLoaded() || !Timer || !Game->CampaignSave || !Story || Players->GetPlayerInfo().Campaign!=Story->GetCampaignId() ||
    Story->GetCampaignId()>=Campaign::SINGLE_MISSIONS || (Simulation && (!Simulation->HasSimulationAuthority() || Simulation->IsNetworked())))return false;
 bProcessing=true;bWakeAgain=false;
 FCampaignFleet::Prepare(*Story);
 if(Simulation)FCampaignFleet::Capture(*Story,*Simulation);
 auto& Fleet=Story->GetPersistentFleet();if(!Fleet.bInitialized || Fleet.bHeroLost){bProcessing=false;return false;}
 FFleetRepairContext Context;Context.Now=double(Timer->GetUniverseTimeSeconds());
 if(auto* Pilot=Simulation?Simulation->GetPlayerShip():nullptr)Context.AlliedFaction=Pilot->GetIFF();
 // The flagship chooses the one-time initial supply location; other system
 // depots receive stock only through industry and freight.
 TArray<FGuid> IDs;Fleet.Hulls.GetKeys(IDs);IDs.Sort([](const FGuid& A,const FGuid& B){return A.ToString()<B.ToString();});
 for(FGuid Id:IDs){const auto& Hull=Fleet.Hulls.FindChecked(Id);if(Hull.Faction==Context.AlliedFaction && !Hull.bTransferred && !Hull.bDestroyed && !Hull.System.IsEmpty())
 {if(Context.SupplySystem.IsNone() || Hull.bHero)Context.SupplySystem=FName(*Hull.System);if(Hull.bHero)break;}}
 for(const auto& Pair:ShipDesignRegistry::GetAll())
 {FRepairHullSpec Spec;Spec.Mass=Pair.Value.Mass;Spec.bFighter=Pair.Value.ShipType==int32(CLASSIFICATION::FIGHTER) || Pair.Value.ShipType==int32(CLASSIFICATION::ATTACK);Context.Designs.Add(Pair.Key,Spec);}
 const FString Empire=UFormattingUtils::GetEmpireNameFromType(static_cast<EEMPIRE_NAME>(Players->GetPlayerInfo().Empire));
 for(const auto& Pair:SSWProductionCatalog::Facilities())
  if(Pair.Value.FacilityType==ESSWProductionFacilityType::Shipyard && Pair.Value.Empire.Equals(Empire,ESearchCase::IgnoreCase))
   Context.AuthorizedShipyards.Add(FEconomyRuntime::Warehouse(Pair.Value.System,Pair.Value.Facility,TEXT("Output")));
 if(Simulation)for(int32 R=0;R<Simulation->GetRegions().size();++R)
  if(auto* Region=Simulation->GetRegions()[R])for(int32 V=0;V<Region->GetShips().size();++V)
   if(auto* Vessel=Region->GetShips()[V];Vessel && Fleet.Hulls.Contains(Vessel->GetHullId()))
   {
    const auto Phase=Vessel->GetFlightPhase();
    const bool Docked=Phase==EOPSMode::DOCKED || Phase==EOPSMode::ALERT || Phase==EOPSMode::LOCKED;
    // No remote fleet repair of a ship still participating in active combat.
    const bool Combat=Simulation->GetMission() && Story->OwnsMission(Simulation->GetMission()) && !Simulation->GetMission()->IsComplete() && !Simulation->GetMission()->IsFreeFlight();
    if((Vessel->IsDropship() && !Docked) || (!Vessel->IsDropship() && (Combat || Vessel->GetVelocity().Size()>1. || !Vessel->GetThreatList().isEmpty())))Context.Deployed.Add(Vessel->GetHullId());
   }
 auto* Domain=GetGameInstance()->GetSubsystem<UGalaxyTrafficSubsystem>();
 FNativeFleetRepairCommand Command;Command.Fleet=Fleet;Command.Context=Context;Command.Policy=Policy;
 const int32 CampaignId=Story->GetCampaignId();const FGuid FleetId=Fleet.FleetId;const TWeakObjectPtr<UFleetRepairSubsystem> Weak(this);FText Why;
 const auto CurrentFleet=[Weak,CampaignId,FleetId]()->FCampaignFleetState*
 {
    auto* Current=Campaign::GetCampaign();
    return Weak.IsValid() && Current && Current->GetCampaignId()==CampaignId && Current->GetPersistentFleet().FleetId==FleetId?&Current->GetPersistentFleet():nullptr;
 };
 const bool Queued=Domain && Domain->QueueFleetRepair(MoveTemp(Command),TFunction<FCampaignFleetState*()>(CurrentFleet),
    [CurrentFleet,Context,Simulation](FText& Reason)
    {
        auto* Current=CurrentFleet();auto* Live=Sim::GetSim();
        if(!Current || Live!=Simulation || (Live && (!Live->HasSimulationAuthority() || Live->IsNetworked())))return false;
        TSet<FGuid> Deployed;
        if(Live)for(int32 R=0;R<Live->GetRegions().size();++R)
            if(auto* Region=Live->GetRegions()[R])for(int32 V=0;V<Region->GetShips().size();++V)
                if(auto* Vessel=Region->GetShips()[V];Vessel && Current->Hulls.Contains(Vessel->GetHullId()))
                {
                    const auto Phase=Vessel->GetFlightPhase();const bool Docked=Phase==EOPSMode::DOCKED || Phase==EOPSMode::ALERT || Phase==EOPSMode::LOCKED;
                    auto* CampaignNow=Campaign::GetCampaign();const bool Combat=Live->GetMission() && CampaignNow && CampaignNow->OwnsMission(Live->GetMission()) && !Live->GetMission()->IsComplete() && !Live->GetMission()->IsFreeFlight();
                    if((Vessel->IsDropship() && !Docked) || (!Vessel->IsDropship() && (Combat || Vessel->GetVelocity().Size()>1. || !Vessel->GetThreatList().isEmpty())))Deployed.Add(Vessel->GetHullId());
                }
        bool Same=Deployed.Num()==Context.Deployed.Num();for(const auto Id:Deployed)Same&=Context.Deployed.Contains(Id);
        if(!Same)Reason=INVTEXT("Fleet deployment changed; repairs will be reassessed.");return Same;
    },[Weak,CampaignId,Context](bool Saved,const FFleetRepairStep& Step,const FText& Reason)
    {if(auto* Owner=Weak.Get())Owner->CompleteWork(CampaignId,Context,Saved,Step,Reason);},Why);
 if(!Queued)CompleteWork(CampaignId,Context,false,{},Why);
 return false;
}
void UFleetRepairSubsystem::CompleteWork(int32 CampaignId,const FFleetRepairContext& Context,bool Saved,const FFleetRepairStep& Result,const FText&)
{
 bProcessing=false;if(bStopping)return;
 auto* Game=Cast<USSWGameInstance>(GetGameInstance());auto* Players=Game?Game->GetSubsystem<UStarshatterPlayerSubsystem>():nullptr;
 auto* Story=Campaign::GetCampaign();auto* Simulation=Sim::GetSim();
 if(!Players || !Players->HasLoaded() || !Story || Story->GetCampaignId()!=CampaignId)return;
 if(!Saved)
 {
    WakeHandle=FTSTicker::GetCoreTicker().AddTicker(FTickerDelegate::CreateUObject(this,&UFleetRepairSubsystem::Process),2.f);return;
 }
 auto& Fleet=Story->GetPersistentFleet();auto& Economy=Players->GetEconomy();
 if(Result.bChanged)
 {
  FCampaignFleet::Prepare(*Story);
  if(Simulation)for(int32 R=0;R<Simulation->GetRegions().size();++R)
   if(auto* Region=Simulation->GetRegions()[R])for(int32 V=0;V<Region->GetShips().size();++V)
    if(auto* Vessel=Region->GetShips()[V])
    {
     if(const auto* Hull=Fleet.Hulls.Find(Vessel->GetHullId());Hull && !Hull->bDestroyed && !Context.Deployed.Contains(Hull->HullId))
     {auto Condition=Hull->Condition;FOwnedHullCondition::CaptureAmmo(*Vessel,Condition);FOwnedHullCondition::Apply(*Vessel,Condition);}
     if(auto* Inventory=Vessel->GetHangar())Inventory->RefreshCampaignFleet();
    }
  // If this second save fails, paid work remains durable in the economy and
  // idempotently reconciles to the older campaign snapshot on the next wake.
  if(!Game->SaveCampaign())UE_LOG(LogTemp,Error,TEXT("[RepairAI] Campaign snapshot pending; durable repair orders retained for reconciliation."));
 }
 LastSupplyFingerprint=SupplyFingerprint(Economy);
 if(Result.NextAt>Context.Now)
  WakeHandle=FTSTicker::GetCoreTicker().AddTicker(FTickerDelegate::CreateUObject(this,&UFleetRepairSubsystem::Process),float(FMath::Clamp(Result.NextAt-Context.Now,1.,86400.)));
 if(bWakeAgain){bWakeAgain=false;Request();}
}
