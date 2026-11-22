#include "CampaignFleetTransit.h"
#include "Sim.h"
#include "SimRegion.h"
#include "Ship.h"
#include "SimElement.h"
#include "QuantumDrive.h"
#include "StarSystem.h"
#include "OrbitalRegion.h"
#include "Orbital.h"
#include "Mission.h"
#include "Game.h"
#include "StarshatterEnvironmentSubsystem.h"
#include "FlightDeck.h"
#include "Hangar.h"
#include "DeckResumeState.h"

namespace CampaignFleetTransit
{
SimRegion* EnsureRegion(Sim& Simulation,const FString& System,const FString& Region)
{
    for(ListIter<SimRegion> R=Simulation.GetRegions();++R;)
        if(R->GetSystem() && System.Equals(UTF8_TO_TCHAR(R->GetSystem()->GetName()),ESearchCase::IgnoreCase) &&
            Region.Equals(UTF8_TO_TCHAR(R->GetName()),ESearchCase::IgnoreCase))return R.value();
    auto* Environment=UStarshatterEnvironmentSubsystem::Get();
    if(!Environment)return nullptr;
    for(auto* World:Environment->GetRuntimeStarSystems())
        if(World && System.Equals(UTF8_TO_TCHAR(World->GetName()),ESearchCase::IgnoreCase))
            if(auto* Meta=World->FindRegion(TCHAR_TO_UTF8(*Region)))
            {
                auto* Result=new SimRegion(&Simulation,Meta);
                Simulation.GetRegions().append(Result);
                return Result;
            }
    return nullptr;
}

TArray<Ship*> Group(Ship& Carrier)
{
    TArray<Ship*> Result;Result.Add(&Carrier);
    auto* Source=Carrier.GetRegion();auto* Element=Carrier.GetElement();
    if(!Source || !Element)return Result;
    const auto* Command=Element->GetCommander();
    for(ListIter<Ship> S=Source->GetShips();++S;)
    {
        Ship* Candidate=S.value();
        if(!Candidate || Candidate==&Carrier || Candidate->IsStatic() || !Candidate->IsStarship() ||
            Candidate->IsDead() || Candidate->IsDying() || Candidate->GetIFF()!=Carrier.GetIFF())continue;
        const auto* Flight=Candidate->GetElement();
        if(Flight && (Flight==Element || Flight->GetCommander()==Element ||
            (Command && (Flight==Command || Flight->GetCommander()==Command))))Result.Add(Candidate);
    }
    return Result;
}

bool BoardCarrier(Sim& Simulation,Ship& Carrier,FText& Reason)
{
    Ship* Pilot=Simulation.GetPlayerShip();
    Ship* Base=Pilot?Pilot->GetCarrier():nullptr;
    SimRegion* Destination=Carrier.GetRegion();
    if(!Pilot || !Base || !Pilot->Design() || Pilot->GetFlightPhase()>EOPSMode::ALERT ||
        Carrier.IsDead() || Carrier.IsDying() || !Destination ||
        !Simulation.GetRegions().contains(Destination) || Pilot->GetIFF()!=Carrier.GetIFF())
    {Reason=INVTEXT("Dock before boarding the training shuttle.");return false;}
    if(Base==&Carrier){Reason=INVTEXT("You are already at the shuttle destination.");return true;}
    Carrier.InitializeRuntimeHangarFromDesign();
    auto* Inventory=Carrier.GetHangar();
    bool HasDeck=false;
    for(ListIter<FlightDeck> Deck=Carrier.GetFlightDecks();++Deck;)
        if(Deck->IsLaunchDeck() && Deck->SpaceLeft(static_cast<int>(Pilot->GetClassification()))>0){HasDeck=true;break;}
    if(!Inventory || !HasDeck)
    {Reason=INVTEXT("Orion has no clear arrival bay. Remain at Janek Station and retry.");return false;}
    // Reserve an empty inventory slot for this existing craft; do not manufacture a spare.
    if(!Inventory->CanStow(Pilot) && !Inventory->CreateSquadron(Text("Training Return"),nullptr,
        Pilot->Design(),1,Pilot->GetIFF(),nullptr,0,1))
    {Reason=INVTEXT("Orion cannot receive the return transfer yet.");return false;}
    if(!Inventory->Stow(Pilot))
    {Reason=INVTEXT("Orion could not reserve the arrival berth. You remain at Janek Station.");return false;}
    auto* PreviousDeck=Pilot->GetDock();
    const auto PreviousPhase=Pilot->GetFlightPhase();
    Pilot->SetCarrier(&Carrier,nullptr);Pilot->SetFlightPhase(EOPSMode::DOCKED);
    if(!Inventory->RestagePlayer(Pilot))
    {
        Inventory->Update(Pilot);
        Inventory->Ignore(Pilot);
        Pilot->SetCarrier(Base,PreviousDeck);Pilot->SetFlightPhase(PreviousPhase);
        Reason=INVTEXT("Orion could not stage the return transfer. Retry from Janek Station.");return false;
    }
    for(ListIter<FlightDeck> Deck=Base->GetFlightDecks();++Deck;)DeckResumeState::Release(*Deck,*Pilot);
    if(auto* OldInventory=Base->GetHangar())
    {
        OldInventory->Update(Pilot);
        OldInventory->Ignore(Pilot);
    }
    Destination->TransferObject(Pilot);
    Pilot->SetThrottle(0);Pilot->SetVelocity(Carrier.GetVelocity());Pilot->ClearTrack();
    if(auto* Element=Pilot->GetElement())Element->SetCarrier(&Carrier);
    Simulation.ActivateRegion(Destination);
    Reason=FText::FromString(FString::Printf(TEXT("Shuttle arrival at %hs. Your active training fighter accompanies you; other stored craft remain at their bases."),Carrier.GetName()));
    UE_LOG(LogTemp,Display,TEXT("[CampaignTransit] Return shuttle: %hs -> %hs, pilot %hs"),Base->GetName(),Carrier.GetName(),Pilot->GetName());
    return true;
}

bool Start(Sim& Simulation,const FString& System,const FString& Region,FText& Reason)
{
    Ship* Pilot=Simulation.GetPlayerShip();
    Ship* Carrier=Pilot?Pilot->GetCarrier():nullptr;
    if(!Pilot || !Carrier || Carrier->IsStatic() || !Carrier->GetRegion() || Pilot->GetFlightPhase()>EOPSMode::ALERT)
    {Reason=INVTEXT("Return aboard Orion before beginning fleet deployment.");return false;}
    SimRegion* Current=Carrier->GetRegion();
    if(Current->GetSystem() && System.Equals(UTF8_TO_TCHAR(Current->GetSystem()->GetName()),ESearchCase::IgnoreCase) &&
        Region.Equals(UTF8_TO_TCHAR(Current->GetName()),ESearchCase::IgnoreCase))
    {Reason=INVTEXT("Your group is already at its deployment sector.");return true;}
    SimRegion* Destination=EnsureRegion(Simulation,System,Region);
    auto* Meta=Destination?Destination->GetOrbitalRegion():nullptr;
    if(!Meta || !Meta->Primary() || Meta->GetType()==Orbital::TERRAIN)
    {Reason=INVTEXT("The fleet destination is unavailable. Deployment orders remain accepted.");return false;}
    const auto Members=Group(*Carrier);
    for(auto* Member:Members)
        if(!Member->GetQuantumDrive() || Member->GetQuantumDrive()->ActiveState()!=QuantumDrive::ACTIVE_READY ||
            !Member->GetQuantumDrive()->IsPowerOn() || Member->GetQuantumDrive()->GetStatus()!=SYSTEM_STATUS::NOMINAL ||
            Member->GetQuantumDrive()->GetEnergy()!=Member->GetQuantumDrive()->GetCapacity())
        {Reason=INVTEXT("A group quantum drive is busy or unavailable. Remain aboard and retry deployment.");return false;}
    FVector Outward=(Meta->Location()-Meta->Primary()->Location()).GetSafeNormal();
    if(Outward.IsNearlyZero())Outward=FVector::ForwardVector;
    const FVector Arrival=Outward*200000.0;
    TArray<QuantumDrive*> Started;
    for(auto* Member:Members)
    {
        auto* Drive=Member->GetQuantumDrive();
        Drive->SetPointDestination(Destination,Arrival+OtherHand(Member->GetLocation()-Carrier->GetLocation()));
        if(!Drive->Engage())
        {
            for(auto* Prior:Started)Prior->CancelJump();
            Reason=INVTEXT("Fleet drives are charging. Deployment orders remain accepted; retry when ready.");return false;
        }
        Started.Add(Drive);
    }
    for(auto* Member:Members){Member->SetThrottle(0);Member->SetVelocity(FVector::ZeroVector);Member->ArcadeStop();}
    Reason=FText::FromString(FString::Printf(TEXT("Orion and its group are deploying to %s, %s. Remain aboard until the jump completes."),*Region,*System));
    UE_LOG(LogTemp,Display,TEXT("[CampaignTransit] %d group ships departing for %s / %s"),Members.Num(),*System,*Region);
    return true;
}
}
