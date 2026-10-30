#include "QuantumTravel.h"
#include "Sim.h"
#include "Ship.h"
#include "SimRegion.h"
#include "StarSystem.h"
#include "QuantumDrive.h"
#include "Farcaster.h"
#include "Misc/PackageName.h"
#include "Game.h"

namespace QuantumTravel
{
    bool IsDestination(SimRegion* Current, SimRegion* Destination)
    {
        if (!Current || !Destination || Current == Destination || Destination->IsAirSpace() || !Current->GetSystem() || !Destination->GetSystem()) return false;
        return Current->GetSystem() == Destination->GetSystem() || Current->GetLinks().contains(Destination);
    }
    TArray<SimRegion*> GetRegions(Sim* Simulation, Ship* Pilot)
    {
        TArray<SimRegion*> Result;
        if (!Simulation || !Pilot || !Pilot->GetRegion() || !Simulation->GetRegions().contains(Pilot->GetRegion())) return Result;
        SimRegion* Current = Pilot->GetRegion();
        // Preserve the legacy ordering: sorted local space regions, then linked systems.
        List<SimRegion> Local;
        for (int Index=0; Index<Simulation->GetRegions().size(); ++Index)
        {
            SimRegion* Candidate = Simulation->GetRegions()[Index];
            if (IsDestination(Current, Candidate) && Candidate->GetSystem() == Current->GetSystem()) Local.append(Candidate);
        }
        Local.sort();
        for (int Index=0; Index<Local.size(); ++Index) Result.Add(Local[Index]);
        for (int Index=0; Index<Simulation->GetRegions().size(); ++Index)
        {
            SimRegion* Candidate = Simulation->GetRegions()[Index];
            if (IsDestination(Current, Candidate) && Candidate->GetSystem() != Current->GetSystem()) Result.Add(Candidate);
        }
        return Result;
    }
    TArray<FDestination> GetDestinations(Sim* Simulation, Ship* Pilot)
    {
        TArray<FDestination> Result;
        for (SimRegion* Region : GetRegions(Simulation, Pilot))
        {
            FDestination Destination;
            Destination.System = UTF8_TO_TCHAR(Region->GetSystem()->GetName());
            Destination.Region = UTF8_TO_TCHAR(Region->GetName());
            Destination.Label = Region->GetSystem() == Pilot->GetRegion()->GetSystem() ? Destination.Region : Destination.System + TEXT(" / ") + Destination.Region;
            Result.Add(MoveTemp(Destination));
        }
        return Result;
    }
    FString Readiness(Ship* Pilot)
    {
        if (!Pilot || Pilot->IsDead() || Pilot->IsDying() || !Pilot->GetRegion()) return TEXT("No active ship.");
        if (!Pilot->IsStarship() || !Pilot->GetQuantumDrive()) return TEXT("No onboard quantum drive. Fighters may use a Farcaster or ride a nearby carrier's quantum jump.");
        if (Pilot->GetFlightPhase() != EOPSMode::ACTIVE || Pilot->InTransition()) return TEXT("Quantum drive requires active flight.");
        if (Pilot->GetWarpFactor() > 1.0) return TEXT("Travel sequence already active.");
        QuantumDrive* Drive = Pilot->GetQuantumDrive();
        if (!Drive->IsPowerOn()) return TEXT("Quantum drive is powered off.");
        if (Drive->GetStatus() != SYSTEM_STATUS::NOMINAL) return TEXT("Quantum drive is not operational.");
        if (Drive->ActiveState() != QuantumDrive::ACTIVE_READY) return FString::Printf(TEXT("Quantum sequence active. Countdown %.1f s"), FMath::Max(0.0, Drive->JumpTime()));
        if (Drive->GetEnergy() != Drive->GetCapacity()) return FString::Printf(TEXT("Quantum drive charging: %.0f%%"), Drive->GetCapacity()>0 ? 100.0*Drive->GetEnergy()/Drive->GetCapacity() : 0.0);
        return FString();
    }
    bool Engage(Sim* Simulation, Ship* Pilot, const FString& System, const FString& Region, FString& Message)
    {
        Message = Readiness(Pilot);
        if (!Message.IsEmpty()) return false;
        for (SimRegion* Candidate : GetRegions(Simulation, Pilot))
        {
            if (!System.Equals(UTF8_TO_TCHAR(Candidate->GetSystem()->GetName()), ESearchCase::IgnoreCase) || !Region.Equals(UTF8_TO_TCHAR(Candidate->GetName()), ESearchCase::IgnoreCase)) continue;
            if (Candidate->GetSystem() != Pilot->GetRegion()->GetSystem())
            {
                const FString Package = TEXT("/Game/Maps/") + System;
                if (!FPackageName::DoesPackageExist(Package)) { Message = TEXT("Destination system map is unavailable."); return false; }
            }
            Pilot->GetQuantumDrive()->SetDestination(Candidate, FVector::ZeroVector);
            if (!Pilot->GetQuantumDrive()->Engage()) { Message = TEXT("Quantum drive rejected the jump."); return false; }
            Message = TEXT("Quantum countdown started.");
            return true;
        }
        Message = TEXT("Destination is no longer reachable from this region.");
        return false;
    }
    TArray<FGate> GetGates(Sim* Simulation, Ship* Pilot)
    {
        TArray<FGate> Result;
        SimRegion* Current = Pilot ? Pilot->GetRegion() : nullptr;
        if (!Simulation || !Current || !Simulation->GetRegions().contains(Current)) return Result;
        for (int Index=0; Index<Current->GetShips().size(); ++Index)
        {
            Ship* Gate = Current->GetShips()[Index];
            Farcaster* Drive = Gate ? Gate->GetFarcaster() : nullptr;
            if (!Drive || Gate->IsDead() || Gate->IsDying()) continue;
            FGate Row;
            Row.Name = UTF8_TO_TCHAR(Gate->GetName());
            Row.RangeKm = FVector::Distance(Pilot->GetLocation(), Gate->GetLocation()) / 1000.0;
            const Ship* Destination = Drive->GetDest();
            if (Destination && Destination->GetRegion())
                Row.Destination = FString::Printf(TEXT("%hs / %hs"), Destination->GetRegion()->GetSystem() ? Destination->GetRegion()->GetSystem()->GetName() : "--", Destination->GetRegion()->GetName());
            else Row.Destination = TEXT("Link unavailable");
            Row.Ready = Destination && Destination->GetFarcaster() && !Destination->IsDead() && !Destination->IsDying() && Drive->IsPowerOn() && Drive->GetStatus() == SYSTEM_STATUS::NOMINAL && Drive->GetEnergy() >= Drive->GetCapacity() && Drive->ActiveState() == QuantumDrive::ACTIVE_READY;
            Result.Add(MoveTemp(Row));
        }
        Result.Sort([](const FGate& A, const FGate& B){ return A.RangeKm < B.RangeKm; });
        return Result;
    }
    bool TargetGate(Sim* Simulation, Ship* Pilot, const FString& GateName, FString& Message)
    {
        if (!Simulation || !Pilot || !Pilot->GetRegion() || Pilot->IsDead() || Pilot->IsDying()) return false;
        Ship* Gate = Pilot->GetRegion()->FindShip(TCHAR_TO_UTF8(*GateName));
        if (!Gate || !Gate->GetFarcaster() || Gate->IsDead() || Gate->IsDying()) { Message = TEXT("Farcaster is no longer available."); return false; }
        Pilot->SetTarget(Gate);
        Message = TEXT("Gate targeted. Fly within 1 km of the operational gate to transit; no onboard quantum drive is required.");
        return true;
    }
    FVector FarcasterMapLocation(const FVector& SimLocation) { return OtherHand(SimLocation); }
}
