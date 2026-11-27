#include "NativeSimulationWorker.h"
#include "HAL/PlatformProcess.h"
#include "HAL/Event.h"
#include "Misc/QueuedThreadPool.h"
#include "Misc/IQueuedWork.h"
#include "Misc/ScopeLock.h"
#include "ProfilingDebugging/CpuProfilerTrace.h"

class FNativeSimulationWorker::FWork : public IQueuedWork
{
public:
    FWork(FNativeSimulationWorker& InOwner,FNativeCollisionInput&& In):Owner(InOwner),Input(MoveTemp(In)){}
    void DoThreadedWork()override
    {
        auto Out=MakeUnique<FNativeCollisionResult>(Calculate(MoveTemp(Input)));
        {FScopeLock Lock(&Owner.Mutex);Owner.Result=MoveTemp(Out);}Owner.Done->Trigger();delete this;
    }
    void Abandon()override{Owner.Done->Trigger();delete this;}
private:FNativeSimulationWorker& Owner;FNativeCollisionInput Input;
};
class FNativeSimulationWorker::FFlightWork : public IQueuedWork
{
public:
    FFlightWork(FNativeSimulationWorker& InOwner,FNativeFlightRequest&& In):Owner(InOwner),Input(MoveTemp(In)){}
    void DoThreadedWork()override
    {
        auto Out=MakeUnique<FNativeFlightResult>(SSWFlightPhysics::Calculate(MoveTemp(Input)));
        {FScopeLock Lock(&Owner.Mutex);Owner.FlightResult=MoveTemp(Out);}Owner.Done->Trigger();delete this;
    }
    void Abandon()override{Owner.Done->Trigger();delete this;}
private:FNativeSimulationWorker& Owner;FNativeFlightRequest Input;
};
FNativeSimulationWorker::FNativeSimulationWorker()
{
    Done=FPlatformProcess::GetSynchEventFromPool(true);Pool=FQueuedThreadPool::Allocate();
    if(!Pool->Create(1,0,TPri_Normal,TEXT("SSW Simulation"))){delete Pool;Pool=nullptr;}
}
FNativeSimulationWorker::~FNativeSimulationWorker()
{if(Pool){Pool->Destroy();delete Pool;}FPlatformProcess::ReturnSynchEventToPool(Done);}
bool FNativeSimulationWorker::Submit(FNativeCollisionInput&& Input)
{
    FScopeLock Lock(&Mutex);if(!Pool || bPending || Input.Epoch==0 || Input.Sequence==0)return false;
    bPending=true;WorkKind=EWorkKind::Collision;Done->Reset();Pool->AddQueuedWork(new FWork(*this,MoveTemp(Input)));return true;
}
TUniquePtr<FNativeCollisionResult> FNativeSimulationWorker::Finish()
{
    {FScopeLock Lock(&Mutex);if(!bPending || WorkKind!=EWorkKind::Collision)return nullptr;}
    TRACE_CPUPROFILER_EVENT_SCOPE(SSW_SimulationWorkerBarrier);
    Done->Wait();FScopeLock Lock(&Mutex);bPending=false;WorkKind=EWorkKind::None;return MoveTemp(Result);
}
bool FNativeSimulationWorker::SubmitFlight(FNativeFlightRequest&& Input)
{
    FScopeLock Lock(&Mutex);if(!Pool || bPending || Input.Sequence==0)return false;
    bPending=true;WorkKind=EWorkKind::Flight;Done->Reset();Pool->AddQueuedWork(new FFlightWork(*this,MoveTemp(Input)));return true;
}
TUniquePtr<FNativeFlightResult> FNativeSimulationWorker::FinishFlight()
{
    {FScopeLock Lock(&Mutex);if(!bPending || WorkKind!=EWorkKind::Flight)return nullptr;}
    TRACE_CPUPROFILER_EVENT_SCOPE(SSW_FlightPhysicsBarrier);
    Done->Wait();FScopeLock Lock(&Mutex);bPending=false;WorkKind=EWorkKind::None;return MoveTemp(FlightResult);
}
FNativeCollisionResult FNativeSimulationWorker::Calculate(FNativeCollisionInput&& Input)
{
    TRACE_CPUPROFILER_EVENT_SCOPE(SSW_SimulationWorkerBroadPhase);
    const double Started=FPlatformTime::Seconds();
    FNativeCollisionResult Out;Out.Epoch=Input.Epoch;Out.Sequence=Input.Sequence;Out.bOffGameThread=!IsInGameThread();Out.Candidates.SetNum(Input.Queries.Num());
    for(int32 I=0;I<Input.Queries.Num();++I)
        SSWBoltCollision::FindCandidates(Input.Hulls,Input.Queries[I].Start,Input.Queries[I].End,Out.Candidates[I],false);
    if(Input.bShipPairs && WorthShipPairs(Input.Hulls.Num()))
    {
        TRACE_CPUPROFILER_EVENT_SCOPE(SSW_SimulationWorkerShipPairs);
        const int32 N=Input.Hulls.Num();Out.ShipPairs.SetNumZeroed(N*N);
        for(int32 A=0;A<N;++A)for(int32 B=A+1;B<N;++B)
            Out.ShipPairs[A*N+B]=!(FVector::Dist(Input.Hulls[A].Position,Input.Hulls[B].Position)>Input.Hulls[A].Radius+Input.Hulls[B].Radius);
    }
    Out.CalculationSeconds=FPlatformTime::Seconds()-Started;
    return Out;
}
bool FNativeSimulationWorker::MatchesGeometry(const FNativeCollisionResult& Out,TConstArrayView<SSWBoltCollision::FHull> Snapshot,TConstArrayView<SSWBoltCollision::FHull> Live)
{
    const int32 N=Snapshot.Num();if(N!=Live.Num() || int64(Out.ShipPairs.Num())!=int64(N)*N || N==0)return false;
    for(int32 I=0;I<N;++I)if(Snapshot[I].Position!=Live[I].Position || Snapshot[I].Radius!=Live[I].Radius)return false;
    return true;
}
bool FNativeSimulationWorker::MayCollide(const FNativeCollisionResult& Out,TConstArrayView<SSWBoltCollision::FHull> Snapshot,
    int32 A,int32 B,const SSWBoltCollision::FHull& LiveA,const SSWBoltCollision::FHull& LiveB)
{
    const int32 N=Snapshot.Num();
    if(A<0 || B<0 || A>=N || B>=N || A==B || int64(Out.ShipPairs.Num())!=int64(N)*N)return true;
    // Explosion, docking or an earlier elastic collision can invalidate the snapshot.
    // Unknown/non-finite/changed geometry always falls back to the existing narrow phase.
    if(LiveA.Position!=Snapshot[A].Position || LiveA.Radius!=Snapshot[A].Radius ||
       LiveB.Position!=Snapshot[B].Position || LiveB.Radius!=Snapshot[B].Radius)return true;
    if(A>B)Swap(A,B);
    return Out.ShipPairs[A*N+B]!=0;
}
bool FNativeSimulationWorker::Matches(const FNativeCollisionResult& Out,uint64 Epoch,uint64 Sequence)
{return Out.Epoch==Epoch && Out.Sequence==Sequence && Epoch!=0 && Sequence!=0;}
