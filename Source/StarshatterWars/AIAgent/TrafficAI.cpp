#include "TrafficAI.h"
#include "EconomyRuntime.h"
#include "Algo/Reverse.h"
#include "Ship.h"
#include "Sim.h"
#include "Starshatter.h"
#include "OfflineCutsceneExport.h"

void TrafficAI::TickLegacyIndustry(Sim& Simulation,double Seconds)
{
    const auto* Director=Starshatter::GetInstance();
    if(!Simulation.HasSimulationAuthority() || !FMath::IsFinite(Seconds) || Seconds<=0 ||
        (Director && Director->InCutscene()) || OfflineCutsceneExport::IsRequested())return;
    Simulation.TickIndustrialActors(Seconds);
    Simulation.TickLegacyJanusActors(Seconds);
}

bool TrafficAI::Eligible(bool Authority,bool Playing,bool Cinematic,bool Exporting)
{return Authority && Playing && !Cinematic && !Exporting;}
TArray<FName> TrafficAI::Route(const TMap<FName,FGalaxyTrafficSector>& Sectors,FName From,FName To)
{
    if(!Sectors.Contains(From) || !Sectors.Contains(To))return {};
    TArray<FName> Queue{From};TMap<FName,FName> Previous;Previous.Add(From,NAME_None);
    for(int32 Index=0;Index<Queue.Num();++Index)
    {
        const FName Current=Queue[Index];
        if(Current==To)
        {TArray<FName> Path;for(FName Step=To;!Step.IsNone();Step=Previous.FindChecked(Step))Path.Add(Step);Algo::Reverse(Path);return Path;}
        auto Next=Sectors.FindChecked(Current).Neighbors;Next.Sort([](FName A,FName B){return A.LexicalLess(B);});
        for(FName Neighbor:Next)if(Sectors.Contains(Neighbor) && !Previous.Contains(Neighbor)){Previous.Add(Neighbor,Current);Queue.Add(Neighbor);}
    }
    return {};
}
FGuid TrafficAI::Schedule(FGalaxyTrafficState& Traffic,FEconomyState& Economy,const FGalaxyTrafficEndpoint& From,
    const FGalaxyTrafficEndpoint& To,const TArray<FName>& Path,FName Source,FName Destination,FName Item,double Quantity,double Mass)
{
    if(From.Id==To.Id || Path.IsEmpty() || Path[0]!=From.Sector || Path.Last()!=To.Sector || Traffic.Journeys.Num()>=512)return {};
    FGalaxyTrafficJourney Job;Job.Id=FGuid::NewGuid();Job.Origin=From.Id;Job.Destination=To.Id;Job.CurrentSector=From.Sector;Job.Route=Path;
    Job.Hold=FName(*(TEXT("Traffic/")+Job.Id.ToString(EGuidFormats::Digits)));Job.Remaining=2;
    if(!Item.IsNone())
    {
        const auto* Pickup=Economy.Inventories.Find(Source);const auto* Drop=Economy.Inventories.Find(Destination);
        if(!Pickup || !Drop || Pickup->Owner!=FEconomyRuntime::IndustryOwner() || Drop->Owner!=Pickup->Owner || Source==Destination ||
            !FMath::IsFinite(Quantity) || Quantity<=0 || !FMath::IsFinite(Mass) || Mass<=0 || Quantity*Mass>1000 ||
            Drop->CapacityTonnes-FEconomyLedger::Used(*Drop)<Quantity*Mass)return {};
        const auto* Stock=Pickup->Items.FindByPredicate([Item](const auto& Stack){return Stack.Type==Item;});
        const auto* Existing=Drop->Items.FindByPredicate([Item](const auto& Stack){return Stack.Type==Item;});
        if(!Stock || !FMath::IsNearlyEqual(Stock->UnitTonnes,Mass) || (Existing && !FMath::IsNearlyEqual(Existing->UnitTonnes,Mass)))return {};
        Job.Reservation=FEconomyLedger::Reserve(Economy,Pickup->Owner,Source,Item,Quantity);
        if(!Job.Reservation.IsValid())return {};
        if(!FEconomyLedger::CreateInventory(Economy,Job.Hold,Pickup->Owner,From.System,1000))
        {FEconomyLedger::Release(Economy,Pickup->Owner,Job.Reservation);return {};}
        Job.Item=Item;Job.Quantity=Quantity;Job.UnitTonnes=Mass;Job.SourceInventory=Source;Job.DestinationInventory=Destination;
        Economy.Inventories.FindChecked(Destination).ReservedOutputTonnes+=Quantity*Mass;Job.bDestinationReserved=true;
    }
    Traffic.Journeys.Add(Job);Traffic.SectorLastServed.Add(From.Sector,Traffic.Clock);return Job.Id;
}
bool TrafficAI::Advance(FGalaxyTrafficJourney& Job,FEconomyState& Economy,double Seconds,bool Realized,bool Arrived,double HopSeconds,double Rate)
{
    if(Job.IsFinished() || !FMath::IsFinite(Seconds) || Seconds<=0 || !FMath::IsFinite(HopSeconds) || HopSeconds<=0 || !FMath::IsFinite(Rate) || Rate<=0)return false;
    Job.Age+=Seconds;
    const FName Owner=FEconomyRuntime::IndustryOwner();
    if(Job.Phase==EGalaxyTrafficPhase::Blocked)
    {if(FEconomyLedger::Quantity(Economy,Job.Hold,Job.Item)>=Job.Quantity){Job.Phase=EGalaxyTrafficPhase::Unloading;Job.Remaining=5;}else Lose(Job,Economy);return false;}
    if((Job.Phase==EGalaxyTrafficPhase::Loading || Job.Phase==EGalaxyTrafficPhase::Unloading) && Realized && !Arrived)return false;
    Job.Remaining=FMath::Max(0.0,Job.Remaining-Seconds);
    if(Job.Remaining>0)return false;
    switch(Job.Phase)
    {
    case EGalaxyTrafficPhase::Waiting:
        Job.Phase=Job.IsCargo()?EGalaxyTrafficPhase::Loading:EGalaxyTrafficPhase::Traveling;
        Job.Remaining=Job.IsCargo()?FMath::Max(1.0,Job.Quantity*Job.UnitTonnes/Rate):HopSeconds;break;
    case EGalaxyTrafficPhase::Loading:
        if(!FEconomyLedger::Transfer(Economy,Owner,Job.SourceInventory,Job.Hold,Job.Item,Job.Quantity,Job.Reservation))
        {Job.BlockedReason=TEXT("Pickup stock or reservation unavailable.");Lose(Job,Economy);return false;}
        Job.Reservation.Invalidate();Job.Phase=EGalaxyTrafficPhase::Traveling;Job.Remaining=HopSeconds;break;
    case EGalaxyTrafficPhase::Traveling:
        if(Realized && !Arrived)return false;
        if(!Realized && Job.RouteIndex+1<Job.Route.Num())
        {++Job.RouteIndex;Job.CurrentSector=Job.Route[Job.RouteIndex];Job.Remaining=HopSeconds;return false;}
        if(Realized){Job.RouteIndex=Job.Route.Num()-1;Job.CurrentSector=Job.Route.Last();}
        Job.Phase=Job.IsCargo()?EGalaxyTrafficPhase::Unloading:EGalaxyTrafficPhase::Completed;
        Job.Remaining=FMath::Max(1.0,Job.Quantity*Job.UnitTonnes/Rate);break;
    case EGalaxyTrafficPhase::Unloading:
        if(Job.bDestinationReserved)
        {
            if(auto* Drop=Economy.Inventories.Find(Job.DestinationInventory))Drop->ReservedOutputTonnes=FMath::Max(0.0,Drop->ReservedOutputTonnes-Job.Quantity*Job.UnitTonnes);
            Job.bDestinationReserved=false;
        }
        if(!FEconomyLedger::Transfer(Economy,Owner,Job.Hold,Job.DestinationInventory,Job.Item,Job.Quantity))
        {Job.Phase=EGalaxyTrafficPhase::Blocked;Job.BlockedReason=TEXT("Destination storage unavailable; cargo remains aboard.");return false;}
        Job.Phase=EGalaxyTrafficPhase::Completed;Economy.Inventories.Remove(Job.Hold);return true;
    default:break;
    }
    return Job.Phase==EGalaxyTrafficPhase::Completed;
}
void TrafficAI::Lose(FGalaxyTrafficJourney& Job,FEconomyState& Economy)
{
    if(Job.IsFinished())return;
    if(Job.Reservation.IsValid())FEconomyLedger::Release(Economy,FEconomyRuntime::IndustryOwner(),Job.Reservation);
    if(Job.bDestinationReserved)
        if(auto* Drop=Economy.Inventories.Find(Job.DestinationInventory))Drop->ReservedOutputTonnes=FMath::Max(0.0,Drop->ReservedOutputTonnes-Job.Quantity*Job.UnitTonnes);
    Job.bDestinationReserved=false;
    if(Job.Hold.ToString().StartsWith(TEXT("Ship/")))
    {
        if(auto* Hold=Economy.Inventories.Find(Job.Hold))
        {Hold->Items.Reset();Hold->ManifestItems.Reset();Hold->CapacityTonnes=0;Hold->ReservedOutputTonnes=0;}
    }
    else Economy.Inventories.Remove(Job.Hold);
    Job.Reservation.Invalidate();Job.Phase=EGalaxyTrafficPhase::Lost;
}
bool TrafficAI::SafeAnchor(const FVector& Candidate,double Radius,const TArray<TPair<FVector,double>>& Obstacles)
{
    if(Candidate.ContainsNaN() || !FMath::IsFinite(Radius) || Radius<=0)return false;
    for(const auto& Obstacle:Obstacles)
        if(Obstacle.Key.ContainsNaN() || !FMath::IsFinite(Obstacle.Value) || Obstacle.Value<0 || FVector::DistSquared(Candidate,Obstacle.Key)<FMath::Square(Radius+Obstacle.Value+500))return false;
    return true;
}

