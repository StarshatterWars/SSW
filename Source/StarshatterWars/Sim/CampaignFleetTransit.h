#pragma once
#include "CoreMinimal.h"
class Sim;
class SimRegion;
class Ship;
namespace CampaignFleetTransit
{
    SimRegion* EnsureRegion(Sim& Simulation,const FString& System,const FString& Region);
    TArray<Ship*> Group(Ship& Carrier);
    bool BoardCarrier(Sim& Simulation, Ship& Carrier, FText& Reason);
    bool Start(Sim& Simulation,const FString& System,const FString& Region,FText& Reason);
}
