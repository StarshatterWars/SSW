#include "JumpRouteReadout.h"
#include "Sim.h"
#include "Ship.h"
#include "SimRegion.h"
#include "StarSystem.h"
#include "Farcaster.h"
#include "QuantumDrive.h"
#include "Instruction.h"
#include "TransportAI.h"
#include "StarshatterEnvironmentSubsystem.h"

TArray<FString> FJumpRouteModel::RemainingRoute(const TArray<FString>& Route, const FString& Current)
{
    TArray<FString> Result;
    const int32 Index = Route.IndexOfByPredicate([&](const FString& Name) { return Name.Equals(Current, ESearchCase::IgnoreCase); });
    if (Index != INDEX_NONE) Result.Append(Route.GetData()+Index, Route.Num()-Index);
    return Result;
}
FJumpRouteReadout FJumpRouteModel::Build(Sim& Simulation, Ship& Player, const TArray<FString>& Selected)
{
    FJumpRouteReadout Result;
    auto* CurrentRegion = Player.GetRegion(); auto* CurrentSystem = CurrentRegion ? CurrentRegion->GetSystem() : nullptr;
    if (!CurrentSystem) return Result;
    const FString Current = UTF8_TO_TCHAR(CurrentSystem->GetName());
    Result.Systems = RemainingRoute(Selected, Current);
    if (Result.Systems.IsEmpty() && !Selected.IsEmpty())
    {
        const auto* Environment = UStarshatterEnvironmentSubsystem::Get();
        if (Environment)
            for (auto* Destination : Environment->GetRuntimeStarSystems())
                if (Destination && Selected.Last().Equals(UTF8_TO_TCHAR(Destination->GetName()), ESearchCase::IgnoreCase))
                    for (auto* Step : TransportAI::FindJumpRoute(CurrentSystem, Destination, Environment->GetRuntimeStarSystems()))
                        Result.Systems.Add(UTF8_TO_TCHAR(Step->GetName()));
    }
    if (Result.Systems.IsEmpty())
    {
        Result.Systems.Add(Current);
        for (ListIter<Instruction> Points = Player.GetFlightPlan(); ++Points;)
            if (Points.value() && Points->GetStatus() <= INSTRUCTION_STATUS::ACTIVE && Points->GetRegion() && Points->GetRegion()->GetSystem())
            {
                const FString Next = UTF8_TO_TCHAR(Points->GetRegion()->GetSystem()->GetName());
                if (!Result.Systems.Last().Equals(Next, ESearchCase::IgnoreCase)) Result.Systems.Add(Next);
            }
    }
    const Ship* Gate = nullptr;
    Ship* Tracked = Player.GetAutoTrackTarget();
    if (!Tracked && Player.GetTarget() && Player.GetTarget()->GetType() == ESimObject::SHIP) Tracked = static_cast<Ship*>(Player.GetTarget());
    if (Tracked && Tracked->GetFarcaster() && !Tracked->IsDead() && !Tracked->IsDying()) Gate = Tracked;
    if (!Gate && Result.Systems.Num() > 1)
    {
        double Best = TNumericLimits<double>::Max();
        for (ListIter<SimRegion> Regions = Simulation.GetRegions(); ++Regions;)
        {
            auto* Region = Regions.value(); if (!Region || Region->GetSystem() != CurrentSystem) continue;
            for (ListIter<Ship> Ships = Region->GetShips(); ++Ships;)
            {
                auto* Candidate = Ships.value(); auto* Device = Candidate ? Candidate->GetFarcaster() : nullptr;
                const auto* Exit = Device ? Device->GetDest() : nullptr;
                if (!Candidate || Candidate->IsDead() || Candidate->IsDying() || !Exit || !Exit->GetRegion() || !Exit->GetRegion()->GetSystem() ||
                    !Result.Systems[1].Equals(UTF8_TO_TCHAR(Exit->GetRegion()->GetSystem()->GetName()), ESearchCase::IgnoreCase)) continue;
                // Positions from different region frames are not comparable distances.
                const double Score = Region == CurrentRegion ? FVector::DistSquared(Player.GetLocation(),Device->StartPoint()) : 1.e30;
                if (!Gate || Score < Best) { Best = Score; Gate = Candidate; }
            }
        }
    }
    if (Gate && Gate->GetFarcaster())
    {
        const auto* Device = Gate->GetFarcaster(); const auto* Exit = Device->GetDest();
        Result.Gate = UTF8_TO_TCHAR(Gate->GetName());
        Result.Region = Gate->GetRegion() ? UTF8_TO_TCHAR(Gate->GetRegion()->GetName()) : TEXT("");
        if (Exit && Exit->GetRegion() && Exit->GetRegion()->GetSystem() && Result.Systems.Num() <= 1)
            Result.Systems.Add(UTF8_TO_TCHAR(Exit->GetRegion()->GetSystem()->GetName()));
        Result.EntryPoint = Device->StartPoint(); Result.bHasEntryPoint = Gate->GetRegion() == CurrentRegion;
        if (Result.bHasEntryPoint) Result.Range = FVector::Dist(Player.GetLocation(),Result.EntryPoint);
        if (!Exit || Exit->IsDead() || Exit->IsDying() || !Device->IsPowerOn()) Result.Status = TEXT("GATE UNAVAILABLE");
        else if (Device->ActiveState() == QuantumDrive::ACTIVE_POSTWARP) Result.Status = TEXT("ARRIVAL / GATE CYCLING");
        else if (Device->ActiveState() != QuantumDrive::ACTIVE_READY) Result.Status = TEXT("TRANSIT / GATE CYCLING");
        else if (!Result.bHasEntryPoint) Result.Status = TEXT("NAVIGATE TO GATE REGION");
        else if (Device->CanEnter(Player)) Result.Status = TEXT("READY TO TRANSIT");
        else Result.Status = TEXT("APPROACH AND ALIGN");
    }
    else if (!Selected.IsEmpty() && Selected.Last().Equals(Current, ESearchCase::IgnoreCase))
    { Result.bArrived = true; Result.Status = TEXT("DESTINATION REACHED"); }
    else if (Result.Systems.Num() > 1) Result.Status = TEXT("NO LIVE GATE TO NEXT SYSTEM");
    return Result;
}
