#pragma once
#include "GameStructs_traffic.h"
#include "GameStructs_economy.h"

struct FPatrolHull {FName Design;int32 IFF=0;};
struct FGalaxyTrafficEndpoint
{
    FName Id,Sector,System,Region,Station;
    FVector Location=FVector::ZeroVector;
    bool bSurface=false,bPhysical=false;
};
struct FGalaxyTrafficSector
{
    FName Id,System,Region;
    bool bSurface=false;
    int32 Affiliation=0;
    TArray<FName> Neighbors;
};
class TrafficAI
{
public:
    static bool PatrolAllowed(bool FreeFlight,bool Training,bool Networked,bool Cinematic);
    static FGuid SchedulePatrol(FGalaxyTrafficState&,const FGalaxyTrafficSector&,const TArray<FPatrolHull>&,int32 MaximumPatrols);
    static void Dispatch(FGalaxyTrafficState&,FEconomyState&,const TMap<FName,FGalaxyTrafficSector>&,
        const TMap<FName,FGalaxyTrafficEndpoint>&,FName ActiveSector,int32 MaximumJourneys);
    static void TickLegacyIndustry(class Sim&,double Seconds);
    static TArray<FName> Route(const TMap<FName,FGalaxyTrafficSector>& Sectors,FName From,FName To);
    static FGuid Schedule(FGalaxyTrafficState&,FEconomyState&,const FGalaxyTrafficEndpoint& From,
        const FGalaxyTrafficEndpoint& To,const TArray<FName>& Route,FName Source=NAME_None,FName Destination=NAME_None,
        FName Item=NAME_None,double Quantity=0,double UnitTonnes=1);
    // bRealized forbids time-only arrival. The world adapter must confirm arrival.
    static bool Advance(FGalaxyTrafficJourney&,FEconomyState&,double Seconds,bool bRealized,bool bArrived,
        double HopSeconds,double TonnesPerSecond);
    static int32 RecoverBlocked(FGalaxyTrafficState&,FEconomyState&,const TMap<FName,FGalaxyTrafficSector>&,const TMap<FName,FGalaxyTrafficEndpoint>&);
    static bool AbstractCargo(FGalaxyTrafficJourney&,FEconomyState&,FName System);
    static void Lose(FGalaxyTrafficJourney&,FEconomyState&);
    static bool Eligible(bool Authority,bool Playing,bool Cinematic,bool Exporting);
    static bool SafeAnchor(const FVector& Candidate,double Radius,const TArray<TPair<FVector,double>>& Obstacles);
private:
    static const FGalaxyTrafficEndpoint* EndpointForInventory(const TMap<FName,FGalaxyTrafficEndpoint>&,FName,const FEconomyInventory&);
    static void ScheduleSector(FGalaxyTrafficState&,FEconomyState&,const TMap<FName,FGalaxyTrafficSector>&,
        const TMap<FName,FGalaxyTrafficEndpoint>&,FName Sector);
};
