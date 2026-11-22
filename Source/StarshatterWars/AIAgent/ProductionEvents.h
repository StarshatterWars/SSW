#pragma once
#include "CoreMinimal.h"
enum class EProductionEvent : uint8 { Started, Stopped, Completed, Blocked, Failed, Cancelled };
DECLARE_MULTICAST_DELEGATE_FourParams(FProductionStateEvent,FName,FGuid,EProductionEvent,const FText&);
// Native simulation events. No actor, level or widget owns job lifetimes.
struct FProductionEventRecord{FName Facility;FGuid Job;EProductionEvent Phase;FText Reason;};
namespace ProductionEvents {
    STARSHATTERWARS_API FProductionStateEvent& Changed();
    STARSHATTERWARS_API void Emit(FName,FGuid,EProductionEvent,const FText&);
    STARSHATTERWARS_API TArray<FProductionEventRecord>* Capture(TArray<FProductionEventRecord>*);
}
