#include "NativeEconomyWorker.h"
#include "EconomyAI.h"
#include "EconomyRuntime.h"
#include "SimDataEvents.h"
#include "HAL/ThreadingBase.h"
#include "Misc/QueuedThreadPool.h"
#include "Misc/IQueuedWork.h"
#include "Misc/ScopeLock.h"
#include "Misc/ScopeExit.h"
class FNativeEconomyWorker::FWork : public IQueuedWork
{
public:
    FWork(FNativeEconomyWorker& InOwner,FNativeEconomyInput&& In,TFunction<void()>&& Done):Owner(InOwner),Input(MoveTemp(In)),Completed(MoveTemp(Done)){}
    void DoThreadedWork()override
    {
        auto Output=MakeUnique<FNativeEconomyResult>(Calculate(MoveTemp(Input)));
        {FScopeLock Lock(&Owner.Mutex);Owner.Result=MoveTemp(Output);Owner.bBusy=false;}
        if(Completed)Completed();delete this;
    }
    void Abandon()override{{FScopeLock Lock(&Owner.Mutex);Owner.bBusy=false;}delete this;}
private:FNativeEconomyWorker& Owner;FNativeEconomyInput Input;TFunction<void()> Completed;
};
FNativeEconomyWorker::FNativeEconomyWorker()
{Pool=FQueuedThreadPool::Allocate();if(!Pool->Create(1,0,TPri_BelowNormal,TEXT("SSW Economy"))){delete Pool;Pool=nullptr;}}
FNativeEconomyWorker::~FNativeEconomyWorker(){if(Pool){Pool->Destroy();delete Pool;}}
bool FNativeEconomyWorker::IsBusy()const{FScopeLock Lock(&Mutex);return bBusy;}
bool FNativeEconomyWorker::Submit(FNativeEconomyInput&& Input,TFunction<void()> Completed)
{
    FScopeLock Lock(&Mutex);if(!Pool || bBusy || Result)return false;bBusy=true;
    Pool->AddQueuedWork(new FWork(*this,MoveTemp(Input),MoveTemp(Completed)));return true;
}
TUniquePtr<FNativeEconomyResult> FNativeEconomyWorker::TakeResult(){FScopeLock Lock(&Mutex);return MoveTemp(Result);}
FNativeEconomyResult FNativeEconomyWorker::Calculate(FNativeEconomyInput&& Input)
{
    FNativeEconomyResult Out;Out.Revision=Input.Revision;Out.Profile=MoveTemp(Input.Profile);Out.State=MoveTemp(Input.State);Out.bOffGameThread=!IsInGameThread();
    FSimDataEvents::Suppress(true);auto* Previous=ProductionEvents::Capture(&Out.Events);ON_SCOPE_EXIT{ProductionEvents::Capture(Previous);FSimDataEvents::Suppress(false);};
    auto& State=Out.State;const double Now=State.EconomyClockSeconds;
    if(!FMath::IsFinite(Now) || Now<0)return Out;
    if(State.BackgroundMineDeadlines.IsEmpty())for(const auto& Mine:Input.Mines)
    {State.BackgroundMineDeadlines.Add(Mine.Id,Now+60);State.BackgroundMineLastCycle.Add(Mine.Id,Now);State.NextBackgroundMineAt=Now+60;}
    if(Input.bInventoryChanged || (State.NextBackgroundMineAt>0 && Now>=State.NextBackgroundMineAt))
    {
        State.NextBackgroundMineAt=0;
        for(const auto& Mine:Input.Mines)
        {
            double& Due=State.BackgroundMineDeadlines.FindOrAdd(Mine.Id);double& Last=State.BackgroundMineLastCycle.FindOrAdd(Mine.Id,Now);
            const FName Hold=FEconomyRuntime::Warehouse(Mine.System.ToString(),Mine.Region.ToString(),TEXT("Ore"));
            const auto HasSpace=[&](){const auto* Inv=State.Inventories.Find(Hold);return !Inv || FEconomyLedger::Used(*Inv)<Inv->CapacityTonnes-UE_SMALL_NUMBER;};
            if(State.BackgroundOreRemaining.Contains(Mine.Id) && State.BackgroundOreRemaining.FindRef(Mine.Id)<=0){Due=0;continue;}
            if(Due<=0 && Input.bInventoryChanged && HasSpace()){Last=Now;Due=Now+60;}
            if(Due>0 && Now>=Due)
            {EconomyAI::AdvanceMine(State,Mine.Id,Hold,Mine.System,Mine.Item,Mine.Reserve,Mine.Rate,Mine.Capacity,FMath::Max(UE_SMALL_NUMBER,Now-Last));Last=Now;Due=HasSpace()?Now+60:0;}
            if(Due>0 && (State.NextBackgroundMineAt<=0 || Due<State.NextBackgroundMineAt))State.NextBackgroundMineAt=Due;
        }
    }
    EconomyAI Controller;Controller.AdvanceProduction(State,Input.Facilities,Input.Recipes);Out.NextProductionAt=Controller.ProductionDeadline();
    if(State.NextRegionalPriceAt<=0)State.NextRegionalPriceAt=Now;
    if(Now>=State.NextRegionalPriceAt && !Input.Goods.IsEmpty())
    {Controller.AdvancePlain(State,Input.Goods,FMath::Max(UE_SMALL_NUMBER,Now-State.LastRegionalPriceAt),Input.NowUtc);State.LastRegionalPriceAt=Now;State.NextRegionalPriceAt=Now+60;}
    return Out;
}
bool FNativeEconomyWorker::Commit(FNativeEconomyResult&& Result,FEconomyState& Live,uint64 Revision,const FString& Profile)
{
    check(IsInGameThread());
    if(Result.bCommitted || Result.Revision!=Revision || Result.Profile!=Profile)return false;
    Result.bCommitted=true;
    // Clock pumping is not a commercial mutation; never rewind it to a worker snapshot.
    Result.State.EconomyClockSeconds=FMath::Max(Live.EconomyClockSeconds,Result.State.EconomyClockSeconds);
    // These live-game domains are not processed by this first worker step.
    // Preserve them even when a movement/scan update did not constitute a ledger transaction.
    Result.State.MiningLeases=MoveTemp(Live.MiningLeases);
    Result.State.MiningDrones=MoveTemp(Live.MiningDrones);
    Result.State.MiningOptics=MoveTemp(Live.MiningOptics);
    Result.State.Contracts=MoveTemp(Live.Contracts);
    Result.State.MarketOrders=MoveTemp(Live.MarketOrders);
    Result.State.GroundFactories=MoveTemp(Live.GroundFactories);
    Result.State.OfflineFacilities=MoveTemp(Live.OfflineFacilities);
    Live=MoveTemp(Result.State);
    for(const auto& Event:Result.Events)ProductionEvents::Emit(Event.Facility,Event.Job,Event.Phase,Event.Reason);
    FSimDataEvents::Publish(Live);return true;
}
