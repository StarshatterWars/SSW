#pragma once
#include "Subsystems/GameInstanceSubsystem.h"
#include "Tickable.h"
#include "TrafficAI.h"
#include "GalaxyTrafficSubsystem.generated.h"

class Ship;class Sim;class SimRegion;class Mission;
UCLASS()
class STARSHATTERWARS_API UGalaxyTrafficSubsystem:public UGameInstanceSubsystem,public FTickableGameObject
{
    GENERATED_BODY()
public:
    void Initialize(FSubsystemCollectionBase&)override;
    void Deinitialize()override;
    void Tick(float DeltaSeconds)override;
    bool IsTickable()const override{return !IsTemplate() && GetGameInstance()!=nullptr;}
    UWorld* GetTickableGameObjectWorld()const override{return GetWorld();}
    TStatId GetStatId()const override{RETURN_QUICK_DECLARE_CYCLE_STAT(UGalaxyTrafficSubsystem,STATGROUP_Tickables);}
    int32 GetCoveredSectorCount()const{return Sectors.Num();}
    int32 GetRealizedShipCount()const{return LiveShips.Num();}
private:
    void BuildGraph(Sim&);
    void UpdateLive(Sim&,FGalaxyTrafficState&,FEconomyState&,double Seconds);
    void Realize(Sim&,FGalaxyTrafficJourney&,FEconomyState&);
    void Despawn(Sim&,FGuid);
    FName SectorKey(SimRegion*)const;
    FVector Goal(Sim&,const FGalaxyTrafficEndpoint&,Ship&)const;
    TMap<FName,FGalaxyTrafficSector> Sectors;
    TMap<FName,FGalaxyTrafficEndpoint> Endpoints;
    TMap<FGuid,FString> LiveShips;
    TMap<FGuid,FVector> LiveGoals;
    const Mission* LastMission=nullptr;
    double Accumulator=0,ScheduleElapsed=0;
    double CheckpointElapsed=0,PersistenceRetry=0;
    bool bPersistenceBlocked=false;
    int32 ActiveBudget=4,MaxJourneys=96;
    double ScheduleSeconds=8,HopSeconds=90,TransferTonnesPerSecond=1;
};
