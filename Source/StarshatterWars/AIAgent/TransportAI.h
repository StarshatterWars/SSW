#pragma once

#include "NavAI.h"

struct FEconomyState;
class ContractAI;
class StarSystem;
class SimRegion;

// Navigation base for hauling ore, components, and finished goods.
class TransportAI : public NavAI
{
public:
    explicit TransportAI(Ship* InShip);

    // Includes the origin and destination. An empty result means no route.
    static TArray<StarSystem*> FindJumpRoute(StarSystem* Origin, StarSystem* Destination,
        const TArray<StarSystem*>& Systems);

    static bool FlyToRegion(Ship& Freighter, SimRegion& Destination, double Seconds);

    static double Load(FEconomyState&, Ship&, ContractAI&, double TransferLimit);
    static double Unload(FEconomyState&, Ship&, FName Destination, double TransferLimit);
    static double Load(Ship& Freighter, ContractAI& Contract, double TransferLimit);
    static double Unload(Ship& Freighter, double& DestinationStock, double FreeSpace, double TransferLimit);
};



