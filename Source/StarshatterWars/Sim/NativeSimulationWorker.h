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
    bool bShipPairs=false;
};
struct FNativeCollisionResult
{
    uint64 Epoch=0,Sequence=0;
    bool bOffGameThread=false;
    TArray<TArray<uint8>> Candidates;
    TArray<uint8> ShipPairs; // upper triangle in a bounded square matrix
    double CalculationSeconds=0;
};

// Immutable ballistic and ship-sphere broad phases; damage and narrow phase stay live.
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
    static bool WorthShipPairs(int32 Hulls){return Hulls>=256 && Hulls<=2048;}
    static bool MatchesGeometry(const FNativeCollisionResult&,TConstArrayView<SSWBoltCollision::FHull> Snapshot,TConstArrayView<SSWBoltCollision::FHull> Live);
    // Caller validates the complete layout once, and invalidates after any live collision.
    static bool PreparedPair(const FNativeCollisionResult& Out,int32 N,int32 A,int32 B)
    {if(A<0 || B<0 || A>=N || B>=N || A==B || int64(Out.ShipPairs.Num())!=int64(N)*N)return true;if(A>B)Swap(A,B);return Out.ShipPairs[A*N+B]!=0;}
    static bool MayCollide(const FNativeCollisionResult&,TConstArrayView<SSWBoltCollision::FHull> Snapshot,
        int32 A,int32 B,const SSWBoltCollision::FHull& LiveA,const SSWBoltCollision::FHull& LiveB);
    static bool WorthDispatching(int32 Hulls,int32 Queries){return Hulls>=256 && Queries>=2 && int64(Hulls)*Queries>=32768;}
private:
    class FWork;
    FQueuedThreadPool* Pool=nullptr;
    FEvent* Done=nullptr;
    FCriticalSection Mutex;
    bool bPending=false;
    TUniquePtr<FNativeCollisionResult> Result;
};
