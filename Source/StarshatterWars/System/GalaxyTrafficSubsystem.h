#pragma once
#include "Subsystems/GameInstanceSubsystem.h"
#include "Tickable.h"
#include "TimerManager.h"
#include "TrafficAI.h"
#include "EconomyAI.h"
#include "NativeEconomyWorker.h"
#include "EconomyCommandQueue.h"
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
    // Admission and persistence execute on the game thread. Only value commands reach the worker.
    bool QueueStorePurchase(FNativeStorePurchase&&,TFunction<bool(FText&)>&&,TFunction<void(bool,const FText&)>&&,FText&);
    bool QueueFleetRepair(FNativeFleetRepairCommand&&,TFunction<FCampaignFleetState*()>&&,TFunction<bool(FText&)>&&,TFunction<void(bool,const FFleetRepairStep&,const FText&)>&&,FText&);
    bool QueueMarketCommand(FNativeMarketCommand&&,TFunction<bool(FText&)>&&,TFunction<bool()>&&,TFunction<void(bool,const FText&,FGuid)>&&,FText&);
    bool IsTransactionPending()const{return ActiveCommand.IsSet() || RequestQueue.Count()>0;}
    bool CanQueueTransaction()const{return !bShuttingDown && RequestQueue.Count()<FEconomyCommandQueue::Capacity;}
    const TArray<FEconomyRequestRow>& GetTransactionQueue()const{return RequestQueue.Snapshot();}
    bool CancelQueuedTransaction(FGuid Receipt);
    FSimpleMulticastDelegate OnTransactionQueueChanged;
private:
    struct FQueuedEconomyRequest
    {
        FNativeStorePurchase Command;
        TUniquePtr<FNativeMarketCommand> Market;
        TUniquePtr<FNativeFleetRepairCommand> Repair;
        TFunction<FCampaignFleetState*()> LiveFleet;
        TFunction<void(bool,const FFleetRepairStep&,const FText&)> RepairCompleted;
        FString Profile;
        FGuid Receipt;
        FText Label;
        TFunction<bool(FText&)> Validate;
        TFunction<void(bool,const FText&)> Completed;
        TFunction<bool()> Persist;
        TFunction<void(bool,const FText&,FGuid)> MarketCompleted;
        void Complete(bool Success,const FText& Reason,FGuid Order=FGuid())
        {check(IsInGameThread());if(RepairCompleted)RepairCompleted(Success,{},Reason);if(Completed)Completed(Success,Reason);if(MarketCompleted)MarketCompleted(Success,Reason,Order);}
    };
    TMap<FGuid,FQueuedEconomyRequest> PendingCommands;
    TOptional<FQueuedEconomyRequest> ActiveCommand;
    FEconomyCommandQueue RequestQueue;
    bool bShuttingDown=false;
    uint64 EconomySessionEpoch=1;
    UFUNCTION() void ProfileLoaded(bool bSuccess);
    void PumpEconomyRequests();
    bool EnqueueEconomyRequest(FQueuedEconomyRequest&&,FText&);
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
    FString EconomyProfileToken(bool IncludeSaveRevision=true)const;
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
