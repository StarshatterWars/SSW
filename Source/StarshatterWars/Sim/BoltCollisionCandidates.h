#pragma once
#include "CoreMinimal.h"
#include "Async/ParallelFor.h"

// Immutable values only: workers never access ships, shots, actors or inventories.
namespace SSWBoltCollision
{
struct FHull { FVector Position=FVector::ZeroVector; double Radius=0; };
inline void FindCandidates(TConstArrayView<FHull> Hulls,const FVector& Start,const FVector& End,
    TArray<uint8>& Results,bool AllowParallel=true)
{
    Results.SetNumUninitialized(Hulls.Num());
    const auto Evaluate=[&](int32 Index)
    {
        const auto& Hull=Hulls[Index];
        const FVector Nearest=FMath::ClosestPointOnSegment(Hull.Position,Start,End);
        // Match Ship::HitBy's rejection exactly, including non-finite inputs.
        Results[Index]=!(FVector::DistSquared(Nearest,Hull.Position)>FMath::Square(Hull.Radius*2.0));
    };
    // Small scenes remain serial; at most four chunks avoid flooding worker threads.
    if(AllowParallel && Hulls.Num()>=256)
        ParallelFor(4,[&](int32 Chunk)
        {
            const int32 Begin=static_cast<int32>(static_cast<int64>(Hulls.Num())*Chunk/4);
            const int32 Finish=static_cast<int32>(static_cast<int64>(Hulls.Num())*(Chunk+1)/4);
            for(int32 Index=Begin;Index<Finish;++Index) Evaluate(Index);
        });
    else for(int32 Index=0;Index<Hulls.Num();++Index) Evaluate(Index);
}
}
