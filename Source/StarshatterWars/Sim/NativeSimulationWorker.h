#pragma once
#include "CoreMinimal.h"
#include "BoltCollisionCandidates.h"
#include "HAL/CriticalSection.h"
class FQueuedThreadPool;
class FEvent;

struct FNativeBoltQuery
{
    uint32 ObjectId=0;
    FVector Start=FVector::ZeroVector,End=FVector::ZeroVector;
};
struct FNativeCollisionInput
{
    uint64 Epoch=0,Sequence=0;
    TArray<SSWBoltCollision::FHull> Hulls;
    TArray<FNativeBoltQuery> Queries;
};
struct FNativeCollisionResult
{
    uint64 Epoch=0,Sequence=0;
    bool bOffGameThread=false;
    TArray<TArray<uint8>> Candidates;
};

// First simulation-domain migration: immutable ballistic broad phase only.
// No Ship*, Sim*, Actor, UObject, delegates or live container access cross this queue.
class STARSHATTERWARS_API FNativeSimulationWorker
{
public:
    FNativeSimulationWorker();
    ~FNativeSimulationWorker();
    bool Submit(FNativeCollisionInput&&);
    TUniquePtr<FNativeCollisionResult> Finish(); // same-frame barrier, also used before teardown
    static FNativeCollisionResult Calculate(FNativeCollisionInput&&);
    static bool Matches(const FNativeCollisionResult&,uint64 Epoch,uint64 Sequence);
    static bool WorthDispatching(int32 Hulls,int32 Queries){return Hulls>=256 && Queries>=2 && int64(Hulls)*Queries>=32768;}
private:
    class FWork;
    FQueuedThreadPool* Pool=nullptr;
    FEvent* Done=nullptr;
    FCriticalSection Mutex;
    bool bPending=false;
    TUniquePtr<FNativeCollisionResult> Result;
};
