#include "TrafficAI.h"
#include "SimDataEvents.h"
#include "EconomyAI.h"
#include "EconomyRuntime.h"
#include "Algo/Reverse.h"
#include "Ship.h"
#include "Sim.h"
#include "Starshatter.h"
#include "OfflineCutsceneExport.h"

namespace
{
FName FreightEscrow(const FGalaxyTrafficJourney& Job){return FName(*(TEXT("NPC freight escrow/")+Job.Id.ToString(EGuidFormats::Digits)));}
bool RefundFreight(FGalaxyTrafficJourney& Job,FEconomyState& State)
{
    if(Job.TradeCredits>0 && !Job.bTradeSettled)
        if(!FEconomyLedger::Pay(State,FreightEscrow(Job),Job.Buyer,Job.TradeCredits,FGuid::NewGuid(),TEXT("Undelivered NPC freight refund")))return false;
    Job.TradeCredits=0;return true;
}
}
bool TrafficAI::PatrolAllowed(bool FreeFlight,bool Training,bool Networked,bool Cinematic)
{return FreeFlight && !Training && !Networked && !Cinematic;}
FGuid TrafficAI::SchedulePatrol(FGalaxyTrafficState& State,const FGalaxyTrafficSector& Sector,const TArray<FPatrolHull>& Hulls,int32 Maximum)
{
 if(Sector.bSurface || Hulls.IsEmpty() || Maximum<=0)return {};
 int32 Count=0;for(const auto& Job:State.Journeys)if(Job.bPatrol && !Job.IsFinished()){++Count;if(Job.CurrentSector==Sector.Id)return {};}
 if(Count>=Maximum || State.Journeys.Num()>=512)return {};
 FRandomStream Random(HashCombine(GetTypeHash(Sector.Id),uint32(++State.Sequence)));
 const int32 Roll=Random.RandRange(0,99);const int32 IFF=Roll<15?0:Roll<30?3:Sector.Affiliation;
 TArray<FPatrolHull> Eligible;for(const auto& Hull:Hulls)if(Hull.IFF==IFF || (IFF==0 && Hull.IFF!=3))Eligible.Add(Hull);
 if(Eligible.IsEmpty())return {};
 const auto& Hull=Eligible[Random.RandRange(0,Eligible.Num()-1)];
 static const TCHAR* Names[]={TEXT("Vega"),TEXT("Morrow"),TEXT("Hale"),TEXT("Orlov"),TEXT("Kade"),TEXT("Sen"),TEXT("Voss"),TEXT("Renn")};
 FGalaxyTrafficJourney Job;Job.Id=FGuid::NewGuid();Job.bPatrol=true;Job.ShipDesign=Hull.Design;Job.PatrolIFF=IFF;Job.PilotName=Names[Random.RandRange(0,7)];
 Job.CurrentSector=Sector.Id;Job.Route={Sector.Id};Job.Origin=FName(*(Sector.Id.ToString()+TEXT("/TransitA")));Job.Destination=FName(*(Sector.Id.ToString()+TEXT("/TransitB")));
 Job.Phase=EGalaxyTrafficPhase::Traveling;Job.Remaining=900;State.Journeys.Add(Job);return Job.Id;
}
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
        if(!Pickup || !Drop || !Pickup->bOperational || !Drop->bOperational || Pickup->Owner!=FEconomyRuntime::IndustryOwner() || Drop->Owner!=Pickup->Owner || Source==Destination ||
            !FMath::IsFinite(Quantity) || Quantity<=0 || !FMath::IsFinite(Mass) || Mass<=0 || Quantity*Mass>1000 ||
            Drop->CapacityTonnes-FEconomyLedger::Used(*Drop)<Quantity*Mass)return {};
        const auto* Stock=Pickup->Items.FindByPredicate([Item](const auto& Stack){return Stack.Type==Item;});
        const auto* Existing=Drop->Items.FindByPredicate([Item](const auto& Stack){return Stack.Type==Item;});
        if(!Stock || !FMath::IsNearlyEqual(Stock->UnitTonnes,Mass) || (Existing && !FMath::IsNearlyEqual(Existing->UnitTonnes,Mass)))return {};
        if(!Pickup->TradingOwner.IsNone() && !Drop->TradingOwner.IsNone() && Pickup->TradingOwner!=Drop->TradingOwner)
        {
            const int64 Ask=Pickup->AskPrices.FindRef(Item),Bid=Drop->BidPrices.FindRef(Item);const double Total=Quantity*Ask;
            if(Ask<=0 || Bid<Ask || !FMath::IsFinite(Total) || Total>9.e15)return {};
            Job.Buyer=Drop->TradingOwner;Job.Seller=Pickup->TradingOwner;Job.TradeCredits=FMath::CeilToInt64(Total);
            if(Economy.Wallets.FindRef(Job.Buyer)<Job.TradeCredits)return {};
        }
        const FName Custodian=Pickup->Owner;
        Job.Reservation=FEconomyLedger::Reserve(Economy,Custodian,Source,Item,Quantity);
        if(!Job.Reservation.IsValid())return {};
        if(!FEconomyLedger::CreateInventory(Economy,Job.Hold,Custodian,From.System,1000))
        {FEconomyLedger::Release(Economy,Custodian,Job.Reservation);return {};}
        if(Job.TradeCredits>0 && !FEconomyLedger::Pay(Economy,Job.Buyer,FreightEscrow(Job),Job.TradeCredits,FGuid::NewGuid(),TEXT("NPC freight escrow")))
        {FEconomyLedger::Release(Economy,Custodian,Job.Reservation);Economy.Inventories.Remove(Job.Hold);return {};}
        Job.Item=Item;Job.Quantity=Quantity;Job.UnitTonnes=Mass;Job.SourceInventory=Source;Job.DestinationInventory=Destination;
        Economy.Inventories.FindChecked(Destination).ReservedOutputTonnes+=Quantity*Mass;Job.bDestinationReserved=true;
    }
    Traffic.Journeys.Add(Job);Traffic.SectorLastServed.Add(From.Sector,Traffic.Clock);return Job.Id;
}
bool TrafficAI::Advance(FGalaxyTrafficJourney& Job,FEconomyState& Economy,double Seconds,bool Realized,bool Arrived,double HopSeconds,double Rate)
{
    if(Job.IsFinished() || !FMath::IsFinite(Seconds) || Seconds<=0 || !FMath::IsFinite(HopSeconds) || HopSeconds<=0 || !FMath::IsFinite(Rate) || Rate<=0)return false;
    Job.Age+=Seconds;
    if(Job.bPatrol)
    {
        Job.Remaining=FMath::Max(0.0,Job.Remaining-Seconds);
        if(Job.Remaining<=0){Job.Phase=EGalaxyTrafficPhase::Completed;return true;}
        return false;
    }
    const FName Owner=FEconomyRuntime::IndustryOwner();
    if(Job.IsCargo() && (Job.Phase==EGalaxyTrafficPhase::Traveling || Job.Phase==EGalaxyTrafficPhase::Unloading))
        if(const auto* Drop=Economy.Inventories.Find(Job.DestinationInventory);!Drop || !Drop->bOperational)
        {Job.Phase=EGalaxyTrafficPhase::Blocked;Job.BlockedReason=TEXT("Destination offline. Cargo retained pending reroute.");}
    if(Job.Phase==EGalaxyTrafficPhase::Blocked)
    {
        Job.BlockedSeconds+=Seconds;const auto* Drop=Economy.Inventories.Find(Job.DestinationInventory);
        const double Reserved=Job.bDestinationReserved?Job.Quantity*Job.UnitTonnes:0;
        if(FEconomyLedger::Quantity(Economy,Job.Hold,Job.Item)+UE_SMALL_NUMBER<Job.Quantity){Lose(Job,Economy);return false;}
        if(Drop && Drop->bOperational && Drop->CapacityTonnes-FEconomyLedger::Used(*Drop)+Reserved+UE_SMALL_NUMBER>=Job.Quantity*Job.UnitTonnes)
        {Job.Phase=EGalaxyTrafficPhase::Unloading;Job.Remaining=FMath::Max(1.0,Job.Quantity*Job.UnitTonnes/Rate);Job.BlockedSeconds=0;}
        return false;
    }
    if((Job.Phase==EGalaxyTrafficPhase::Loading || Job.Phase==EGalaxyTrafficPhase::Unloading) && Realized && !Arrived)return false;
    Job.Remaining=FMath::Max(0.0,Job.Remaining-Seconds);
    if(Job.Remaining>0)return false;
    switch(Job.Phase)
    {
    case EGalaxyTrafficPhase::Waiting:
        Job.Phase=Job.IsCargo()?EGalaxyTrafficPhase::Loading:EGalaxyTrafficPhase::Traveling;
        Job.Remaining=Job.IsCargo()?FMath::Max(1.0,Job.Quantity*Job.UnitTonnes/Rate):HopSeconds;break;
    case EGalaxyTrafficPhase::Loading:
        if(const auto* Pickup=Economy.Inventories.Find(Job.SourceInventory);!Pickup || !Pickup->bOperational){Job.BlockedReason=TEXT("Pickup offline; reservation cancelled.");Lose(Job,Economy);return false;}
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
        if(Job.TradeCredits>0 && !Job.bTradeSettled && (Economy.Wallets.FindRef(FreightEscrow(Job))<Job.TradeCredits || Economy.Wallets.FindRef(Job.Seller)>MAX_int64-Job.TradeCredits))
        {Job.Phase=EGalaxyTrafficPhase::Blocked;Job.BlockedReason=TEXT("NPC settlement unavailable; cargo remains aboard.");return false;}
        if(Job.bDestinationReserved)
        {
            if(auto* Drop=Economy.Inventories.Find(Job.DestinationInventory))Drop->ReservedOutputTonnes=FMath::Max(0.0,Drop->ReservedOutputTonnes-Job.Quantity*Job.UnitTonnes);
            Job.bDestinationReserved=false;
        }
        if(!FEconomyLedger::Transfer(Economy,Owner,Job.Hold,Job.DestinationInventory,Job.Item,Job.Quantity))
        {Job.Phase=EGalaxyTrafficPhase::Blocked;Job.BlockedReason=TEXT("Destination storage unavailable; cargo remains aboard.");return false;}
        if(Job.TradeCredits>0 && !Job.bTradeSettled)
        {
            if(!FEconomyLedger::Pay(Economy,FreightEscrow(Job),Job.Seller,Job.TradeCredits,FGuid::NewGuid(),TEXT("NPC freight delivery settlement"),ECommercePurpose::Purchase))
            {FEconomyLedger::Transfer(Economy,Owner,Job.DestinationInventory,Job.Hold,Job.Item,Job.Quantity);Job.Phase=EGalaxyTrafficPhase::Blocked;return false;}
            Job.bTradeSettled=true;
            if(const auto* Destination=Economy.Inventories.Find(Job.DestinationInventory))EconomyAI::RecordTrade(Economy,Destination->System,Job.Item,Job.Quantity,double(Job.TradeCredits));
        }
        Job.Phase=EGalaxyTrafficPhase::Completed;Economy.Inventories.Remove(Job.Hold);return true;
    default:break;
    }
    return Job.Phase==EGalaxyTrafficPhase::Completed;
}
bool TrafficAI::AbstractCargo(FGalaxyTrafficJourney& Job,FEconomyState& State,FName System)
{
    if(!Job.IsCargo() || Job.IsFinished())return true;
    const FName Hold(*(TEXT("Traffic/")+Job.Id.ToString(EGuidFormats::Digits)));
    if(Job.Hold==Hold)return State.Inventories.Contains(Hold);
    const auto* Source=State.Inventories.Find(Job.Hold);
    if(!Source || Source->Owner!=FEconomyRuntime::IndustryOwner() || Source->Items.ContainsByPredicate([&](const auto& S){return S.Type!=Job.Item && S.Quantity>0;}))return false;
    const double Amount=FEconomyLedger::Quantity(State,Job.Hold,Job.Item);
    auto* Target=FEconomyLedger::CreateInventory(State,Hold,FEconomyRuntime::IndustryOwner(),System,1000);
    if(!Target || Target->Owner!=FEconomyRuntime::IndustryOwner() || Target->System!=System)return false;
    if(Amount>0 && !FEconomyLedger::Transfer(State,FEconomyRuntime::IndustryOwner(),Job.Hold,Hold,Job.Item,Amount))return false;
    State.Inventories.Remove(Job.Hold);Job.Hold=Hold;return true;
}
void TrafficAI::Lose(FGalaxyTrafficJourney& Job,FEconomyState& Economy)
{
    if(Job.IsFinished())return;
    RefundFreight(Job,Economy);
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
        if(!Pair.Value.Station.IsNone() && Pair.Value.System==Inventory.System)
        {
            if(Text.StartsWith(Pair.Value.System.ToString()+TEXT("/")+Pair.Value.Station.ToString()+TEXT("/")))return &Pair.Value;
            if(Ship::IsTransferStationName(Pair.Value.Station.ToString()) && (!Transfer || Pair.Key.LexicalLess(Transfer->Id)))Transfer=&Pair.Value;
        }
    return (Text.Contains(TEXT("/NPC Market/")) || Text.StartsWith(TEXT("Fleet Repair/")))?Transfer:nullptr;
}
void TrafficAI::ScheduleSector(FGalaxyTrafficState& Traffic,FEconomyState& Economy,const TMap<FName,FGalaxyTrafficSector>& Sectors,const TMap<FName,FGalaxyTrafficEndpoint>& Endpoints,FName Sector)
{
    TArray<FName> Keys;Economy.Inventories.GetKeys(Keys);Keys.Sort([](FName A,FName B){return A.LexicalLess(B);});
    int32 Examined=0;
    for(FName Source:Keys)
    {
        const auto& Inventory=Economy.Inventories.FindChecked(Source);
        if(Inventory.Owner!=FEconomyRuntime::IndustryOwner() || Inventory.bHullStorage || !Inventory.bOperational || Inventory.TradingOwner.IsNone())continue;
        const auto* From=EndpointForInventory(Endpoints,Source,Inventory);if(!From || From->Sector!=Sector)continue;
        for(const auto& Item:Inventory.Items)
        {
            if(++Examined>256)break;
            const double Available=FMath::Max(0.0,FEconomyLedger::Available(Economy,Source,Item.Type)-Inventory.DesiredStock.FindRef(Item.Type));if(Available<=0)continue;
            for(FName Destination:Keys)
            {
                if(Destination==Source)continue;const auto& Drop=Economy.Inventories.FindChecked(Destination);
                if(Drop.Owner!=Inventory.Owner || Drop.bSupplyOnly || !Drop.bOperational)continue;
                const double Desired=Drop.DesiredStock.IsEmpty()?Available*.25:Drop.DesiredStock.FindRef(Item.Type);
                double Incoming=0;for(const auto& Journey:Traffic.Journeys)if(!Journey.IsFinished() && Journey.DestinationInventory==Destination && Journey.Item==Item.Type)Incoming+=Journey.Quantity;
                const double Deficit=Desired-FEconomyLedger::Quantity(Economy,Destination,Item.Type)-Incoming;
                if(Deficit<=UE_SMALL_NUMBER)continue;
                const auto* To=EndpointForInventory(Endpoints,Destination,Drop);if(!To || To->Id==From->Id)continue;
                const auto Path=TrafficAI::Route(Sectors,From->Sector,To->Sector);if(Path.IsEmpty())continue;
                double Count=FMath::Min3(FMath::Min(Available,Deficit),100.0/Item.UnitTonnes,FMath::Max(0.0,Drop.CapacityTonnes-FEconomyLedger::Used(Drop))/Item.UnitTonnes);
                if(Inventory.WholeUnitItems.Contains(Item.Type))Count=FMath::FloorToDouble(Count);
                // A declining market does not justify expensive speculative imports above its target.
                const double Trend=EconomyAI::PriceTrend(Economy,Drop.System,Item.Type);
                if(Trend<.9)Count=FMath::Min(Count,FMath::Max(0.0,Deficit*.5));
                if(Inventory.WholeUnitItems.Contains(Item.Type))Count=FMath::FloorToDouble(Count);
                if(Count>0 && TrafficAI::Schedule(Traffic,Economy,*From,*To,Path,Source,Destination,Item.Type,Count,Item.UnitTonnes).IsValid())return;
            }
        }
        if(Examined>256)break;
    }
    const FName FromId(*(Sector.ToString()+TEXT("/TransitA"))),ToId(*(Sector.ToString()+TEXT("/TransitB")));
    if(const auto* From=Endpoints.Find(FromId))if(const auto* To=Endpoints.Find(ToId))TrafficAI::Schedule(Traffic,Economy,*From,*To,{Sector});
}

