#include "MiningDroneSubsystem.h"
#include "MiningDroneActor.h"
#include "ShipActor.h"
#include "StarshatterPlayerSubsystem.h"
#include "ShipInsuranceSubsystem.h"
#include "SSWRuntimeSubsystem.h"
#include "OfflineCutsceneExport.h"
#include "Starshatter.h"
#include "WeaponDesignRegistry.h"
#include "WeaponDesign.h"
#include "Ship.h"
#include "ShipDesign.h"
#include "Sim.h"
#include "SimRegion.h"
#include "Asteroid.h"
#include "MiningAI.h"
#include "MiningDroneSaveTransaction.h"
#include "MiningDroneBayService.h"
#include "CargoCatalogSubsystem.h"
#include "StationServices.h"
#include "GameStructs_economy.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "JsonObjectConverter.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"

namespace
{
bool ActiveDroneGameplay(UGameInstance* Instance,UWorld* World,Sim* Simulation)
{
    const auto* Runtime=Instance?Instance->GetSubsystem<USSWRuntimeSubsystem>():nullptr;const auto* Director=Starshatter::GetInstance();
    const bool Playing=Simulation && Simulation->GetMission() && Runtime && Runtime->GetGameMode()==EGameMode::PLAY;
    return World && MiningDroneAI::Eligible(Simulation && Simulation->HasSimulationAuthority(),Playing,Director && Director->InCutscene(),OfflineCutsceneExport::IsRequested(),World->IsPaused());
}
}

void UMiningDroneSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{Super::Initialize(Collection);LoadCatalog();}
void UMiningDroneSubsystem::Deinitialize()
{for(auto& Pair:Visuals)if(Pair.Value.IsValid())Pair.Value->Destroy();Visuals.Reset();Super::Deinitialize();}
bool UMiningDroneSubsystem::LoadCatalog()
{
    FString Json;TArray<TSharedPtr<FJsonValue>> Rows;TMap<FName,FMiningDroneBayDefinition> Candidate;
    if(!FFileHelper::LoadFileToString(Json,*(FPaths::ProjectContentDir()/TEXT("GameData/Cargo/mining_drones.json"))) || !FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(Json),Rows))return false;
    for(const auto& Row:Rows)
    {
        const auto Object=Row->AsObject();FMiningDroneBayDefinition Bay;
        if(!Object || !FJsonObjectConverter::JsonObjectToUStruct(Object.ToSharedRef(),&Bay) || Bay.ShipDesign.IsNone() || Candidate.Contains(Bay.ShipDesign) ||
            Bay.BayCount<1 || Bay.BayCount>12 || Bay.Bandwidth<1 || Bay.Bandwidth>12 || Bay.DroneBandwidth<1 || Bay.DroneBandwidth>Bay.Bandwidth ||
            !FMath::IsFinite(Bay.CapacityTonnes) || Bay.CapacityTonnes<=0 || Bay.CapacityTonnes>100 || !FMath::IsFinite(Bay.Speed) || Bay.Speed<=0 || Bay.Speed>2000 ||
            !FMath::IsFinite(Bay.ControlRange) || Bay.ControlRange<100 || Bay.ControlRange>100000 || !FMath::IsFinite(Bay.MiningRange) || Bay.MiningRange<=0 || Bay.MiningRange>1000 ||
            !FMath::IsFinite(Bay.MiningRateScale) || Bay.MiningRateScale<=0 || Bay.MiningRateScale>1 || Bay.MiningWeapon.IsNone())return false;
        Candidate.Add(Bay.ShipDesign,Bay);
    }
    if(Candidate.IsEmpty())return false;Bays=MoveTemp(Candidate);return true;
}
const FMiningDroneBayDefinition* UMiningDroneSubsystem::GetBay(const Ship* Craft) const
{return Craft?Bays.Find(FName(UTF8_TO_TCHAR(Craft->DesignName()))):nullptr;}
bool UMiningDroneSubsystem::HasBay() const
{auto* Simulation=Sim::GetSim();return GetBay(Simulation?Simulation->GetPlayerShip():nullptr)!=nullptr;}
bool UMiningDroneSubsystem::Authority(FText& Reason) const
{
    auto* Simulation=Sim::GetSim();auto* Owner=Simulation?Simulation->GetPlayerShip():nullptr;
    if(!ActiveDroneGameplay(GetGameInstance(),GetWorld(),Simulation) || Simulation->IsNetworked() || !Owner || Owner->IsDead() || Owner->IsDying() || !GetBay(Owner))
    {Reason=INVTEXT("Mining drone control requires a live fitted ship in unpaused offline gameplay. Cinematics, exports and network commands cannot operate this bay.");return false;}
    return true;
}
const TArray<FMiningDroneState>& UMiningDroneSubsystem::GetDrones() const
{
    static const TArray<FMiningDroneState> Empty;auto* Simulation=Sim::GetSim();auto* Owner=Simulation?Simulation->GetPlayerShip():nullptr;
    const auto* Fleet=Owner?Simulation->ReadEconomy().MiningDrones.Find(Owner->GetCargoInventoryKey()):nullptr;return Fleet?Fleet->Drones:Empty;
}
bool UMiningDroneSubsystem::PrepareBay(FText& Reason)
{
    if(!Authority(Reason))return false;auto* Simulation=Sim::GetSim();auto* Owner=Simulation->GetPlayerShip();
    if(auto* Insurance=GetGameInstance()->GetSubsystem<UShipInsuranceSubsystem>())Insurance->RefreshAssignment();
    auto* Players=GetGameInstance()->GetSubsystem<UStarshatterPlayerSubsystem>();
    if(!Players || !Players->GetPlayerInfo().AssignedHangarHull.IsValid() || Owner->GetCargoInventoryKey()!=FName(*(TEXT("Ship/")+Players->GetPlayerInfo().AssignedHangarHull.ToString(EGuidFormats::Digits))))
    {Reason=INVTEXT("The fitted drone bay needs a persistent assigned hull.");return false;}
    Owner->SyncCargoInventory();auto& State=Simulation->GetEconomy();
    const FName Key=Owner->GetCargoInventoryKey();
    if(auto* Existing=State.MiningDrones.Find(Key))
    {
        if(BoundFleets.Contains(Key))return true;
        const auto Before=State;bool Changed=false;
        for(auto& Drone:Existing->Drones)
        {
            if(Drone.bRepeatMining){Drone.bRepeatMining=false;Changed=true;}
            if(Drone.State!=EMiningDroneState::Docked && Drone.State!=EMiningDroneState::Lost)
            {Drone.State=EMiningDroneState::Returning;Drone.TargetContact=INDEX_NONE;Changed=true;}
        }
        if(Changed && !Players->SaveActivePlayerProgress()){State=Before;Reason=INVTEXT("Could not restore deployed mining drones.");return false;}
        BoundFleets.Add(Key);return true;
    }
    const auto Before=State;const auto* Bay=GetBay(Owner);FMiningDroneFleetState Fleet;Fleet.ShipDesign=Bay->ShipDesign;
    Fleet.Region=Owner->GetRegion()?FName(UTF8_TO_TCHAR(Owner->GetRegion()->GetName())):NAME_None;
    for(int32 Index=0;Index<Bay->BayCount;++Index){FMiningDroneState Drone;Drone.Id=FGuid::NewGuid();Fleet.Drones.Add(Drone);}
    State.MiningDrones.Add(Owner->GetCargoInventoryKey(),Fleet);
    if(!Players || !Players->SaveActivePlayerProgress()){State=Before;Reason=INVTEXT("Could not save the fitted drone bay.");return false;}
    BoundFleets.Add(Key);
    Reason=INVTEXT("Mining drone bay ready.");return true;
}
FMiningDroneFrame UMiningDroneSubsystem::Frame(Ship& Owner,int32 Target,const FMiningDroneBayDefinition& Bay,Asteroid*& Rock) const
{
    FMiningDroneFrame Result;Rock=nullptr;auto* Simulation=Sim::GetSim();Result.bAuthority=Simulation && Simulation->HasSimulationAuthority() && !Simulation->IsNetworked();
    Result.bOwnerAlive=!Owner.IsDead() && !Owner.IsDying();Result.OwnerPosition=Owner.GetLocation();Result.OwnerFreeTonnes=0;
    const auto* Hold=Simulation?Simulation->ReadEconomy().Inventories.Find(Owner.GetCargoInventoryKey()):nullptr;
    if(Hold && Hold->Owner==FEconomyLedger::Pilot())Result.OwnerFreeTonnes=FMath::Max(0.0,Hold->CapacityTonnes-FEconomyLedger::Used(*Hold));
    auto* Region=Owner.GetRegion();if(!Region)return Result;
    for(ListIter<Asteroid> Rocks=Region->GetRoids();++Rocks;)
        if(Rocks.value() && (Target>=0?Rocks.value()->GetContactId()==Target:static_cast<SimObject*>(Rocks.value())==Owner.GetTarget())){Rock=Rocks.value();break;}
    if(!Rock || Rock->GetIntegrity()<=0)return Result;
    const auto Scan=MiningAI::ScanAsteroids(*Region,Owner.GetLocation(),Bay.ControlRange,MiningAI::ScanTier::Assay);
    for(const auto& Reading:Scan.Readings)if(Reading.ContactId==Rock->GetContactId() && Reading.RemainingTonnes.IsSet()){Result.bTargetAssayed=true;break;}
    Result.bTargetValid=true;Result.TargetContact=Rock->GetContactId();Result.TargetPosition=Rock->GetLocation();Result.TargetRadius=Rock->GetRadius();
    Result.TargetOre=DataTableUtils::MakeValidName(Rock->GetResource().GetMineralName().ToString());Result.RemainingTonnes=Rock->GetResource().AmountTonnes;return Result;
}
double UMiningDroneSubsystem::Unload(Ship& Owner,FName Ore,double Amount)
{
    auto* Simulation=Sim::GetSim();if(!Simulation || !Simulation->HasSimulationAuthority() || Simulation->IsNetworked() || Ore.IsNone() || !FMath::IsFinite(Amount) || Amount<=0)return 0;
    auto& State=Simulation->GetEconomy();const auto* Hold=State.Inventories.Find(Owner.GetCargoInventoryKey());
    if(!Hold || Hold->Owner!=FEconomyLedger::Pilot())return 0;
    const double Count=FMath::Min(Amount,FMath::Max(0.0,Hold->CapacityTonnes-FEconomyLedger::Used(*Hold)));
    return Count>0 && FEconomyLedger::Deposit(State,Owner.GetCargoInventoryKey(),Ore,Count,1)?Count:0;
}
bool UMiningDroneSubsystem::CanLoadSpare(FText& Reason) const
{
    if (!Authority(Reason)) return false;
    auto* Simulation = Sim::GetSim(); const auto* Owner = Simulation ? Simulation->GetPlayerShip() : nullptr;
    const auto* Players = GetGameInstance()->GetSubsystem<UStarshatterPlayerSubsystem>();
    if (!Owner || !Players || !Players->GetPlayerInfo().AssignedHangarHull.IsValid() ||
        Owner->GetCargoInventoryKey() != FName(*(TEXT("Ship/") + Players->GetPlayerInfo().AssignedHangarHull.ToString(EGuidFormats::Digits))))
    { Reason = INVTEXT("The fitted drone bay needs your persistent assigned hull."); return false; }
    const auto* Catalog = GetGameInstance()->GetSubsystem<UCargoCatalogSubsystem>();
    const auto* Drone = Catalog ? Catalog->FindCargo(TEXT("mining drone"), TEXT("units")) : nullptr;
    if (!Drone || !FMath::IsNearlyEqual(Drone->Size, .5))
    { Reason = INVTEXT("The mining drone cargo definition is unavailable."); return false; }
    const auto* Bay = GetBay(Owner);
    return Bay && MiningDroneBayService::CanLoad(Simulation->ReadEconomy(), Owner->GetCargoInventoryKey(), *Bay,
        FStationServices::IsDocked(), Simulation->HasSimulationAuthority(), Reason);
}
bool UMiningDroneSubsystem::LoadSpare(FText& Reason)
{
    if (!PrepareBay(Reason)) return false;
    auto* Simulation = Sim::GetSim(); auto* Owner = Simulation ? Simulation->GetPlayerShip() : nullptr;
    if (!Owner) return false;
    Owner->SyncCargoInventory();
    if (!CanLoadSpare(Reason)) return false;
    auto& State = Simulation->GetEconomy(); const auto Before = State;
    const auto* Bay = GetBay(Owner);
    if (!Bay || !MiningDroneBayService::Load(State, Owner->GetCargoInventoryKey(), *Bay,
        FStationServices::IsDocked(), Simulation->HasSimulationAuthority(), Reason)) return false;
    auto* Players = GetGameInstance()->GetSubsystem<UStarshatterPlayerSubsystem>();
    const TArray<TPair<Asteroid*, double>> NoExtraction;
    if (!MiningDroneSaveTransaction::Commit(State, Before, NoExtraction,
        [&]() { return Players && Players->SaveActivePlayerProgress(); }, [&]() { Owner->ApplyCargoInventoryToManifest(); }))
    { Reason = INVTEXT("Could not save the spare drone. Cargo and bay contents were restored."); return false; }
    LastStatus = Reason; return true;
}
bool UMiningDroneSubsystem::Command(FGuid Id,int32 Action,FText& Reason)
{
    if(!Authority(Reason) || !PrepareBay(Reason))return false;auto* Simulation=Sim::GetSim();auto* Owner=Simulation->GetPlayerShip();
    if(Owner->GetFlightPhase()<=EOPSMode::ALERT){Reason=INVTEXT("Launch your ship before deploying mining drones.");return false;}
    Owner->SyncCargoInventory();auto& State=Simulation->GetEconomy();const auto Before=State;auto& Fleet=State.MiningDrones.FindChecked(Owner->GetCargoInventoryKey());
    if(!Id.IsValid())for(const auto& Item:Fleet.Drones)if(Item.State==EMiningDroneState::Docked){Id=Item.Id;break;}
    auto* Drone=Fleet.Drones.FindByPredicate([Id](const auto& Item){return Item.Id==Id;});if(!Drone){Reason=INVTEXT("Select a mining drone.");return false;}
    const auto* Bay=GetBay(Owner);Asteroid* Rock=nullptr;const auto Context=Frame(*Owner,(Action==1 || Action==5)?INDEX_NONE:Drone->TargetContact,*Bay,Rock);bool Changed=false;
    const double PreviousOre=Drone->OreTonnes;
    switch(Action)
    {
    case 0:Changed=MiningDroneAI::Launch(Fleet,Id,*Bay,Context,Reason);Fleet.Region=Owner->GetRegion()?FName(UTF8_TO_TCHAR(Owner->GetRegion()->GetName())):NAME_None;break;
    case 1:Changed=MiningDroneAI::Mine(*Drone,*Bay,Context,Reason);break;
    case 5:Changed=MiningDroneAI::Mine(*Drone,*Bay,Context,Reason,true);break;
    case 2:Changed=MiningDroneAI::Stop(*Drone,Context,Reason);break;
    case 3:Changed=MiningDroneAI::Return(*Drone,Context,Reason);break;
    case 4:Changed=MiningDroneAI::Dock(*Drone,Context,[&](FName Ore,double Tonnes){return Unload(*Owner,Ore,Tonnes);},Reason);break;
    }
    Changed|=PreviousOre!=Drone->OreTonnes;
    if(!Changed){State=Before;return false;}
    Owner->ApplyCargoInventoryToManifest();auto* Players=GetGameInstance()->GetSubsystem<UStarshatterPlayerSubsystem>();
    if(!Players || !Players->SaveActivePlayerProgress()){State=Before;Owner->ApplyCargoInventoryToManifest();Reason=INVTEXT("Could not save drone command; cargo and drone state were restored.");return false;}
    LastStatus=Reason;return true;
}
bool UMiningDroneSubsystem::Launch(FGuid Id,FText& Reason){return Command(Id,0,Reason);}
bool UMiningDroneSubsystem::Mine(FGuid Id,FText& Reason){return Command(Id,1,Reason);}
bool UMiningDroneSubsystem::MineRepeatedly(FGuid Id,FText& Reason){return Command(Id,5,Reason);}
bool UMiningDroneSubsystem::Stop(FGuid Id,FText& Reason){return Command(Id,2,Reason);}
bool UMiningDroneSubsystem::Return(FGuid Id,FText& Reason){return Command(Id,3,Reason);}
bool UMiningDroneSubsystem::Dock(FGuid Id,FText& Reason){return Command(Id,4,Reason);}
void UMiningDroneSubsystem::UpdateVisuals(Ship& Owner,const FMiningDroneFleetState& Fleet)
{
    auto* World=GetWorld();if(!World)return;AShipActor* OwnerActor=nullptr;
    for(TActorIterator<AShipActor> Actors(World);Actors;++Actors)if(Actors->GetRuntimeShip()==&Owner){OwnerActor=*Actors;break;}
    if(!OwnerActor)return;
    TSet<FGuid> Active;const auto Convert=[](FVector Delta){return FVector(Delta.Z,Delta.X,Delta.Y);};
    for(const auto& Drone:Fleet.Drones)
    {
        if(Drone.State==EMiningDroneState::Docked || Drone.State==EMiningDroneState::Lost)continue;Active.Add(Drone.Id);
        auto& Visual=Visuals.FindOrAdd(Drone.Id);if(!Visual.IsValid())Visual=World->SpawnActor<AMiningDroneActor>();if(!Visual.IsValid())continue;
        Asteroid* Rock=nullptr;const auto* Bay=GetBay(&Owner);const auto Context=Frame(Owner,Drone.TargetContact,*Bay,Rock);
        const FVector Position=OwnerActor->GetActorLocation()+Convert(Drone.Position-Owner.GetLocation());
        const FVector Target=OwnerActor->GetActorLocation()+Convert(Context.TargetPosition-Owner.GetLocation());
        Visual->UpdateVisual(Position,Context.bTargetValid?Target-Position:OwnerActor->GetActorLocation()-Position,Drone.State==EMiningDroneState::Mining && Context.bTargetValid,Target);
    }
    for(auto It=Visuals.CreateIterator();It;++It)if(!Active.Contains(It.Key())){if(It.Value().IsValid())It.Value()->Destroy();It.RemoveCurrent();}
}
void UMiningDroneSubsystem::Tick(float DeltaTime)
{
    auto* Simulation=Sim::GetSim();auto* Owner=Simulation?Simulation->GetPlayerShip():nullptr;
    if(!ActiveDroneGameplay(GetGameInstance(),GetWorld(),Simulation) || Simulation->IsNetworked() || !Owner || !GetBay(Owner))
    {Accumulator=0;if(!GetWorld() || !GetWorld()->IsPaused()){for(auto& Pair:Visuals)if(Pair.Value.IsValid())Pair.Value->Destroy();Visuals.Reset();}return;}
    auto& State=Simulation->GetEconomy();const FName Key=Owner->GetCargoInventoryKey();auto* Fleet=State.MiningDrones.Find(Key);
    if(!Fleet){for(auto& Pair:Visuals)if(Pair.Value.IsValid())Pair.Value->Destroy();Visuals.Reset();return;}
    if(!BoundFleets.Contains(Key) && !Owner->IsDead() && !Owner->IsDying()){FText Reason;if(!PrepareBay(Reason))return;Fleet=State.MiningDrones.Find(Key);if(!Fleet)return;}
    UpdateVisuals(*Owner,*Fleet);Accumulator+=DeltaTime;if(Accumulator<.5)return;const double Seconds=FMath::Min(Accumulator,1.0);Accumulator=0;
    const auto Before=State;const auto* Bay=GetBay(Owner);const auto* Weapon=WeaponDesignRegistry::FindLegacy(Bay->MiningWeapon);
    const double Rate=Weapon?Weapon->MiningTonnesPerSecond*Bay->MiningRateScale:0;bool Changed=false;TArray<TPair<Asteroid*,double>> Extracted;
    const FName Region=Owner->GetRegion()?FName(UTF8_TO_TCHAR(Owner->GetRegion()->GetName())):NAME_None;
    for(auto& Drone:Fleet->Drones)
    {
        if(Drone.State==EMiningDroneState::Docked || Drone.State==EMiningDroneState::Lost)continue;
        if(Fleet->Region!=Region){MiningDroneAI::Lose(Drone);Changed=true;continue;}
        Asteroid* Rock=nullptr;const auto Context=Frame(*Owner,Drone.TargetContact,*Bay,Rock);
        Changed|=MiningDroneAI::Tick(Drone,*Bay,Context,Seconds,Rate,[&](double Requested)
        {const double Amount=Rock?Rock->ExtractOre(Requested):0;if(Amount>0)Extracted.Emplace(Rock,Amount);return Amount;},[&](FName Ore,double Tonnes){return Unload(*Owner,Ore,Tonnes);});
    }
    if(!Changed)return;auto* Players=GetGameInstance()->GetSubsystem<UStarshatterPlayerSubsystem>();
    if(!MiningDroneSaveTransaction::Commit(State,Before,Extracted,[&](){return Players && Players->SaveActivePlayerProgress();},[&](){Owner->ApplyCargoInventoryToManifest();}))
        LastStatus=INVTEXT("Could not save mining drones; asteroid, drone cargo and owner hold were restored.");
}
