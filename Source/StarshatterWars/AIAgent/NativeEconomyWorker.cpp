#include "NativeEconomyWorker.h"
#include "EconomyAI.h"
#include "EconomyRuntime.h"
#include "SimDataEvents.h"
#include "HAL/ThreadingBase.h"
#include "Misc/QueuedThreadPool.h"
#include "Misc/IQueuedWork.h"
#include "Misc/ScopeLock.h"
#include "Misc/ScopeExit.h"
#include "UObject/Class.h"
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
    if(Input.Market)
    {
        Out.Market=MakeUnique<FNativeMarketReply>(MarketCommandAI::Execute(Out.State,*Input.Market));return Out;
    }
    if(Input.Purchase)
    {
        auto& Command=*Input.Purchase;Out.Purchase=MakeUnique<FNativeStorePurchaseResult>();auto& Reply=*Out.Purchase;Reply.Receipt=Command.Receipt;
        Reply.SourceStored=Command.Stored;Reply.SourceSkins=Command.Skins;
        FS_PlayerGameInfo Candidate;Candidate.Economy=MoveTemp(Out.State);Candidate.StoredShipEquipment=MoveTemp(Command.Stored);Candidate.OwnedShipSkins=MoveTemp(Command.Skins);
        if(Command.Quantity>0 && Command.Offer.Price>0 && Command.Offer.Price<=MAX_int64/Command.Quantity && Command.ExpectedTotal==Command.Offer.Price*Command.Quantity)
            Reply.bSucceeded=ShipStoreAI::Purchase(Candidate,Command.Offer,Command.Quantity,Command.Dock,true,Command.Receipt,[]{return true;},Reply.Reason);
        else Reply.Reason=INVTEXT("The purchase quote is invalid or changed. No credits were charged.");
        Out.State=MoveTemp(Candidate.Economy);Reply.Stored=MoveTemp(Candidate.StoredShipEquipment);Reply.Skins=MoveTemp(Candidate.OwnedShipSkins);return Out;
    }
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
bool FNativeEconomyWorker::CommitPurchase(FNativeEconomyResult&& Result,FS_PlayerGameInfo& Live,uint64 Revision,const FString& Profile,TFunctionRef<bool()> Save,FText& Reason)
{
    check(IsInGameThread());
    if(!Result.Purchase || !Result.Purchase->bSucceeded){Reason=Result.Purchase?Result.Purchase->Reason:INVTEXT("No purchase result.");return false;}
    if(Result.bCommitted || Result.Revision!=Revision || Result.Profile!=Profile)
    {Reason=INVTEXT("Your profile or inventory changed while processing the purchase. No credits were charged; please retry.");return false;}
    bool SameStores=Live.StoredShipEquipment.Num()==Result.Purchase->SourceStored.Num();
    if(SameStores)for(int32 I=0;I<Live.StoredShipEquipment.Num();++I)
        if(!FStoredShipEquipment::StaticStruct()->CompareScriptStruct(&Live.StoredShipEquipment[I],&Result.Purchase->SourceStored[I],0)){SameStores=false;break;}
    bool SameSkins=Live.OwnedShipSkins.Num()==Result.Purchase->SourceSkins.Num();
    if(SameSkins)for(const auto Id:Live.OwnedShipSkins)if(!Result.Purchase->SourceSkins.Contains(Id)){SameSkins=false;break;}
    if(!SameStores || !SameSkins){Reason=INVTEXT("Local equipment or licenses changed during processing. Nothing was charged; please retry.");return false;}
    const auto Before=Live;FScopedSimDataChangeBatch Batch(Live.Economy);auto Reply=MoveTemp(Result.Purchase);
    if(!Commit(MoveTemp(Result),Live.Economy,Revision,Profile))return false;
    Live.StoredShipEquipment=MoveTemp(Reply->Stored);Live.OwnedShipSkins=MoveTemp(Reply->Skins);
    if(!Save()){Live=Before;Reason=INVTEXT("Could not save the purchase. Credits and inventory restored.");return false;}
    Reason=Reply->Reason;return true;
}
bool FNativeEconomyWorker::Commit(FNativeEconomyResult&& Result,FEconomyState& Live,uint64 Revision,const FString& Profile)
{
    check(IsInGameThread());
    if(Result.Purchase || Result.Market || Result.bCommitted || Result.Revision!=Revision || Result.Profile!=Profile)return false;
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
bool FNativeEconomyWorker::CommitMarket(FNativeEconomyResult&& Result,FEconomyState& Live,uint64 Revision,const FString& Profile,TFunctionRef<bool()> Save,FText& Reason)
{
    check(IsInGameThread());
    if(!Result.Market || !Result.Market->bSucceeded){Reason=Result.Market?Result.Market->Reason:INVTEXT("No market result.");return false;}
    if(Result.bCommitted || Result.Revision!=Revision || Result.Profile!=Profile)
    {Reason=INVTEXT("Market, profile or inventory changed during processing. Nothing exchanged; please retry.");return false;}
    bool Same=Live.MarketOrders.Num()==Result.Market->SourceOrders.Num();
    if(Same)for(const auto& Pair:Live.MarketOrders)
    {
        const auto* Source=Result.Market->SourceOrders.Find(Pair.Key);
        if(!Source || !FMarketOrder::StaticStruct()->CompareScriptStruct(&Pair.Value,Source,0)){Same=false;break;}
    }
    if(!Same){Reason=INVTEXT("The order book changed while processing. Nothing exchanged; please retry.");return false;}
    const auto Before=Live;FScopedSimDataChangeBatch Batch(Live);auto Reply=MoveTemp(Result.Market);auto Orders=MoveTemp(Result.State.MarketOrders);
    if(!Commit(MoveTemp(Result),Live,Revision,Profile))return false;
    Live.MarketOrders=MoveTemp(Orders);
    if(!Save()){Live=Before;Reason=INVTEXT("Could not save the market change. Cargo, credits, escrow and reservations restored.");return false;}
    Reason=Reply->Reason;return true;
}