const FGalaxyTrafficEndpoint* TrafficAI::EndpointForInventory(const TMap<FName,FGalaxyTrafficEndpoint>& Endpoints,FName Key,const FEconomyInventory& Inventory)
{
    const FString Text=Key.ToString();const FGalaxyTrafficEndpoint* Transfer=nullptr;
    for(const auto& Pair:Endpoints)
        if(Pair.Value.bPhysical && Pair.Value.System==Inventory.System)
        {
            if(Text.StartsWith(Pair.Value.System.ToString()+TEXT("/")+Pair.Value.Station.ToString()+TEXT("/")))return &Pair.Value;
            if(Ship::IsTransferStationName(Pair.Value.Station.ToString()) && (!Transfer || Pair.Key.LexicalLess(Transfer->Id)))Transfer=&Pair.Value;
        }
    return Text.Contains(TEXT("/NPC Market/"))?Transfer:nullptr;
}
void TrafficAI::ScheduleSector(FGalaxyTrafficState& Traffic,FEconomyState& Economy,const TMap<FName,FGalaxyTrafficSector>& Sectors,const TMap<FName,FGalaxyTrafficEndpoint>& Endpoints,FName Sector)
{
    TArray<FName> Keys;Economy.Inventories.GetKeys(Keys);Keys.Sort([](FName A,FName B){return A.LexicalLess(B);});
    int32 Examined=0;
    for(FName Source:Keys)
    {
        const auto& Inventory=Economy.Inventories.FindChecked(Source);
        if(Inventory.Owner!=FEconomyRuntime::IndustryOwner())continue;
        const auto* From=EndpointForInventory(Endpoints,Source,Inventory);if(!From || From->Sector!=Sector)continue;
        for(const auto& Item:Inventory.Items)
        {
            if(++Examined>256)break;
            const double Available=FEconomyLedger::Available(Economy,Source,Item.Type);if(Available<=0)continue;
            for(FName Destination:Keys)
            {
                if(Destination==Source)continue;const auto& Drop=Economy.Inventories.FindChecked(Destination);
                if(Drop.Owner!=Inventory.Owner || FEconomyLedger::Quantity(Economy,Destination,Item.Type)>=Available*.25)continue;
                const auto* To=EndpointForInventory(Endpoints,Destination,Drop);if(!To || To->Id==From->Id)continue;
                const auto Path=TrafficAI::Route(Sectors,From->Sector,To->Sector);if(Path.IsEmpty())continue;
                const double Count=FMath::Min3(Available,100.0/Item.UnitTonnes,FMath::Max(0.0,Drop.CapacityTonnes-FEconomyLedger::Used(Drop))/Item.UnitTonnes);
                if(Count>0 && TrafficAI::Schedule(Traffic,Economy,*From,*To,Path,Source,Destination,Item.Type,Count,Item.UnitTonnes).IsValid())return;
            }
        }
        if(Examined>256)break;
    }
    const FName FromId(*(Sector.ToString()+TEXT("/TransitA"))),ToId(*(Sector.ToString()+TEXT("/TransitB")));
    if(const auto* From=Endpoints.Find(FromId))if(const auto* To=Endpoints.Find(ToId))TrafficAI::Schedule(Traffic,Economy,*From,*To,{Sector});
}

void TrafficAI::Dispatch(FGalaxyTrafficState& Traffic,FEconomyState& Economy,const TMap<FName,FGalaxyTrafficSector>& Sectors,const TMap<FName,FGalaxyTrafficEndpoint>& Endpoints,FName Active,int32 MaxJourneys)
{
    if(Traffic.Journeys.Num()>=MaxJourneys)return;
    int32 Near=0;for(const auto& Job:Traffic.Journeys)if(!Job.IsFinished() && Job.CurrentSector==Active)++Near;
    if(Near<2 && Sectors.Contains(Active))ScheduleSector(Traffic,Economy,Sectors,Endpoints,Active);
    FName Least;double Oldest=TNumericLimits<double>::Max();
    for(const auto& Pair:Sectors)
    {const double Served=Traffic.SectorLastServed.FindRef(Pair.Key);if(Served<Oldest || (Served==Oldest && (Least.IsNone() || Pair.Key.LexicalLess(Least)))){Oldest=Served;Least=Pair.Key;}}
    if(!Least.IsNone() && Traffic.Journeys.Num()<MaxJourneys)ScheduleSector(Traffic,Economy,Sectors,Endpoints,Least);
}
