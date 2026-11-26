#pragma once
#include "Subsystems/GameInstanceSubsystem.h"
#include "Tickable.h"
#include "TimerManager.h"
#include "TrafficAI.h"
#include "EconomyAI.h"
#include "NativeEconomyWorker.h"
#include "GeneratedShipNames.h"
#include "GalaxyTrafficSubsystem.generated.h"

class Ship;class Sim;class SimRegion;class Mission;
struct FFractalVerseWorld;
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
    bool LoadEconomyCatalogs();
    void AdvanceServerWorld(FFractalVerseWorld&, double Seconds);
    int32 GetFacilityCount()const{return Facilities.Num();}
    int32 GetBackgroundMineCount()const{return Mines.Num();}
    int32 GetCoveredSectorCount()const{return Sectors.Num();}
    int32 GetRealizedShipCount()const{return LiveShips.Num();}
private:
    struct FBackgroundMine { FName Id,System,Region,Item;double Reserve=0,Rate=0,Capacity=0; };
    void TickEconomy(FEconomyState&,double Seconds,int64 Now);
    void DetachLive();
    void GroundBuildingRetired(const Ship&,const SimRegion&);
    FDelegateHandle ProductionChangeHandle;
    void RunManagement();
    void AdvanceManagement(float Seconds);
    void ScheduleManagement(double Delay=0);
    FTimerHandle ManagementTimer;
    double LastManagementTime=0;
    bool bManagementRunning=false,bManagementWake=false,bEconomyChanged=true;
    TSet<FGuid> LiveArrivals;
    const FEconomyState* BoundEconomy=nullptr;
    int32 GroundFactoryCount=-1;
    TUniquePtr<FNativeEconomyWorker> EconomyWorker;
    uint64 EconomyMutationRevision=0;
    double WorkerProductionAt=0;
    bool bApplyingWorkerResult=false,bWorkerInitialized=false;
    void EconomyWorkerCompleted();
    FString EconomyProfileToken()const;
    void BuildGraph(Sim&);
    void UpdateLive(Sim&,FGalaxyTrafficState&,FEconomyState&,double Seconds);
    void Realize(Sim&,FGalaxyTrafficJourney&,FEconomyState&);
    void Despawn(Sim&,FGuid);
    FName SectorKey(SimRegion*)const;
    FVector Goal(Sim&,const FGalaxyTrafficEndpoint&,Ship&)const;
    EconomyAI EconomyController;
    TMap<FName,FSSWProductionFacility> Facilities;
    TMap<FName,FSSWProductionRecipe> Recipes;
    TArray<FBackgroundMine> Mines;
    TArray<FPatrolHull> PatrolHulls;
    int32 PatrolBudget=3;
    int32 MaximumPatrols=48;
    bool bCatalogsLoaded=false;
    bool bAuthorityGraphReady=false;
    double EconomyAccumulator=0;
    TMap<FName,FGalaxyTrafficSector> Sectors;
    TMap<FName,FGalaxyTrafficEndpoint> Endpoints;
    TMap<FGuid,FString> LiveShips;
    TMap<FGuid,FVector> LiveGoals;
    const Mission* LastMission=nullptr;
    double Accumulator=0,ScheduleElapsed=0;
    double CheckpointElapsed=0,PersistenceRetry=0;
    bool bPersistenceBlocked=false;
    int32 ActiveBudget=4,MaxJourneys=96;
    TUniquePtr<class FGeneratedShipNames> GeneratedNames;
    double ScheduleSeconds=8,HopSeconds=90,TransferTonnesPerSecond=1;
};
