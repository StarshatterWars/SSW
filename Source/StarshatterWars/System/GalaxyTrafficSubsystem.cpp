#include "GalaxyTrafficSubsystem.h"
#include "StarshatterPlayerSubsystem.h"
#include "SSWRuntimeSubsystem.h"
#include "OfflineCutsceneExport.h"
#include "EconomyRuntime.h"
#include "Sim.h"
#include "Ship.h"
#include "ShipDesign.h"
#include "ShipDesignRegistry.h"
#include "SimRegion.h"
#include "StarSystem.h"
#include "OrbitalRegion.h"
#include "CombatGroupRegistry.h"
#include "Asteroid.h"
#include "Instruction.h"
#include "WaypointArrival.h"
#include "TransportAI.h"
#include "Starshatter.h"
#include "Mission.h"
#include "Game.h"
#include "Engine/World.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"

namespace
{
FName NodeKey(FName System,FName Region){return FName(*(System.ToString()+TEXT("/")+Region.ToString()));}
bool Observed(Sim& Simulation,const Ship& Craft)
{
    if(Craft.GetRegion()==Simulation.GetActiveRegion())return true;
    if(auto* Region=Craft.GetRegion())for(ListIter<Ship> Ships=Region->GetShips();++Ships;)
        if(Ships->HasNetworkPilot() || Ships.value()==Simulation.GetPlayerShip())return true;
    return false;
}
Ship* StationAt(Sim& Simulation,const FGalaxyTrafficEndpoint& Endpoint)
{
    if(Endpoint.Station.IsNone())return nullptr;
    auto* Station=Simulation.FindShip(TCHAR_TO_UTF8(*Endpoint.Station.ToString()),TCHAR_TO_UTF8(*Endpoint.Region.ToString()));
    return Station && !Station->IsDead() && !Station->IsDying()?Station:nullptr;
}
}
void UGalaxyTrafficSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
    Super::Initialize(Collection);FString Json;TSharedPtr<FJsonObject> Config;
    if(FFileHelper::LoadFileToString(Json,*(FPaths::ProjectContentDir()/TEXT("GameData/Traffic/traffic.json"))) && FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(Json),Config) && Config)
    {
        int32 Number=0;double Seconds=0;
        if(Config->TryGetNumberField(TEXT("ActiveShipBudget"),Number))ActiveBudget=FMath::Clamp(Number,0,8);
        if(Config->TryGetNumberField(TEXT("MaximumJourneys"),Number))MaxJourneys=FMath::Clamp(Number,8,256);
        if(Config->TryGetNumberField(TEXT("ScheduleSeconds"),Seconds) && FMath::IsFinite(Seconds))ScheduleSeconds=FMath::Clamp(Seconds,2.0,120.0);
        if(Config->TryGetNumberField(TEXT("AbstractHopSeconds"),Seconds) && FMath::IsFinite(Seconds))HopSeconds=FMath::Clamp(Seconds,10.0,1800.0);
        if(Config->TryGetNumberField(TEXT("TransferTonnesPerSecond"),Seconds) && FMath::IsFinite(Seconds))TransferTonnesPerSecond=FMath::Clamp(Seconds,.01,100.0);
    }
}
void UGalaxyTrafficSubsystem::Deinitialize()
{
    if(auto* Simulation=Sim::GetSim()){TArray<FGuid> Ids;LiveShips.GetKeys(Ids);for(FGuid Id:Ids)Despawn(*Simulation,Id);}
    LiveShips.Reset();LiveGoals.Reset();Sectors.Reset();Endpoints.Reset();Super::Deinitialize();
}
FName UGalaxyTrafficSubsystem::SectorKey(SimRegion* Region)const
{return Region && Region->GetSystem()?NodeKey(FName(UTF8_TO_TCHAR(Region->GetSystem()->GetName())),FName(UTF8_TO_TCHAR(Region->GetName()))):NAME_None;}
void UGalaxyTrafficSubsystem::BuildGraph(Sim& Simulation)
{
    Sectors.Reset();Endpoints.Reset();TMap<FName,StarSystem*> Systems;
    for(ListIter<StarSystem> Iter=Simulation.GetSystemList();++Iter;)
    {
        auto* System=Iter.value();if(!System)continue;const FName SystemId(UTF8_TO_TCHAR(System->GetName()));Systems.Add(SystemId,System);
        for(ListIter<OrbitalRegion> Regions=System->GetAllRegions();++Regions;)
        {
            auto* Region=Regions.value();if(!Region)continue;
            FGalaxyTrafficSector Node;Node.System=SystemId;Node.Region=FName(UTF8_TO_TCHAR(Region->GetName()));Node.Id=NodeKey(Node.System,Node.Region);Node.bSurface=Region->GetType()==Orbital::TERRAIN;Sectors.Add(Node.Id,Node);
        }
    }
    // Include instantiated regions too: authored surface/mission sectors may not
    // appear in the all-orbits table. They remain abstract if atmospheric.
    for(ListIter<SimRegion> Iter=Simulation.GetRegions();++Iter;)
        if(auto* Region=Iter.value();Region && Region->GetSystem())
        {FGalaxyTrafficSector Node;Node.Id=SectorKey(Region);Node.System=FName(UTF8_TO_TCHAR(Region->GetSystem()->GetName()));Node.Region=FName(UTF8_TO_TCHAR(Region->GetName()));Node.bSurface=Region->IsAirSpace();Sectors.FindOrAdd(Node.Id)=Node;}
    for(const auto& Pair:CombatGroupRegistry::GetAll())
    {
        const auto& Group=Pair.Value;if(Group.System.IsEmpty() || Group.Region.IsEmpty() || !Systems.Contains(FName(*Group.System)))continue;
        const FName Id=NodeKey(FName(*Group.System),FName(*Group.Region));
        if(!Sectors.Contains(Id))
        {FGalaxyTrafficSector Node;Node.Id=Id;Node.System=FName(*Group.System);Node.Region=FName(*Group.Region);
            for(const auto& Unit:Group.Unit)Node.bSurface|=Unit.UnitDesign.Contains(TEXT("Airbase"),ESearchCase::IgnoreCase);
            Sectors.Add(Id,Node);}
    }
    for(auto& Pair:Sectors)
    {
        for(const auto& Other:Sectors)
            if(Pair.Key!=Other.Key && (Pair.Value.System==Other.Value.System ||
                (Systems.FindRef(Pair.Value.System) && Systems.FindRef(Other.Value.System) && Systems.FindRef(Pair.Value.System)->HasLinkTo(Systems.FindRef(Other.Value.System)))))Pair.Value.Neighbors.AddUnique(Other.Key);
        for(int32 Side=0;Side<2;++Side)
        {
            FGalaxyTrafficEndpoint Endpoint;Endpoint.Sector=Pair.Key;Endpoint.System=Pair.Value.System;Endpoint.Region=Pair.Value.Region;Endpoint.bSurface=Pair.Value.bSurface;
            Endpoint.Id=FName(*(Pair.Key.ToString()+(Side?TEXT("/TransitB"):TEXT("/TransitA"))));Endpoint.Location=FVector(Side?28000:-28000,8000,Side?9000:-9000);
            Endpoints.Add(Endpoint.Id,Endpoint);
        }
    }
    for(const auto& Pair:CombatGroupRegistry::GetAll())
    {
        const auto& Group=Pair.Value;const FName Sector=NodeKey(FName(*Group.System),FName(*Group.Region));const auto* Node=Sectors.Find(Sector);if(!Node)continue;
        bool Station=Ship::IsTransferStationName(Group.Name);
        for(const auto& Unit:Group.Unit)
            Station|=Unit.UnitDesign.Contains(TEXT("Station"),ESearchCase::IgnoreCase) || Unit.UnitDesign.Contains(TEXT("Airbase"),ESearchCase::IgnoreCase) || Unit.UnitDesign.Contains(TEXT("Processing"),ESearchCase::IgnoreCase);
        if(!Station)continue;
        FGalaxyTrafficEndpoint Endpoint;Endpoint.Id=FName(*(Sector.ToString()+TEXT("/")+Group.Name));Endpoint.Sector=Sector;Endpoint.System=Node->System;Endpoint.Region=Node->Region;
        Endpoint.Station=FName(*Group.Name);Endpoint.Location=OtherHand(Group.Location);Endpoint.bSurface=Node->bSurface;Endpoint.bPhysical=true;Endpoints.Add(Endpoint.Id,Endpoint);
    }
    // Discover live stations even if authored only in this mission's element list.
    for(ListIter<SimRegion> Regions=Simulation.GetRegions();++Regions;)
        if(auto* Region=Regions.value())for(ListIter<Ship> Ships=Region->GetShips();++Ships;)
        {
            auto* Station=Ships.value();if(!Station || !Station->IsStatic() || Station->GetFarcaster())continue;
            const FName Sector=SectorKey(Region);const auto* Node=Sectors.Find(Sector);if(!Node)continue;
            FGalaxyTrafficEndpoint Endpoint;Endpoint.Sector=Sector;Endpoint.System=Node->System;Endpoint.Region=Node->Region;Endpoint.Station=FName(UTF8_TO_TCHAR(Station->GetName()));
            Endpoint.Id=FName(*(Sector.ToString()+TEXT("/")+Endpoint.Station.ToString()));Endpoint.Location=Station->GetLocation();Endpoint.bSurface=Node->bSurface;Endpoint.bPhysical=true;Endpoints.Add(Endpoint.Id,Endpoint);
        }
    UE_LOG(LogTemp,Display,TEXT("[GalaxyTraffic] Coverage: %d systems, %d sectors, %d endpoints; active ship budget %d. Surface traffic remains abstract."),Systems.Num(),Sectors.Num(),Endpoints.Num(),ActiveBudget);
}
FVector UGalaxyTrafficSubsystem::Goal(Sim& Simulation,const FGalaxyTrafficEndpoint& Endpoint,Ship& Craft)const
{
    if(!Endpoint.Station.IsNone())
        if(auto* Station=StationAt(Simulation,Endpoint))
        {FVector Berth;if(Station->GetTransferBerth(0,Craft.GetNavigationHullRadius(),Berth))return Berth;}
    return Endpoint.Location;
}
void UGalaxyTrafficSubsystem::Despawn(Sim& Simulation,FGuid Id)
{
    if(const auto* Name=LiveShips.Find(Id))if(auto* Ship=Simulation.FindShip(TCHAR_TO_UTF8(**Name)))Simulation.DestroyShip(Ship);
    LiveShips.Remove(Id);LiveGoals.Remove(Id);
}
void UGalaxyTrafficSubsystem::Realize(Sim& Simulation,FGalaxyTrafficJourney& Job,FEconomyState& Economy)
{
    if(LiveShips.Num()>=ActiveBudget || LiveShips.Contains(Job.Id) || Job.IsFinished())return;
    const auto* Node=Sectors.Find(Job.CurrentSector);if(!Node || Node->bSurface)return;
    auto* Region=Simulation.FindRegion(Node->Region.ToString());if(!Region || Region->IsAirSpace() || SectorKey(Region)!=Node->Id)return;
    bool Visible=Region==Simulation.GetActiveRegion();
    for(ListIter<Ship> Ships=Region->GetShips();++Ships;)Visible|=Ships->HasNetworkPilot();
    if(!Visible)return;
    ShipDesign* Design=ShipDesignRegistry::FindLegacy(FName(Job.IsCargo()?TEXT("IM3500"):TEXT("Courier")));if(!Design)return;
    const auto* From=Endpoints.Find(Job.Origin);if(!From)return;
    FVector Center=Job.CurrentSector==From->Sector?From->Location:FVector(30000,8000,0);
    TArray<TPair<FVector,double>> Obstacles;
    for(ListIter<Ship> Ships=Region->GetShips();++Ships;)if(Ships.value())Obstacles.Emplace(Ships->GetLocation(),Ships->GetNavigationHullRadius());
    for(ListIter<Asteroid> Rocks=Region->GetRoids();++Rocks;)if(Rocks.value())Obstacles.Emplace(Rocks->GetLocation(),Rocks->GetRadius());
    FVector Point;bool Safe=false;
    for(int32 Attempt=0;Attempt<12;++Attempt)
    {
        const double Angle=(Attempt+Job.Id.A%12)*PI/6;
        Point=Center+FVector(FMath::Cos(Angle)*18000,5000,FMath::Sin(Angle)*18000);
        if(TrafficAI::SafeAnchor(Point,FMath::Max(1000.0,double(Design->radius)),Obstacles) &&
            (!Simulation.GetPlayerShip() || FVector::DistSquared(Point,Simulation.GetPlayerShip()->GetLocation())>FMath::Square(12000.0))) {Safe=true;break;}
    }
    if(!Safe)return;
    const FString Name=TEXT("Traffic ")+Job.Id.ToString(EGuidFormats::Digits).Left(12);
    auto* Craft=Simulation.CreateShip(TCHAR_TO_UTF8(*Name),"",Design,TCHAR_TO_UTF8(*Node->Region.ToString()),OtherHand(Point),0,0);
    if(!Craft)return;
    // Constructor defaults may describe random civilian freight; ambient jobs
    // start empty and only load stock reserved by TrafficAI.
    SSWFreightManifest::FLoad Empty;Craft->SetSimulatedCargo(Empty);
    if(auto* Hold=Economy.Inventories.Find(Craft->GetCargoInventoryKey()))
    {Hold->Items.Reset();Hold->ManifestItems.Reset();Hold->ReservedOutputTonnes=0;}
    if(Job.IsCargo())
    {
        SSWFreightManifest::FLoad Load;Load.Item=Job.Item.ToString();Load.Unit=Job.UnitTonnes==1?TEXT("t"):TEXT("units");
        if(From)Load.Origin=SSWFreightManifest::FEndpoint{From->System.ToString(),From->Region.ToString(),From->Station.ToString(),From->Location};
        if(const auto* To=Endpoints.Find(Job.Destination))Load.Destination=SSWFreightManifest::FEndpoint{To->System.ToString(),To->Region.ToString(),To->Station.ToString(),To->Location};
        Craft->SetSimulatedCargo(Load);const FName NativeHold=Craft->GetCargoInventoryKey();
        if(Job.Hold!=NativeHold)
        {
            const double Aboard=FEconomyLedger::Quantity(Economy,Job.Hold,Job.Item);
            if(Aboard>0 && !FEconomyLedger::Transfer(Economy,FEconomyRuntime::IndustryOwner(),Job.Hold,NativeHold,Job.Item,Aboard)) {Economy.Inventories.Remove(NativeHold);Simulation.DestroyShip(Craft);return;}
            Economy.Inventories.Remove(Job.Hold);Job.Hold=NativeHold;Craft->ApplyCargoInventoryToManifest();
        }
    }
    LiveShips.Add(Job.Id,Name);UE_LOG(LogTemp,Display,TEXT("[GalaxyTraffic] Realized %s in %s (%s)."),*Name,*Job.CurrentSector.ToString(),Job.IsCargo()?TEXT("reserved cargo"):TEXT("local transit"));
}
void UGalaxyTrafficSubsystem::UpdateLive(Sim& Simulation,FGalaxyTrafficState& Traffic,FEconomyState& Economy,double Seconds)
{
    for(auto& Job:Traffic.Journeys)
    {
        const auto* Name=LiveShips.Find(Job.Id);if(!Name)continue;
        auto* Craft=Simulation.FindShip(TCHAR_TO_UTF8(**Name));
        if(!Craft || Craft->IsDead() || Craft->IsDying()) {TrafficAI::Lose(Job,Economy);LiveShips.Remove(Job.Id);LiveGoals.Remove(Job.Id);continue;}
        Job.CurrentSector=SectorKey(Craft->GetRegion());const int32 Actual=Job.Route.IndexOfByKey(Job.CurrentSector);if(Actual!=INDEX_NONE)Job.RouteIndex=Actual;
        if(Job.IsCargo())Craft->ApplyCargoInventoryToManifest();
        if(!Observed(Simulation,*Craft)){Despawn(Simulation,Job.Id);continue;}
        const bool Pickup=Job.Phase==EGalaxyTrafficPhase::Waiting || Job.Phase==EGalaxyTrafficPhase::Loading;
        const auto* Endpoint=Endpoints.Find(Pickup?Job.Origin:Job.Destination);if(!Endpoint){Craft->SetAutoNav(false);Craft->StopScriptedMotion();continue;}
        if(Endpoint->bPhysical && !StationAt(Simulation,*Endpoint)){Craft->SetAutoNav(false);Craft->StopScriptedMotion();continue;}
        auto* Region=Simulation.FindRegion(Endpoint->Region.ToString());
        if(!Region || SectorKey(Region)!=Endpoint->Sector){Craft->SetAutoNav(false);Craft->StopScriptedMotion();continue;}
        if(Craft->GetRegion()!=Region)
        {Craft->SetAutoNav(false);TransportAI::FlyToRegion(*Craft,*Region,Seconds);continue;}
        const FVector Destination=Goal(Simulation,*Endpoint,*Craft);
        const bool Arrived=FVector::DistSquared(Craft->GetLocation(),Destination)<FMath::Square(FMath::Max(800.0,LegacyWaypointArrival::Radius(Craft->GetNavigationHullRadius())));
        if(Arrived){Craft->SetAutoNav(false);Craft->StopScriptedMotion();}
        else if(!LiveGoals.Contains(Job.Id) || !LiveGoals.FindChecked(Job.Id).Equals(Destination,100))
        {
            Craft->SetAutoNav(false);Craft->ClearFlightPlan();auto* Waypoint=new Instruction(Region,OtherHand(Destination));Waypoint->SetSpeed(250);Waypoint->SetWeaponsFree(0);Craft->AddNavPoint(Waypoint);Craft->SetAutoNav(true);LiveGoals.Add(Job.Id,Destination);
        }
    }
}
void UGalaxyTrafficSubsystem::Tick(float DeltaSeconds)
{
    auto* Simulation=Sim::GetSim();auto* Runtime=GetGameInstance()->GetSubsystem<USSWRuntimeSubsystem>();auto* Players=GetGameInstance()->GetSubsystem<UStarshatterPlayerSubsystem>();
    const auto* Director=Starshatter::GetInstance();
    const bool Playing=Simulation && Simulation->GetMission() && Runtime && Runtime->GetGameMode()==EGameMode::PLAY;
    if(!TrafficAI::Eligible(Simulation && Simulation->HasSimulationAuthority(),Playing,Director && Director->InCutscene(),OfflineCutsceneExport::IsRequested()) || !Players || !GetWorld() || GetWorld()->IsPaused())return;
    if(bPersistenceBlocked)
    {
        for(const auto& Pair:LiveShips)if(auto* Craft=Simulation->FindShip(TCHAR_TO_UTF8(*Pair.Value)))
        {Craft->SetAutoNav(false);Craft->StopScriptedMotion();}
        PersistenceRetry+=DeltaSeconds;if(PersistenceRetry<2)return;PersistenceRetry=0;
        if(!Players->SaveActivePlayerProgress())return;
        bPersistenceBlocked=false;LiveGoals.Reset();CheckpointElapsed=0;
    }
    if(auto* Player=Simulation->GetPlayerShip();Player && Player->Design() && FCStringAnsi::Stricmp(Player->Design()->name,"CameraPod")==0)return;
    if(!Simulation->GetPlayerShip() && !Simulation->IsNetworked())return;
    if(LastMission!=Simulation->GetMission())
    {LiveShips.Reset();LiveGoals.Reset();LastMission=Simulation->GetMission();BuildGraph(*Simulation);}
    if(Sectors.IsEmpty()){BuildGraph(*Simulation);return;}
    const double Seconds=FMath::Max(0.0,double(DeltaSeconds))*FMath::Max(1u,Runtime->GetTimeCompression());
    auto& Pilot=Players->GetMutablePlayerInfo();auto& Traffic=Pilot.Traffic;auto& Economy=Simulation->GetEconomy();
    UpdateLive(*Simulation,Traffic,Economy,Seconds);Accumulator+=Seconds;ScheduleElapsed+=Seconds;if(Accumulator<1)return;
    const double Step=FMath::Min(Accumulator,5.0);Accumulator=0;Traffic.Clock+=Step;CheckpointElapsed+=Step;bool Changed=false;
    for(auto& Job:Traffic.Journeys)
    {
        if(Job.IsFinished()){Job.Age+=Step;continue;}
        bool Realized=LiveShips.Contains(Job.Id),Arrived=false;
        if(Realized)
        {
            auto* Craft=Simulation->FindShip(TCHAR_TO_UTF8(*LiveShips.FindChecked(Job.Id)));
            const auto* Endpoint=Endpoints.Find(Job.Phase==EGalaxyTrafficPhase::Waiting || Job.Phase==EGalaxyTrafficPhase::Loading?Job.Origin:Job.Destination);
            if(Craft && Endpoint && (!Endpoint->bPhysical || StationAt(*Simulation,*Endpoint)) && SectorKey(Craft->GetRegion())==Endpoint->Sector)
            {const FVector Destination=Goal(*Simulation,*Endpoint,*Craft);Arrived=FVector::DistSquared(Craft->GetLocation(),Destination)<FMath::Square(FMath::Max(800.0,LegacyWaypointArrival::Radius(Craft->GetNavigationHullRadius())));}
        }
        const auto PreviousPhase=Job.Phase;const int32 PreviousLeg=Job.RouteIndex;const FName PreviousHold=Job.Hold;
        TrafficAI::Advance(Job,Economy,Step,Realized,Arrived,HopSeconds,TransferTonnesPerSecond);
        if(Realized && Job.IsFinished())Despawn(*Simulation,Job.Id);
        else if(!Realized && !Job.IsFinished())Realize(*Simulation,Job,Economy);
        Changed|=PreviousPhase!=Job.Phase || PreviousLeg!=Job.RouteIndex || PreviousHold!=Job.Hold;
    }
    Traffic.Journeys.RemoveAll([](const auto& Job){return Job.IsFinished() && Job.Age>120;});
    if(ScheduleElapsed>=ScheduleSeconds && Traffic.Journeys.Num()<MaxJourneys)
    {
        ScheduleElapsed=0;const int32 Count=Traffic.Journeys.Num();
        TrafficAI::Dispatch(Traffic,Economy,Sectors,Endpoints,SectorKey(Simulation->GetActiveRegion()),MaxJourneys);
        Changed|=Count!=Traffic.Journeys.Num();
    }
    // Keep cargo and journey position in the same existing atomic player save.
    // A failed checkpoint suspends further traffic decisions instead of replaying
    // an already completed delivery. Dedicated world persistence is a separate service.
    if(!Simulation->IsNetworked() && (Changed || CheckpointElapsed>=30))
    {
        if(!Players->SaveActivePlayerProgress())
        {bPersistenceBlocked=true;PersistenceRetry=0;LiveGoals.Reset();UE_LOG(LogTemp,Error,TEXT("[GalaxyTraffic] Checkpoint failed; traffic suspended pending atomic save retry."));}
        else CheckpointElapsed=0;
    }
}
