#include "TransportAI.h"
#include "EconomyRuntime.h"
#include "Engine/DataTable.h"

#include "Algo/Reverse.h"
#include "ContractAI.h"
#include "Farcaster.h"
#include "Ship.h"
#include "SimRegion.h"
#include "StarSystem.h"
#include "Sim.h"

TransportAI::TransportAI(Ship* InShip)
    : NavAI(InShip)
{
}

TArray<StarSystem*> TransportAI::FindJumpRoute(StarSystem* Origin, StarSystem* Destination,
    const TArray<StarSystem*>& Systems)
{
    if (!Origin || !Destination) return {};

    TArray<StarSystem*> Frontier{Origin};
    TMap<StarSystem*, StarSystem*> Previous;
    Previous.Add(Origin, nullptr);
    for (int32 Index = 0; Index < Frontier.Num(); ++Index)
    {
        StarSystem* Current = Frontier[Index];
        if (Current == Destination)
        {
            TArray<StarSystem*> Route;
            for (StarSystem* Step = Destination; Step; Step = Previous.FindChecked(Step))
                Route.Add(Step);
            Algo::Reverse(Route);
            return Route;
        }

        for (StarSystem* Candidate : Systems)
        {
            if (!Candidate || Previous.Contains(Candidate) || !Current->HasLinkTo(Candidate))
                continue;

            Previous.Add(Candidate, Current);
            Frontier.Add(Candidate);
        }
    }
    return {};
}

namespace
{
Farcaster* FindNextGate(SimRegion& Origin, SimRegion& Destination)
{
    TArray<SimRegion*> Frontier{&Origin};
    TMap<SimRegion*, Farcaster*> FirstGate;
    FirstGate.Add(&Origin, nullptr);
    for (int32 Index = 0; Index < Frontier.Num(); ++Index)
    {
        SimRegion* Current = Frontier[Index];
        Farcaster* First = FirstGate.FindChecked(Current);
        for (ListIter<Ship> Ships = Current->GetShips(); ++Ships;)
        {
            Ship* GateShip = Ships.value();
            if (!GateShip || GateShip->IsDead() || GateShip->IsDying()) continue;

            Farcaster* Gate = GateShip->GetFarcaster();
            const Ship* Exit = Gate ? Gate->GetDest() : nullptr;
            SimRegion* Next = Exit && !Exit->IsDead() && !Exit->IsDying() ? Exit->GetRegion() : nullptr;
            if (!Gate || !Gate->HasValidApproach() || !Exit || !Exit->GetFarcaster() ||
                !Next || FirstGate.Contains(Next)) continue;
            if (Next == &Destination) return First ? First : Gate;

            FirstGate.Add(Next, First ? First : Gate);
            Frontier.Add(Next);
        }
    }
    return nullptr;
}
}