int32 TrafficAI::RecoverBlocked(FGalaxyTrafficState& Traffic,FEconomyState& Economy,const TMap<FName,FGalaxyTrafficSector>& Sectors,const TMap<FName,FGalaxyTrafficEndpoint>& Endpoints)
{
    int32 Recovered=0;
    for(auto& Job:Traffic.Journeys)
    {
        if(Job.Phase!=EGalaxyTrafficPhase::Blocked || Job.BlockedSeconds<60 || Job.bReturning)continue;
        // Cargo already aboard is offered to another reachable business first.
        // Work on a candidate ledger so refund, replacement escrow and storage
        // reservation either all commit together or leave the original job intact.
        if(FEconomyLedger::Quantity(Economy,Job.Hold,Job.Item)+UE_SMALL_NUMBER<Job.Quantity)continue;
        TArray<FName> Keys;Economy.Inventories.GetKeys(Keys);Keys.Sort([](FName A,FName B){return A.LexicalLess(B);});
        FName Best;TArray<FName> BestPath;int64 BestBid=0;const FGalaxyTrafficEndpoint* BestEndpoint=nullptr;
        const double Mass=Job.Quantity*Job.UnitTonnes;
        for(FName Key:Keys)
        {
            if(Key==Job.SourceInventory || Key==Job.DestinationInventory || Key==Job.Hold)continue;
            const auto& Store=Economy.Inventories.FindChecked(Key);
            if(!Store.bOperational || Store.bSupplyOnly || Store.bHullStorage || Store.Owner!=FEconomyRuntime::IndustryOwner() || Store.TradingOwner.IsNone())continue;
            const double Need=Store.DesiredStock.FindRef(Job.Item)-FEconomyLedger::Quantity(Economy,Key,Job.Item);
            double Incoming=0;for(const auto& Other:Traffic.Journeys)if(Other.Id!=Job.Id && !Other.IsFinished() && Other.DestinationInventory==Key && Other.Item==Job.Item)Incoming+=Other.Quantity;
            const int64 Bid=Store.BidPrices.FindRef(Job.Item);
            if(Need-Incoming+UE_SMALL_NUMBER<Job.Quantity || Store.CapacityTonnes-FEconomyLedger::Used(Store)+UE_SMALL_NUMBER<Mass || Bid<=0 || double(Bid)*Job.Quantity+UE_SMALL_NUMBER<Job.TradeCredits)continue;
            const int64 Refund=Store.TradingOwner==Job.Buyer?Job.TradeCredits:0;
            if(double(Economy.Wallets.FindRef(Store.TradingOwner))+Refund<Job.TradeCredits)continue;
            const auto* Point=EndpointForInventory(Endpoints,Key,Store);if(!Point || Point->bSurface)continue;
            auto Path=Route(Sectors,Job.CurrentSector,Point->Sector);if(Path.IsEmpty())continue;
            if(Best.IsNone() || Bid>BestBid || (Bid==BestBid && Path.Num()<BestPath.Num())){Best=Key;BestBid=Bid;BestPath=MoveTemp(Path);BestEndpoint=Point;}
        }
        if(!Best.IsNone() && BestEndpoint)
        {
            FEconomyState Candidate=Economy;auto Replacement=Job;const int64 Price=Job.TradeCredits;
            if(RefundFreight(Replacement,Candidate))
            {
                Replacement.Buyer=Candidate.Inventories.FindChecked(Best).TradingOwner;Replacement.TradeCredits=Price;Replacement.bTradeSettled=false;
                if(Price==0 || FEconomyLedger::Pay(Candidate,Replacement.Buyer,FreightEscrow(Replacement),Price,FGuid::NewGuid(),TEXT("Rerouted NPC freight escrow")))
                {
                    if(Replacement.bDestinationReserved)if(auto* Old=Candidate.Inventories.Find(Replacement.DestinationInventory))Old->ReservedOutputTonnes=FMath::Max(0.0,Old->ReservedOutputTonnes-Mass);
                    Candidate.Inventories.FindChecked(Best).ReservedOutputTonnes+=Mass;
                    Replacement.Destination=BestEndpoint->Id;Replacement.DestinationInventory=Best;Replacement.Route=BestPath;Replacement.RouteIndex=0;Replacement.Remaining=90;
                    Replacement.Phase=EGalaxyTrafficPhase::Traveling;Replacement.BlockedSeconds=0;Replacement.bDestinationReserved=true;Replacement.BlockedReason=TEXT("Replacement buyer reserved; delivering retained cargo.");
                    Economy=MoveTemp(Candidate);Job=MoveTemp(Replacement);++Recovered;continue;
                }
            }
        }
        const auto* Origin=Endpoints.Find(Job.Origin);auto* Source=Economy.Inventories.Find(Job.SourceInventory);
        if(!Origin || !Source || !Source->bOperational || Source->CapacityTonnes-FEconomyLedger::Used(*Source)+UE_SMALL_NUMBER<Job.Quantity*Job.UnitTonnes)continue;
        const auto Path=Route(Sectors,Job.CurrentSector,Origin->Sector);if(Path.IsEmpty()){Job.BlockedReason=TEXT("No return route. Cargo retained until a route is restored.");continue;}
        if(!RefundFreight(Job,Economy))continue;
        if(Job.bDestinationReserved)if(auto* Old=Economy.Inventories.Find(Job.DestinationInventory))Old->ReservedOutputTonnes=FMath::Max(0.0,Old->ReservedOutputTonnes-Job.Quantity*Job.UnitTonnes);
        Job.bDestinationReserved=true;Source->ReservedOutputTonnes+=Job.Quantity*Job.UnitTonnes;
        Job.Destination=Origin->Id;Job.DestinationInventory=Job.SourceInventory;Job.Route=Path;Job.RouteIndex=0;Job.Remaining=90;Job.Phase=EGalaxyTrafficPhase::Traveling;Job.bReturning=true;Job.BlockedSeconds=0;
        Job.BlockedReason=TEXT("Destination unavailable; returning cargo to origin.");++Recovered;
    }
    return Recovered;
}
void TrafficAI::Dispatch(FGalaxyTrafficState& Traffic,FEconomyState& Economy,const TMap<FName,FGalaxyTrafficSector>& Sectors,const TMap<FName,FGalaxyTrafficEndpoint>& Endpoints,FName Active,int32 MaxJourneys)
{
    if(Traffic.Journeys.Num()>=MaxJourneys)return;
    int32 Near=0;for(const auto& Job:Traffic.Journeys)if(!Job.IsFinished() && Job.CurrentSector==Active)++Near;
    if(Near<2 && Sectors.Contains(Active))
    {
        ScheduleSector(Traffic,Economy,Sectors,Endpoints,Active);
        Traffic.SectorLastServed.Add(Active,FMath::Max(UE_SMALL_NUMBER,Traffic.Clock));
    }
    FName Least;double Oldest=TNumericLimits<double>::Max();
    for(const auto& Pair:Sectors)
    {const double Served=Traffic.SectorLastServed.FindRef(Pair.Key);if(Served<Oldest || (Served==Oldest && (Least.IsNone() || Pair.Key.LexicalLess(Least)))){Oldest=Served;Least=Pair.Key;}}
    if(!Least.IsNone() && Traffic.Journeys.Num()<MaxJourneys)
    {
        ScheduleSector(Traffic,Economy,Sectors,Endpoints,Least);
        // Service means examined, even when no trade can start. Otherwise an
        // empty lexical-first sector starves every unloaded producer forever.
        Traffic.SectorLastServed.Add(Least,FMath::Max(UE_SMALL_NUMBER,Traffic.Clock));
    }
}

