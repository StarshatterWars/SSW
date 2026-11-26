#include "LocalGameStateSave.h"
#include "Sim.h"
#include "SimRegion.h"
#include "SimElement.h"
#include "Ship.h"
#include "ShipDesign.h"
#include "ShipDesignRegistry.h"
#include "StarSystem.h"
#include "Asteroid.h"
#include "Camera.h"
#include "Instruction.h"
#include "Mission.h"
#include "MissionElement.h"
#include "MissionEvent.h"
#include "Weapon.h"
#include "WeaponGroup.h"
#include "Power.h"
#include "Shield.h"
#include "OwnedHullCondition.h"
#include "Game.h"
#include "FlightDeck.h"
#include "Hangar.h"
#include "DeckResumeState.h"
#include "CampaignFleetTransit.h"
void FLocalWorldStateCodec::Capture(Sim& Simulation,ULocalUniverseState& Out)
{
    if(!Simulation.mission || Simulation.IsNetworked() || !Simulation.GetPlayerShip())return;
    Out.NavigationCoordinatesVersion=1;
    const auto OldShips=Out.Ships;const bool Same=Out.MissionId==Simulation.mission->GetIdentity();
    Out.AgentAssignmentId=Simulation.AssignedAgentMissionId;Out.MissionId=Simulation.mission->GetIdentity();Out.MissionName=UTF8_TO_TCHAR(Simulation.mission->GetName());Out.bMissionComplete=Simulation.mission->IsComplete();
    Out.MissionElapsed=double(Game::GetGameTime()-Simulation.start_time)/1000.0;Out.Ships.Reset();Out.Asteroids.Reset();Out.MiningRuns.Reset();Out.EventIds.Reset();Out.EventStatuses.Reset();
    for(ListIter<SimRegion> R=Simulation.regions;++R;)
    {
        auto* Region=R.value();if(!Region)continue;
        for(ListIter<Ship> S=Region->GetShips();++S;)
        {
            auto* Vessel=S.value();if(!Vessel || !Vessel->Design())continue;FLocalShipState Row;
            Row.HullId=Vessel->GetHullId();
            Simulation.HullRegistry.Register(Row.HullId,FName(UTF8_TO_TCHAR(Vessel->DesignName())),UTF8_TO_TCHAR(Vessel->GetName()),FName(*FString::Printf(TEXT("Faction/%d"),Vessel->GetIFF())),Vessel->GetIFF());
            Simulation.HullRegistry.Rename(Row.HullId,UTF8_TO_TCHAR(Vessel->GetName()));
            if(Vessel->IsDead() || Vessel->IsDying())Simulation.HullRegistry.SetStatus(Row.HullId,EHullOperationalState::Destroyed);
            else if(const auto* Record=Simulation.HullRegistry.Find(Row.HullId);Record && Record->Status!=EHullOperationalState::Repairing)
                Simulation.HullRegistry.SetStatus(Row.HullId,Vessel->GetIntegrity()<Vessel->Design()->integrity?EHullOperationalState::Damaged:EHullOperationalState::Operational);
            Row.bMagazineIssue=Vessel->HasMagazineIssue();
            Row.Name=UTF8_TO_TCHAR(Vessel->GetName());Row.Design=UTF8_TO_TCHAR(Vessel->DesignName());Row.Region=UTF8_TO_TCHAR(Region->GetName());Row.System=Region->GetSystem()?UTF8_TO_TCHAR(Region->GetSystem()->GetName()):TEXT("");
            Row.Element=Vessel->GetElement()?UTF8_TO_TCHAR(Vessel->GetElement()->GetName()):TEXT("");Row.IFF=Vessel->GetIFF();Row.bDestroyed=Vessel->IsDead() || Vessel->IsDying();
            Row.Position=Vessel->GetLocation();Row.Velocity=Vessel->GetVelocity();Row.Forward=Vessel->GetHeading();Row.Up=Vessel->GetLiftLine();Row.Right=Vessel->GetBeamLine();Row.Throttle=Vessel->GetThrottle();
            Row.FlightPhase=static_cast<int32>(Vessel->GetFlightPhase());if(auto* Carrier=Vessel->GetCarrier()){Row.Carrier=UTF8_TO_TCHAR(Carrier->GetName());for(int32 I=0;I<Carrier->NumFlightDecks();++I)if(Carrier->GetFlightDeck(I)==Vessel->GetDock())Row.Deck=I;}
            FGuid::ParseExact(Vessel->GetCargoInventoryKey().ToString().Right(32),EGuidFormats::Digits,Row.CargoId);
            FOwnedHullCondition::Capture(*Vessel,Row.Condition);if(Vessel->GetShield())Row.Shield=FMath::RoundToInt(Vessel->GetShield()->GetShieldLevel());
            for(ListIter<WeaponGroup> Groups=Vessel->GetWeapons();++Groups;)for(ListIter<Weapon> Weapons=Groups->GetWeapons();++Weapons;)Row.Ammo.Add(Weapons->Ammo());
            for(ListIter<PowerSource> Reactors=Vessel->GetReactors();++Reactors;)Row.Fuel.Add(Reactors->GetCapacity());
            Row.bHasFlightPlan=true;
            for(ListIter<Instruction> Nav=Vessel->GetFlightPlan();++Nav;)
            {
                Row.Navigation.Add(static_cast<int32>(Nav->GetStatus()));FLocalNavState N;
                N.Region=UTF8_TO_TCHAR(Nav->GetRegionName());N.Position=Nav->GetLocation();N.Target=UTF8_TO_TCHAR(Nav->GetTargetName());
                N.Action=int32(Nav->GetAction());N.Status=int32(Nav->GetStatus());N.Formation=int32(Nav->GetFormation());N.Priority=Nav->GetPriority();N.Speed=Nav->GetSpeed();
                N.Emcon=Nav->GetEMCON();N.WeaponsFree=Nav->GetWeaponsFree();N.Farcast=Nav->GetFarcast();N.Hold=Nav->GetHoldTime();Row.FlightPlan.Add(N);
            }
            if(auto* Element=Vessel->GetElement())for(int32 I=0;I<Element->NumObjectives();++I)Row.Objectives.Add(Element->GetObjective(I)?static_cast<int32>(Element->GetObjective(I)->GetStatus()):0);
            Out.Ships.Add(MoveTemp(Row));
            if(const auto* Run=Simulation.JanekMiningRuns.Find(Vessel->GetIdentity())){FLocalMiningRunState Work;Work.Ship=UTF8_TO_TCHAR(Vessel->GetName());Work.Time=Run->Time;Work.NextPulse=Run->NextPulse;Work.bDelivering=Run->bDelivering;Work.LoadedTonnes=Run->LoadedTonnes;Work.Reservation=Run->Contract.GetId();Work.DepositId=Run->DepositId.Get(INDEX_NONE);Out.MiningRuns.Add(Work);}
        }
        int32 Index=0;for(ListIter<Asteroid> A=Region->GetRoids();++A;){if(!A.value())continue;FLocalAsteroidState Row;Row.Region=UTF8_TO_TCHAR(Region->GetName());Row.Index=Index++;Row.Position=A->GetLocation();Row.ContactId=A->ContactId;Row.Resource=A->Resource;Row.InitialTonnes=A->InitialResourceTonnes;Row.Integrity=A->GetIntegrity();Out.Asteroids.Add(Row);}
    }
    if(Same)for(auto Old:OldShips)if(!Out.Ships.ContainsByPredicate([&](const auto& S)
        {return Old.HullId.IsValid()?S.HullId==Old.HullId:(S.Name==Old.Name && S.Design==Old.Design);}))
        {Old.bDestroyed=true;Out.Ships.Add(Old);}
    for(ListIter<MissionEvent> E=Simulation.events;++E;){Out.EventIds.Add(E->EventID());Out.EventStatuses.Add(static_cast<int32>(E->status));}
    Out.HullRegistry=Simulation.HullRegistry.Snapshot();
}
bool FLocalWorldStateCodec::Restore(Sim& Simulation,const ULocalUniverseState& In)
{
    if(!Simulation.mission || Simulation.IsNetworked() || In.MissionId!=Simulation.mission->GetIdentity() ||
        (!Simulation.mission->IsFreeFlight() && In.MissionName!=UTF8_TO_TCHAR(Simulation.mission->GetName())) || In.bMissionComplete)return false;
    // Region transfers clear the previous region's player pointer. Retain the
    // actual provisioned pilot until all saved transforms/decks have been applied.
    Ship* ResumePilot=Simulation.GetPlayerShip();
    TSet<FGuid> Ids;
    for(const auto& Row:In.Ships)
    {
        if(Row.HullId.IsValid() && Ids.Contains(Row.HullId))return false;
        if(Row.HullId.IsValid())Ids.Add(Row.HullId);
        if(ResumePilot && Row.HullId.IsValid() && Row.Name==UTF8_TO_TCHAR(ResumePilot->GetName()) &&
            Row.Design==UTF8_TO_TCHAR(ResumePilot->DesignName()) && Row.HullId!=ResumePilot->GetHullId())return false;
    }
    auto ResolveSavedVessel=[&](const FLocalShipState& Row)
        {return Row.HullId.IsValid()?Simulation.FindShipByHullId(Row.HullId):Simulation.FindShip(TCHAR_TO_UTF8(*Row.Name));};
    FString RegistryError;
    if(!Simulation.HullRegistry.Import(In.HullRegistry,RegistryError))
    {UE_LOG(LogTemp,Error,TEXT("[HullRegistry] Checkpoint rejected: %s"),*RegistryError);return false;}
    for(const auto& Row:In.Ships)
    {
        if(Row.Position.ContainsNaN() || Row.Velocity.ContainsNaN())continue;
        auto* Region=CampaignFleetTransit::EnsureRegion(Simulation,Row.System,Row.Region);if(!Region)continue;
        Ship* Vessel=ResolveSavedVessel(Row);
        if(!Vessel && !Row.bDestroyed){auto* Design=ShipDesignRegistry::FindLegacy(FName(*Row.Design));if(Design)Vessel=Simulation.CreateShip(TCHAR_TO_UTF8(*Row.Name),"",Design,TCHAR_TO_UTF8(*Row.Region),OtherHand(Row.Position),Row.IFF,0,nullptr,Row.HullId);}
        if(!Vessel || Row.Design!=UTF8_TO_TCHAR(Vessel->DesignName()))continue;
        if(Vessel->GetRegion()!=Region)Region->TransferObject(Vessel);
        Vessel->MoveTo(Row.Position);Vessel->SetVelocity(Row.Velocity);
        Camera Frame=Vessel->GetCam();Frame.SetOrientation(Row.Right,Row.Up,Row.Forward);Vessel->CloneCam(Frame);
        FOwnedHullCondition::Apply(*Vessel,Row.Condition);Vessel->RestoreSavedCargoInventory(Row.CargoId);
        if(Row.bDestroyed){Vessel->SetIntegrity(0);Vessel->SetTransition(0,Ship::TRANSITION_DEAD,Row.Position);continue;}
        Vessel->SetMagazineIssue(Row.bMagazineIssue);
        int32 Index=0;for(ListIter<WeaponGroup> Groups=Vessel->GetWeapons();++Groups;)for(ListIter<Weapon> Weapons=Groups->GetWeapons();++Weapons;)if(Row.Ammo.IsValidIndex(Index))Weapons->SetAmmo(FMath::Max(0,Row.Ammo[Index++]));
        Index=0;for(ListIter<PowerSource> Reactors=Vessel->GetReactors();++Reactors;)if(Row.Fuel.IsValidIndex(Index))Reactors->SetCapacity(FMath::Clamp(Row.Fuel[Index++],0.0,100.0));
        if(Vessel->GetShield())Vessel->GetShield()->SetNetShieldLevel(FMath::Clamp(Row.Shield,0,100));
        if(Row.bHasFlightPlan)
        {
            MissionElement* AuthoredElement=nullptr;
            if(In.NavigationCoordinatesVersion==0)
                for(ListIter<MissionElement> E=Simulation.mission->GetElements();++E;)
                    if(Row.Element==UTF8_TO_TCHAR(E->GetName().data())){AuthoredElement=E.value();break;}
            // Repair only a complete, exact match to the old coordinate swap.
            // User-edited routes and already-correct checkpoints remain intact.
            bool RepairSwappedRoute=AuthoredElement && Row.FlightPlan.Num()>0 && AuthoredElement->NavList().size()==Row.FlightPlan.Num();
            for(int32 N=0;RepairSwappedRoute && N<Row.FlightPlan.Num();++N)
            {
                const Instruction* Authored=AuthoredElement->NavList()[N];
                RepairSwappedRoute=Authored && Row.FlightPlan[N].Action==int32(Authored->GetAction()) &&
                    Row.FlightPlan[N].Position.Equals(OtherHand(Authored->GetLocation()),.01);
            }
            if(RepairSwappedRoute)UE_LOG(LogTemp,Display,TEXT("[LocalState] Repaired swapped flight-plan coordinates for %s"),*Row.Element);
            Vessel->ClearFlightPlan();
            int32 NavIndex=0;
            for(const auto& N:Row.FlightPlan)
            {
                const int32 SourceIndex=NavIndex++;
                if(N.Position.ContainsNaN() || !FMath::IsFinite(N.Hold))continue;
                const FVector Position=RepairSwappedRoute?AuthoredElement->NavList()[SourceIndex]->GetLocation():N.Position;
                auto* Nav=new Instruction(TCHAR_TO_UTF8(*N.Region),Position,static_cast<EInstruction>(N.Action));
                Nav->SetStatus(static_cast<INSTRUCTION_STATUS>(N.Status));Nav->SetFormation(static_cast<INSTRUCTION_FORMATION>(N.Formation));Nav->SetTarget(TCHAR_TO_UTF8(*N.Target));
                Nav->SetSpeed(N.Speed);Nav->SetPriority(N.Priority);Nav->SetEMCON(N.Emcon);Nav->SetWeaponsFree(N.WeaponsFree);Nav->SetFarcast(N.Farcast);Nav->SetHoldTime(N.Hold);
                Vessel->AddNavPoint(Nav);
            }
        }
        Index=0;for(ListIter<Instruction> Nav=Vessel->GetFlightPlan();++Nav;){if(Row.Navigation.IsValidIndex(Index))Nav->SetStatus(static_cast<INSTRUCTION_STATUS>(Row.Navigation[Index]));++Index;}
        if(auto* Element=Vessel->GetElement())for(int32 I=0;I<Element->NumObjectives() && I<Row.Objectives.Num();++I)if(Element->GetObjective(I))Element->GetObjective(I)->SetStatus(static_cast<INSTRUCTION_STATUS>(Row.Objectives[I]));
        if(!Row.Element.IsEmpty() && !Vessel->GetElement()){auto* Element=Simulation.FindElement(TCHAR_TO_UTF8(*Row.Element));if(!Element)Element=Simulation.CreateElement(TCHAR_TO_UTF8(*Row.Element),Row.IFF);if(Element)Element->AddShip(Vessel);}
    }
    // Remove provisioned slot ownership before applying saved flight phases.
    // Otherwise READY slots teleport an already-airborne saved fighter into the bay.
    for(const auto& Row:In.Ships)if(auto* Vessel=ResolveSavedVessel(Row))
        if(auto* Carrier=Vessel->GetCarrier())
            for(int32 I=0;I<Carrier->NumFlightDecks();++I)
                DeckResumeState::Release(*Carrier->GetFlightDeck(I),*Vessel);
    for(const auto& Row:In.Ships)if(auto* Vessel=ResolveSavedVessel(Row);Vessel && !Row.bDestroyed)
    {
        auto* PreviousCarrier=Vessel->GetCarrier();
        auto* SavedCarrier=Simulation.FindShip(TCHAR_TO_UTF8(*Row.Carrier));
        if(PreviousCarrier && PreviousCarrier!=SavedCarrier)
            if(auto* Inventory=PreviousCarrier->GetHangar()){Inventory->Update(Vessel);Inventory->Ignore(Vessel);}
        if(auto* Carrier=SavedCarrier;Carrier && Row.Deck>=0 && Row.Deck<Carrier->NumFlightDecks())Vessel->SetCarrier(Carrier,Carrier->GetFlightDeck(Row.Deck));
        else if(Row.FlightPhase>static_cast<int32>(EOPSMode::ALERT))Vessel->SetCarrier(nullptr,nullptr);
        Vessel->SetFlightPhase(static_cast<EOPSMode>(FMath::Clamp(Row.FlightPhase,0,int32(EOPSMode::DOCKING))));Vessel->SetThrottle(FMath::Clamp(Row.Throttle,0.0,100.0));
        // Deck-controlled snapshots restart safely at their authored spot, using
        // the restored carrier transform and landing-gear clearance.
        if(Row.FlightPhase>=int32(EOPSMode::DOCKED) && Row.FlightPhase<=int32(EOPSMode::LOCKED))
        {
            if(auto* Deck=Vessel->GetDock();Deck && Deck->IsLaunchDeck())
            {
                bool Parked=false;
                if(auto* Carrier=Vessel->GetCarrier();Carrier && Carrier->GetHangar())
                {
                    auto* Inventory=Carrier->GetHangar();
                    if((Inventory->CanStow(Vessel) || Inventory->CreateSquadron(Text("Resumed craft"),nullptr,Vessel->Design(),1,Vessel->GetIFF(),nullptr,0,1)) && Inventory->Stow(Vessel))
                    {
                        Vessel->SetFlightPhase(EOPSMode::DOCKED);
                        Parked=Inventory->RestagePlayer(Vessel);
                    }
                }
                else Parked=DeckResumeState::Park(*Deck,*Vessel);
                if(!Parked)
                    UE_LOG(LogTemp,Warning,TEXT("[LocalState] No compatible deck spot for resumed ship '%hs'"),Vessel->GetName());
            }
        }
        else if(Row.FlightPhase==int32(EOPSMode::LAUNCH) || Row.FlightPhase==int32(EOPSMode::TAKEOFF))
            Vessel->BeginCarrierLaunchClearance();

    }
    for(const auto& Row:In.Asteroids)if(auto* Region=Simulation.FindRegion(Row.Region))
    {int32 Index=0;for(ListIter<Asteroid> A=Region->GetRoids();++A;){if(Index++!=Row.Index)continue;if(FMath::IsFinite(Row.Resource.AmountTonnes) && Row.Resource.AmountTonnes>=0){A->Resource=Row.Resource;A->InitialResourceTonnes=FMath::Max(Row.Resource.AmountTonnes,Row.InitialTonnes);A->integrity=FMath::Max(0.0,Row.Integrity);A->ContactId=Row.ContactId;A->MoveTo(Row.Position);}break;}}
    for(ListIter<MissionEvent> E=Simulation.events;++E;){const int32 I=In.EventIds.IndexOfByKey(E->EventID());if(In.EventStatuses.IsValidIndex(I))E->status=static_cast<INSTRUCTION_STATUS>(In.EventStatuses[I]);}
    for(const auto& Work:In.MiningRuns)if(auto* Vessel=Simulation.FindShip(TCHAR_TO_UTF8(*Work.Ship)))
    {auto& Run=Simulation.JanekMiningRuns.FindOrAdd(Vessel->GetIdentity());Run.Time=Work.Time;Run.NextPulse=Work.NextPulse;Run.bDelivering=Work.bDelivering;Run.LoadedTonnes=Work.LoadedTonnes;if(Work.DepositId!=INDEX_NONE)Run.DepositId=Work.DepositId;Run.Contract.RestoreReservation(Simulation.ReadEconomy(),Work.Reservation);}
    if(FMath::IsFinite(In.MissionElapsed) && In.MissionElapsed>=0 && In.MissionElapsed<7*86400)Simulation.start_time=Game::GetGameTime()-static_cast<uint32>(In.MissionElapsed*1000);
    if(ResumePilot && ResumePilot->GetRegion())
    {
        ResumePilot->GetRegion()->SetPlayerShip(ResumePilot);
        Simulation.ActivateRegion(ResumePilot->GetRegion());
        UE_LOG(LogTemp,Display,TEXT("[LocalState] Resumed pilot %hs in active sector %hs"),ResumePilot->GetName(),ResumePilot->GetRegion()->GetName());
    }
    return true;
}

