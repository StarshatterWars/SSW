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
    bPending=true;Done->Reset();Pool->AddQueuedWork(new FWork(*this,MoveTemp(Input)));return true;
}
TUniquePtr<FNativeCollisionResult> FNativeSimulationWorker::Finish()
{
    {FScopeLock Lock(&Mutex);if(!bPending)return nullptr;}
    TRACE_CPUPROFILER_EVENT_SCOPE(SSW_SimulationWorkerBarrier);
    Done->Wait();FScopeLock Lock(&Mutex);bPending=false;return MoveTemp(Result);
}
FNativeCollisionResult FNativeSimulationWorker::Calculate(FNativeCollisionInput&& Input)
{
    TRACE_CPUPROFILER_EVENT_SCOPE(SSW_SimulationWorkerBroadPhase);
    FNativeCollisionResult Out;Out.Epoch=Input.Epoch;Out.Sequence=Input.Sequence;Out.bOffGameThread=!IsInGameThread();Out.Candidates.SetNum(Input.Queries.Num());
    for(int32 I=0;I<Input.Queries.Num();++I)
        SSWBoltCollision::FindCandidates(Input.Hulls,Input.Queries[I].Start,Input.Queries[I].End,Out.Candidates[I],false);
    return Out;
}
bool FNativeSimulationWorker::Matches(const FNativeCollisionResult& Out,uint64 Epoch,uint64 Sequence)
{return Out.Epoch==Epoch && Out.Sequence==Sequence && Epoch!=0 && Sequence!=0;}