bool TrafficAI::AdvanceScheduled(FGalaxyTrafficJourney& Job,FEconomyState& Economy,double Now,bool Realized,bool Arrived,bool InventoryChanged,double HopSeconds,double Rate)
{
    if(Job.IsFinished() || !FMath::IsFinite(Now) || Now<0)return false;
    if(Job.Phase!=Job.ScheduledPhase || Job.RouteIndex!=Job.ScheduledRouteIndex)
        Job.TransitionAt=0;
    if(InventoryChanged && Job.IsCargo())
    {
        // A storage or facility change can interrupt a route immediately; no repeated retry timer.
        if(const auto* Drop=Economy.Inventories.Find(Job.DestinationInventory);!Drop || !Drop->bOperational)
        {if(Job.Phase==EGalaxyTrafficPhase::Traveling || Job.Phase==EGalaxyTrafficPhase::Unloading){Job.Phase=EGalaxyTrafficPhase::Blocked;Job.BlockedReason=TEXT("Destination offline. Awaiting recovery event.");Job.TransitionAt=0;}}
    }
    if(Job.Phase==EGalaxyTrafficPhase::Blocked)
    {
        if(InventoryChanged)Advance(Job,Economy,UE_SMALL_NUMBER,Realized,Arrived,HopSeconds,Rate);
        if(Job.Phase==EGalaxyTrafficPhase::Blocked && Job.TransitionAt<=0 && Job.BlockedSeconds<60){Job.LastTransitionAt=Now;Job.TransitionAt=Now+60-Job.BlockedSeconds;}
        Job.ScheduledPhase=Job.Phase;Job.ScheduledRouteIndex=Job.RouteIndex;return false;
    }
    if(Realized && !Arrived && (Job.Phase==EGalaxyTrafficPhase::Loading || Job.Phase==EGalaxyTrafficPhase::Unloading))
    {Job.TransitionAt=0;Job.LastTransitionAt=Now;return false;}
    if(Job.TransitionAt<=0)
    {
        Job.LastTransitionAt=Now;Job.TransitionAt=Now+FMath::Max(UE_SMALL_NUMBER,Job.Remaining);
        Job.ScheduledPhase=Job.Phase;Job.ScheduledRouteIndex=Job.RouteIndex;
    }
    if(Now<Job.TransitionAt)return false;
    // Realized arrivals are continuous gameplay events, never a timer teleport.
    if(Realized && Job.Phase==EGalaxyTrafficPhase::Traveling && !Arrived)return false;
    const double Boundary=Job.TransitionAt;const double Elapsed=FMath::Max(UE_SMALL_NUMBER,Boundary-Job.LastTransitionAt);
    const auto Before=Job.Phase;const int32 Leg=Job.RouteIndex;
    const bool Complete=Advance(Job,Economy,Elapsed,Realized,Arrived,HopSeconds,Rate);
    Job.LastTransitionAt=Now;Job.TransitionAt=Job.IsFinished() || Job.Phase==EGalaxyTrafficPhase::Blocked?0:Now+FMath::Max(UE_SMALL_NUMBER,Job.Remaining);
    Job.ScheduledPhase=Job.Phase;Job.ScheduledRouteIndex=Job.RouteIndex;
    if(Before!=Job.Phase || Leg!=Job.RouteIndex)FSimDataEvents::Publish(Economy);
    return Complete;
}
