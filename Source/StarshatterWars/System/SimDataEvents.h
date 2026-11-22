#pragma once
#include "CoreMinimal.h"
struct FEconomyState;
DECLARE_MULTICAST_DELEGATE_OneParam(FSSWSimDataChanged,const FEconomyState&);
// Presentation invalidation only. Mutations remain in the simulation/ledger.
class STARSHATTERWARS_API FSimDataEvents
{
public:
 static FSSWSimDataChanged& Changed();
 static uint64 CurrentThreadRevision();
 static void Suppress(bool Begin);
 static void Publish(const FEconomyState& State);
 static void BeginBatch(const FEconomyState& State);
 static void EndBatch();
};

struct FScopedSimDataChangeBatch
{
 explicit FScopedSimDataChangeBatch(const FEconomyState& State){FSimDataEvents::BeginBatch(State);}
 ~FScopedSimDataChangeBatch(){FSimDataEvents::EndBatch();}
 FScopedSimDataChangeBatch(const FScopedSimDataChangeBatch&)=delete;
 FScopedSimDataChangeBatch& operator=(const FScopedSimDataChangeBatch&)=delete;
};