bool TransportAI::FlyToRegion(Ship& Freighter, SimRegion& Destination, double Seconds)
{
    if(auto* Simulation=Sim::GetSim();Simulation && !Simulation->HasSimulationAuthority())return false;
    if(Freighter.IsDead() || Freighter.IsDying())return false;
    if(!FMath::IsFinite(Seconds) || Seconds<0)
    { Freighter.StopScriptedMotion();return false; }
    SimRegion* Origin = Freighter.GetRegion();
    if (Freighter.GetWarpFactor() > 1.0)
    { Freighter.SetScriptedContactSpeed(Freighter.GetVelocity().Size());return false; }
    if(Seconds==0)return Origin==&Destination;
    if (Origin == &Destination) {Freighter.StopScriptedMotion();return true;}
    if (!Origin) {Freighter.StopScriptedMotion();return false;}

    Farcaster* Gate = FindNextGate(*Origin, Destination);
    if (!Gate)
    {
        Freighter.StopScriptedMotion();
        return false;
    }

    const FVector Position = Freighter.GetLocation();
    const FVector Entry = Gate->StartPoint();
    const FVector Axis = (Entry - Gate->ApproachPoint(0)).GetSafeNormal();
    const FVector Approach = Gate->GetApproachTarget(Position);
    const FVector Target = Approach.Equals(Gate->ApproachPoint(0)) ? Approach : Entry;
    const FVector Offset = Target - Position;
    const double Distance = Offset.Size();
    if(Position.ContainsNaN() || Target.ContainsNaN() || !FMath::IsFinite(Distance))
    { Freighter.StopScriptedMotion();return false; }
    const FVector Heading = Distance > 1.0 ? Offset.GetSafeNormal() : Axis;
    Camera Frame = Freighter.GetCam();
    const FVector Forward = Frame.vpn().GetSafeNormal();
    const FQuat Turn = FQuat::FindBetweenNormals(Forward, Heading.IsNearlyZero() ? Forward : Heading);
    const double Angle = Turn.GetAngle();
    if (Angle > UE_SMALL_NUMBER)
    {
        const FQuat StepTurn = FQuat::Slerp(FQuat::Identity, Turn,
            FMath::Min(1.0, FMath::DegreesToRadians(35.0) * Seconds / Angle));
        Frame.LookAt(Position + StepTurn.RotateVector(Forward), Position, StepTurn.RotateVector(Frame.vup()));
        Freighter.CloneCam(Frame);
    }

    const double Step = FMath::Min(Distance, 350.0 * Seconds);
    if (Distance > 1.0)
    {
        Freighter.MoveTo(Position + Heading * Step);
        Freighter.SetScriptedContactSpeed(Step / Seconds);
        Freighter.SetVelocity(FVector::ZeroVector);
    }
    else
    {
        // Farcaster detects aligned forward motion on the next simulation update.
        Freighter.SetVelocity(Axis * 50.0);
        if (!Gate->CanEnter(Freighter))
            Freighter.StopScriptedMotion();
        else Freighter.SetScriptedContactSpeed(Freighter.GetVelocity().Size());
    }
    return false;
}

double TransportAI::Load(Ship& Freighter, ContractAI& Contract, double TransferLimit)
{
    const double Loaded = Freighter.LoadMinedOre(Contract.GetItem(),
        FMath::Min(Contract.GetReservedTonnes(), TransferLimit));
    Contract.CommitPickup(Loaded);
    return Loaded;
}

double TransportAI::Unload(Ship& Freighter, double& DestinationStock, double FreeSpace, double TransferLimit)
{
    const double Unloaded = Freighter.UnloadMinedOre(FMath::Min(FreeSpace, TransferLimit));
    DestinationStock += Unloaded;
    return Unloaded;
}



double TransportAI::Load(FEconomyState& State, Ship& Freighter, ContractAI& Contract, double Limit)
{
    if (!FMath::IsFinite(Limit) || Limit <= 0) return 0;
    Freighter.SyncCargoInventory();
    const double Loaded = Freighter.LoadMinedOre(Contract.GetItem(), FMath::Min(Contract.GetReservedTonnes(), Limit), false);
    if (Loaded <= 0) return 0;
    if (!FEconomyLedger::Transfer(State, FEconomyRuntime::IndustryOwner(), Contract.GetSource(), Freighter.GetCargoInventoryKey(),
        DataTableUtils::MakeValidName(Contract.GetItem()), Loaded, Contract.GetId()))
    { Freighter.UnloadMinedOre(Loaded, false); return 0; }
    Contract.CommitPickup(Loaded);
    return Loaded;
}
double TransportAI::Unload(FEconomyState& State, Ship& Freighter, FName Destination, double Limit)
{
    if (!FMath::IsFinite(Limit) || Limit <= 0 || !Freighter.GetCargoManifest()) return 0;
    Freighter.SyncCargoInventory();
    const auto* To = State.Inventories.Find(Destination);
    if (!To) return 0;
    const double Amount = FMath::Min3(Freighter.GetMinedOreTonnes(), Limit, FMath::Max(0.0, To->CapacityTonnes - FEconomyLedger::Used(*To)));
    if (Amount <= 0 || !FEconomyLedger::Transfer(State, FEconomyRuntime::IndustryOwner(), Freighter.GetCargoInventoryKey(), Destination,
        DataTableUtils::MakeValidName(Freighter.GetCargoManifest()->Item), Amount)) return 0;
    return Freighter.UnloadMinedOre(Amount, false);
}
