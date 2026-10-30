#pragma once
#include "CoreMinimal.h"
class Sim;
class Ship;
class SimRegion;

namespace QuantumTravel
{
    struct FDestination { FString System; FString Region; FString Label; };
    struct FGate { FString Name; FString Destination; double RangeKm = 0; bool Ready = false; };
    bool IsDestination(SimRegion* Current, SimRegion* Destination);
    TArray<SimRegion*> GetRegions(Sim* Simulation, Ship* Pilot);
    TArray<FDestination> GetDestinations(Sim* Simulation, Ship* Pilot);
    TArray<FGate> GetGates(Sim* Simulation, Ship* Pilot);
    FString Readiness(Ship* Pilot);
    bool Engage(Sim* Simulation, Ship* Pilot, const FString& System, const FString& Region, FString& Message);
    bool TargetGate(Sim* Simulation, Ship* Pilot, const FString& GateName, FString& Message);
    FVector FarcasterMapLocation(const FVector& SimLocation);
}
