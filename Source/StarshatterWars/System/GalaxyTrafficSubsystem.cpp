#include "GalaxyTrafficSubsystem.h"
#include "ProductionCatalog.h"
#include "DynamicMissionAI.h"
#include "AgentAI.h"
#include "PlayerCharacter.h"
#include "SSWGameInstance.h"
#include "CargoCatalogSubsystem.h"
#include "JsonObjectConverter.h"
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
#include "Weapon.h"
#include "WeaponGroup.h"
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
bool UGalaxyTrafficSubsystem::LoadEconomyCatalogs()
{
    TMap<FName,FSSWProductionFacility> NewFacilities;TMap<FName,FSSWProductionRecipe> NewRecipes;TArray<FBackgroundMine> NewMines;
    const auto Load=[](const TCHAR* File,TArray<TSharedPtr<FJsonValue>>& Rows)
    {FString Text;return FFileHelper::LoadFileToString(Text,*(FPaths::ProjectContentDir()/File)) && FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(Text),Rows);};
    FString CatalogError;if(!SSWProductionCatalog::Reload(CatalogError)){UE_LOG(LogTemp,Error,TEXT("%s"),*CatalogError);return false;}
    NewFacilities=SSWProductionCatalog::Facilities();NewRecipes=SSWProductionCatalog::Recipes();TArray<TSharedPtr<FJsonValue>> Rows;
    Rows.Reset();if(!Load(TEXT("GameData/Cargo/background_mining.json"),Rows))return false;
    for(const auto& Value:Rows){const auto O=Value->AsObject();if(!O)return false;FBackgroundMine Mine;FString Id,System,Region,Item;
        if(!O->TryGetStringField(TEXT("Id"),Id) || !O->TryGetStringField(TEXT("System"),System) || !O->TryGetStringField(TEXT("Region"),Region) || !O->TryGetStringField(TEXT("Item"),Item) ||
            !O->TryGetNumberField(TEXT("ReserveTonnes"),Mine.Reserve) || !O->TryGetNumberField(TEXT("RateTonnesPerSecond"),Mine.Rate) || !O->TryGetNumberField(TEXT("StorageTonnes"),Mine.Capacity))return false;
        if(Id.IsEmpty() || System.IsEmpty() || Region.IsEmpty() || Item.IsEmpty() || !FMath::IsFinite(Mine.Reserve) || Mine.Reserve<=0 || !FMath::IsFinite(Mine.Rate) || Mine.Rate<=0 || !FMath::IsFinite(Mine.Capacity) || Mine.Capacity<=0)return false;
        Mine.Id=FName(*Id);Mine.System=FName(*System);Mine.Region=FName(*Region);Mine.Item=DataTableUtils::MakeValidName(Item);NewMines.Add(Mine);}
    for(const auto& Pair:NewFacilities)if(!NewRecipes.Contains(Pair.Value.Recipe))return false;
    Facilities=MoveTemp(NewFacilities);Recipes=MoveTemp(NewRecipes);Mines=MoveTemp(NewMines);
    UE_LOG(LogTemp,Display,TEXT("[Economy] Native catalogs: %d facilities, %d recipes, %d background mining crews. No level actors required."),Facilities.Num(),Recipes.Num(),Mines.Num());return true;
}
void UGalaxyTrafficSubsystem::TickEconomy(FEconomyState& State,double Seconds,int64 Now)
{
    if(!bCatalogsLoaded || !FMath::IsFinite(Seconds) || Seconds<=0)return;
    EconomyAccumulator+=Seconds;if(EconomyAccumulator<1)return;
    const double Step=EconomyAccumulator;EconomyAccumulator=0;State.EconomyClockSeconds+=Step;
    for(const auto& Mine:Mines)EconomyAI::AdvanceMine(State,Mine.Id,FEconomyRuntime::Warehouse(Mine.System.ToString(),Mine.Region.ToString(),TEXT("Ore")),Mine.System,Mine.Item,Mine.Reserve,Mine.Rate,Mine.Capacity,Step);
    EconomyAI::AdvanceFacilities(State,Facilities,Recipes,Step);
    if(auto* Catalog=GetGameInstance()->GetSubsystem<UCargoCatalogSubsystem>())
    {FEconomyRuntime::InitializeMarkets(State,*Catalog);EconomyController.Advance(State,*Catalog,Step,Now);}
}
void UGalaxyTrafficSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
    Super::Initialize(Collection);bCatalogsLoaded=LoadEconomyCatalogs();FString Json;TSharedPtr<FJsonObject> Config;
    FString PatrolText;TArray<TSharedPtr<FJsonValue>> PatrolRows;
    if(FFileHelper::LoadFileToString(PatrolText,*(FPaths::ProjectContentDir()/TEXT("GameData/Traffic/patrols.json"))) && FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(PatrolText),PatrolRows))
        for(const auto& Value:PatrolRows)if(auto O=Value->AsObject())
        {FString Design;int32 IFF=0;if(O->TryGetStringField(TEXT("Design"),Design) && !Design.IsEmpty() && O->TryGetNumberField(TEXT("IFF"),IFF) && (IFF==1 || IFF==2 || IFF==3))PatrolHulls.Add({FName(*Design),IFF});}

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
    if(auto* Simulation=Sim::GetSim())Simulation->OnMissionUnloading.RemoveAll(this);
    LiveShips.Reset();LiveGoals.Reset();Sectors.Reset();Endpoints.Reset();Super::Deinitialize();
}
FName UGalaxyTrafficSubsystem::SectorKey(SimRegion* Region)const
{return Region && Region->GetSystem()?NodeKey(FName(UTF8_TO_TCHAR(Region->GetSystem()->GetName())),FName(UTF8_TO_TCHAR(Region->GetName()))):NAME_None;}
void UGalaxyTrafficSubsystem::DetachLive()
{
    if(auto* Simulation=Sim::GetSim()){TArray<FGuid> Ids;LiveShips.GetKeys(Ids);for(FGuid Id:Ids)Despawn(*Simulation,Id);}
    LiveGoals.Reset();LastMission=nullptr;
}
void UGalaxyTrafficSubsystem::BuildGraph(Sim& Simulation)
{
    Simulation.OnMissionUnloading.RemoveAll(this);
    Simulation.OnMissionUnloading.AddUObject(this,&UGalaxyTrafficSubsystem::DetachLive);
    Sectors.Reset();Endpoints.Reset();TMap<FName,StarSystem*> Systems;
    for(ListIter<StarSystem> Iter=Simulation.GetSystemList();++Iter;)
    {
        auto* System=Iter.value();if(!System)continue;const FName SystemId(UTF8_TO_TCHAR(System->GetName()));Systems.Add(SystemId,System);
        for(ListIter<OrbitalRegion> Regions=System->GetAllRegions();++Regions;)
        {
            auto* Region=Regions.value();if(!Region)continue;
            FGalaxyTrafficSector Node;Node.System=SystemId;Node.Region=FName(UTF8_TO_TCHAR(Region->GetName()));Node.Id=NodeKey(Node.System,Node.Region);Node.bSurface=Region->GetType()==Orbital::TERRAIN;Node.Affiliation=System->GetAffiliation();Sectors.Add(Node.Id,Node);
        }
    }
    // Include instantiated regions too: authored surface/mission sectors may not
    // appear in the all-orbits table. They remain abstract if atmospheric.
    for(ListIter<SimRegion> Iter=Simulation.GetRegions();++Iter;)
        if(auto* Region=Iter.value();Region && Region->GetSystem())
        {FGalaxyTrafficSector Node;Node.Id=SectorKey(Region);Node.System=FName(UTF8_TO_TCHAR(Region->GetSystem()->GetName()));Node.Region=FName(UTF8_TO_TCHAR(Region->GetName()));Node.bSurface=Region->IsAirSpace();Node.Affiliation=Region->GetSystem()->GetAffiliation();Sectors.FindOrAdd(Node.Id)=Node;}
    for(const auto& Pair:CombatGroupRegistry::GetAll())
    {
        const auto& Group=Pair.Value;if(Group.System.IsEmpty() || Group.Region.IsEmpty() || !Systems.Contains(FName(*Group.System)))continue;
        const FName Id=NodeKey(FName(*Group.System),FName(*Group.Region));
        if(!Sectors.Contains(Id))
        {FGalaxyTrafficSector Node;Node.Id=Id;Node.System=FName(*Group.System);Node.Region=FName(*Group.Region);
            for(const auto& Unit:Group.Unit)Node.bSurface|=Unit.UnitDesign.Contains(TEXT("Airbase"),ESearchCase::IgnoreCase);
            Node.Affiliation=Systems.FindRef(FName(*Group.System))->GetAffiliation();Sectors.Add(Id,Node);}
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
    for(const auto& Mine:Mines)
    {
        const FName Sector=NodeKey(Mine.System,Mine.Region);
        if(!Sectors.Contains(Sector))continue;
        FGalaxyTrafficEndpoint E;E.Id=FName(*(Sector.ToString()+TEXT("/")+Mine.Region.ToString()));E.Sector=Sector;E.System=Mine.System;E.Region=Mine.Region;E.Station=Mine.Region;
        E.Location=FVector::ZeroVector;E.bPhysical=false;Endpoints.Add(E.Id,E);
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
    if(const auto* Name=LiveShips.Find(Id))if(auto* Craft=Simulation.FindShip(TCHAR_TO_UTF8(**Name)))
    {
        auto* Players=GetGameInstance()->GetSubsystem<UStarshatterPlayerSubsystem>();
        if(Players)for(auto& Journey:Players->GetMutablePlayerInfo().Traffic.Journeys)
            if(Journey.Id==Id && Journey.IsCargo() && !Journey.IsFinished())
            {
                const FName System=Craft->GetRegion() && Craft->GetRegion()->GetSystem()?FName(UTF8_TO_TCHAR(Craft->GetRegion()->GetSystem()->GetName())):NAME_None;
                if(!TrafficAI::AbstractCargo(Journey,Simulation.GetEconomy(),System))return;
                break;
            }
        if(Players)for(auto& Journey:Players->GetMutablePlayerInfo().Traffic.Journeys)if(Journey.Id==Id && Journey.bPatrol)
        {
            Journey.PatrolPosition=Craft->GetLocation();Journey.PatrolVelocity=Craft->GetVelocity();Journey.bHasPatrolPosition=true;
            Journey.HullFraction=Craft->Design() && Craft->Design()->integrity>0?FMath::Clamp(Craft->GetIntegrity()/Craft->Design()->integrity,0.0,1.0):0;
            Journey.PatrolAmmo.Reset();for(ListIter<WeaponGroup> G=Craft->GetWeapons();++G;)for(ListIter<Weapon> W=G->GetWeapons();++W;)Journey.PatrolAmmo.Add(W->Ammo());
        }
        Simulation.DestroyShip(Craft);
    }
    LiveShips.Remove(Id);LiveGoals.Remove(Id);
}
void UGalaxyTrafficSubsystem::Realize(Sim& Simulation,FGalaxyTrafficJourney& Job,FEconomyState& Economy)
{
    auto* Players=GetGameInstance()?GetGameInstance()->GetSubsystem<UStarshatterPlayerSubsystem>():nullptr;if(!Players)return;
    int32 Existing=0;for(const auto& Pair:LiveShips)for(const auto& Row:Players->GetPlayerInfo().Traffic.Journeys)if(Row.Id==Pair.Key && Row.bPatrol==Job.bPatrol)++Existing;
    if(Existing>=(Job.bPatrol?PatrolBudget:ActiveBudget) || LiveShips.Contains(Job.Id) || Job.IsFinished())return;
    if(Job.bPatrol && (!Simulation.GetMission() || !TrafficAI::PatrolAllowed(Simulation.GetMission()->IsFreeFlight(),Cast<USSWGameInstance>(GetGameInstance())->IsTrainingTestSession(),Simulation.IsNetworked(),false)))return;
    const auto* Node=Sectors.Find(Job.CurrentSector);if(!Node || Node->bSurface)return;
    auto* Region=Simulation.FindRegion(Node->Region.ToString());if(!Region || Region->IsAirSpace() || SectorKey(Region)!=Node->Id)return;
    bool Visible=Region==Simulation.GetActiveRegion();
    for(ListIter<Ship> Ships=Region->GetShips();++Ships;)Visible|=Ships->HasNetworkPilot();
    if(!Visible)return;
    ShipDesign* Design=ShipDesignRegistry::FindLegacy(Job.bPatrol?Job.ShipDesign:FName(Job.IsCargo()?TEXT("IM3500"):TEXT("Courier")));if(!Design)return;
    const FString Name=(Job.bPatrol?TEXT("Patrol ")+Job.PilotName+TEXT(" "):FString(TEXT("Traffic ")))+Job.Id.ToString(EGuidFormats::Digits).Left(12);
    if(auto* Restored=Simulation.FindShip(TCHAR_TO_UTF8(*Name)))
    {
        if(Restored!=Simulation.GetPlayerShip() && !Restored->HasNetworkPilot() && Restored->Design()==Design && Restored->GetRegion()==Region && !Restored->IsDead() && !Restored->IsDying())LiveShips.Add(Job.Id,Name);
        return; // A checkpoint-owned ship is never duplicated or reset to spawn condition.
    }
    const auto* From=Endpoints.Find(Job.Origin);if(!From)return;
    const auto* VisibleGoal=Endpoints.Find(Job.Phase==EGalaxyTrafficPhase::Waiting || Job.Phase==EGalaxyTrafficPhase::Loading?Job.Origin:Job.Destination);
    if(!VisibleGoal || (VisibleGoal->bPhysical && !StationAt(Simulation,*VisibleGoal)))return;
    FVector Center=Job.CurrentSector==From->Sector?From->Location:FVector(30000,8000,0);
    TArray<TPair<FVector,double>> Obstacles;
    for(ListIter<Ship> Ships=Region->GetShips();++Ships;)if(Ships.value())Obstacles.Emplace(Ships->GetLocation(),Ships->GetNavigationHullRadius());
    for(ListIter<Asteroid> Rocks=Region->GetRoids();++Rocks;)if(Rocks.value())Obstacles.Emplace(Rocks->GetLocation(),Rocks->GetRadius());
    FVector Point;bool Safe=false;
    if(Job.bPatrol && Job.bHasPatrolPosition && TrafficAI::SafeAnchor(Job.PatrolPosition,FMath::Max(1000.0,double(Design->radius)),Obstacles)){Point=Job.PatrolPosition;Safe=true;}
    for(int32 Attempt=0;Attempt<12 && !Safe;++Attempt)
    {
        const double Angle=(Attempt+Job.Id.A%12)*PI/6;
        Point=Center+FVector(FMath::Cos(Angle)*18000,5000,FMath::Sin(Angle)*18000);
        if(TrafficAI::SafeAnchor(Point,FMath::Max(1000.0,double(Design->radius)),Obstacles) &&
            (!Simulation.GetPlayerShip() || FVector::DistSquared(Point,Simulation.GetPlayerShip()->GetLocation())>FMath::Square(12000.0))) {Safe=true;break;}
    }
    if(!Safe)return;
    auto* Craft=Simulation.CreateShip(TCHAR_TO_UTF8(*Name),"",Design,TCHAR_TO_UTF8(*Node->Region.ToString()),OtherHand(Point),Job.bPatrol?Job.PatrolIFF:0,0);
    if(!Craft)return;
    if(Job.bPatrol)
    {
        Craft->SetIntegrity(Design->integrity*Job.HullFraction);if(Job.bHasPatrolPosition)Craft->SetVelocity(Job.PatrolVelocity);
        int32 Index=0;for(ListIter<WeaponGroup> G=Craft->GetWeapons();++G;)for(ListIter<Weapon> W=G->GetWeapons();++W;)if(Job.PatrolAmmo.IsValidIndex(Index))W->SetAmmo(Job.PatrolAmmo[Index++]);
    }

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
        if(Arrived && Job.bPatrol){Swap(Job.Origin,Job.Destination);LiveGoals.Remove(Job.Id);Craft->SetAutoNav(false);continue;}
        if(Arrived){Craft->SetAutoNav(false);Craft->StopScriptedMotion();}
        else if(!LiveGoals.Contains(Job.Id) || !LiveGoals.FindChecked(Job.Id).Equals(Destination,100))
        {
            Craft->SetAutoNav(false);Craft->ClearFlightPlan();auto* Waypoint=new Instruction(Region,OtherHand(Destination));Waypoint->SetSpeed(250);Waypoint->SetWeaponsFree(Job.bPatrol?1:0);Craft->AddNavPoint(Waypoint);Craft->SetAutoNav(true);LiveGoals.Add(Job.Id,Destination);
        }
    }
}
void UGalaxyTrafficSubsystem::Tick(float DeltaSeconds)
{
    auto* Simulation=Sim::GetSim();auto* Runtime=GetGameInstance()->GetSubsystem<USSWRuntimeSubsystem>();auto* Players=GetGameInstance()->GetSubsystem<UStarshatterPlayerSubsystem>();
    const auto* Director=Starshatter::GetInstance();
    const bool Playing=Simulation && Simulation->GetMission() && Runtime && Runtime->GetGameMode()==EGameMode::PLAY;
    const auto* Instance=Cast<USSWGameInstance>(GetGameInstance());
    if(IsRunningCommandlet() || !Players || !Players->HasLoaded() || !GetWorld() || GetWorld()->IsPaused() ||
        (Instance && Instance->IsTrainingTestSession()) || (Director && Director->InCutscene()) || OfflineCutsceneExport::IsRequested() ||
        (Simulation && (!Simulation->HasSimulationAuthority() || Simulation->IsNetworked())))return;
    auto& Pilot=Players->GetMutablePlayerInfo();auto& Traffic=Pilot.Traffic;auto& Economy=Players->GetEconomy();
    if(!Playing && Economy.EconomyClockSeconds<=0)return;
    const double Seconds=FMath::Max(0.0,double(DeltaSeconds))*(Playing && Runtime?FMath::Max(1u,Runtime->GetTimeCompression()):1u);
    if(Sectors.IsEmpty() && Simulation)BuildGraph(*Simulation);
    if(bPersistenceBlocked)
    {
        for(const auto& Pair:LiveShips)if(auto* Craft=Simulation?Simulation->FindShip(TCHAR_TO_UTF8(*Pair.Value)):nullptr)
        {Craft->SetAutoNav(false);Craft->StopScriptedMotion();}
        PersistenceRetry+=DeltaSeconds;if(PersistenceRetry<2)return;PersistenceRetry=0;
        if(!Players->SaveActivePlayerProgress())return;
        bPersistenceBlocked=false;LiveGoals.Reset();CheckpointElapsed=0;
    }
    TickEconomy(Economy,Seconds,FDateTime::UtcNow().ToUnixTimestamp());
    if(Sectors.IsEmpty())return;
    if(auto* Player=Simulation?Simulation->GetPlayerShip():nullptr;Player && Player->Design() && FCStringAnsi::Stricmp(Player->Design()->name,"CameraPod")==0)return;

    if(Simulation && LastMission!=Simulation->GetMission())
    {TArray<FGuid> Old;LiveShips.GetKeys(Old);for(FGuid Id:Old)Despawn(*Simulation,Id);LiveShips.Reset();LiveGoals.Reset();LastMission=Simulation->GetMission();BuildGraph(*Simulation);}
    if(Simulation)UpdateLive(*Simulation,Traffic,Economy,Seconds);Accumulator+=Seconds;ScheduleElapsed+=Seconds;if(Accumulator<1)return;
    const double Step=Accumulator;Accumulator=0;Traffic.Clock+=Step;CheckpointElapsed+=Step;bool Changed=TrafficAI::RecoverBlocked(Traffic,Economy,Sectors,Endpoints)>0;
    if(Playing && Instance)if(auto* Character=PlayerCharacter::GetCurrentPlayer())Changed|=DynamicMissionAI::Advance(*Simulation,Pilot,const_cast<USSWGameInstance*>(Instance)->GetTrainingAgent(),AgentAI::PilotKey(*Character),Step);

    for(auto& Job:Traffic.Journeys)
    {
        if(Job.IsFinished()){Job.Age+=Step;continue;}
        bool Realized=Simulation && LiveShips.Contains(Job.Id),Arrived=false;
        if(Realized)
        {
            auto* Craft=Simulation->FindShip(TCHAR_TO_UTF8(*LiveShips.FindChecked(Job.Id)));
            const auto* Endpoint=Endpoints.Find(Job.Phase==EGalaxyTrafficPhase::Waiting || Job.Phase==EGalaxyTrafficPhase::Loading?Job.Origin:Job.Destination);
            if(Craft && Endpoint && (!Endpoint->bPhysical || StationAt(*Simulation,*Endpoint)) && SectorKey(Craft->GetRegion())==Endpoint->Sector)
            {const FVector Destination=Goal(*Simulation,*Endpoint,*Craft);Arrived=FVector::DistSquared(Craft->GetLocation(),Destination)<FMath::Square(FMath::Max(800.0,LegacyWaypointArrival::Radius(Craft->GetNavigationHullRadius())));}
        }
        const auto PreviousPhase=Job.Phase;const int32 PreviousLeg=Job.RouteIndex;const FName PreviousHold=Job.Hold;
        TrafficAI::Advance(Job,Economy,Step,Realized,Arrived,HopSeconds,TransferTonnesPerSecond);
        if(Simulation && Realized && Job.IsFinished())Despawn(*Simulation,Job.Id);
        else if(Simulation && !Realized && !Job.IsFinished())Realize(*Simulation,Job,Economy);
        Changed|=PreviousPhase!=Job.Phase || PreviousLeg!=Job.RouteIndex || PreviousHold!=Job.Hold;
    }
    Traffic.Journeys.RemoveAll([](const auto& Job){return Job.IsFinished() && Job.Age>120;});
    if(ScheduleElapsed>=ScheduleSeconds && Traffic.Journeys.Num()<MaxJourneys)
    {
        ScheduleElapsed=0;const int32 Count=Traffic.Journeys.Num();
        if(Playing && Instance && Simulation->GetMission() && TrafficAI::PatrolAllowed(Simulation->GetMission()->IsFreeFlight(),Instance->IsTrainingTestSession(),Simulation->IsNetworked(),false))
        {
            if(const auto* Active=Sectors.Find(SectorKey(Simulation->GetActiveRegion())))TrafficAI::SchedulePatrol(Traffic,*Active,PatrolHulls,MaximumPatrols);
            TArray<FName> Keys;Sectors.GetKeys(Keys);Keys.Sort([](FName A,FName B){return A.LexicalLess(B);});
            if(!Keys.IsEmpty())TrafficAI::SchedulePatrol(Traffic,Sectors.FindChecked(Keys[Traffic.Sequence%Keys.Num()]),PatrolHulls,MaximumPatrols);
        }

        TrafficAI::Dispatch(Traffic,Economy,Sectors,Endpoints,SectorKey(Simulation?Simulation->GetActiveRegion():nullptr),MaxJourneys);
        Changed|=Count!=Traffic.Journeys.Num();
    }
    // Keep cargo and journey position in the same existing atomic player save.
    // A failed checkpoint suspends further traffic decisions instead of replaying
    // an already completed delivery. Dedicated world persistence is a separate service.
    if(Changed || CheckpointElapsed>=300)
    {
        if(!Players->SaveActivePlayerProgress())
        {bPersistenceBlocked=true;PersistenceRetry=0;LiveGoals.Reset();UE_LOG(LogTemp,Error,TEXT("[GalaxyTraffic] Checkpoint failed; traffic suspended pending atomic save retry."));}
        else CheckpointElapsed=0;
    }
}
