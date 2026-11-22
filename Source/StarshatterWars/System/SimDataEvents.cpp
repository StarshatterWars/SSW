#include "SimDataEvents.h"
namespace {thread_local int32 SSWSuppression=0;thread_local uint64 SSWMutationCount=0;thread_local int32 SSWBatchDepth=0;thread_local const FEconomyState* SSWBatchRootState=nullptr;thread_local bool SSWBatchDirty=false;}
FSSWSimDataChanged& FSimDataEvents::Changed(){static FSSWSimDataChanged Event;return Event;}
void FSimDataEvents::BeginBatch(const FEconomyState& State){if(SSWBatchDepth++==0){SSWBatchRootState=&State;SSWBatchDirty=false;}}
void FSimDataEvents::EndBatch(){check(SSWBatchDepth>0);if(--SSWBatchDepth==0){const auto* State=SSWBatchRootState;const bool Send=SSWBatchDirty;SSWBatchRootState=nullptr;SSWBatchDirty=false;if(Send && SSWSuppression==0 && IsInGameThread())Changed().Broadcast(*State);}}
void FSimDataEvents::Suppress(bool Begin){if(Begin)++SSWSuppression;else{check(SSWSuppression>0);--SSWSuppression;}}
uint64 FSimDataEvents::CurrentThreadRevision(){return SSWMutationCount;}
void FSimDataEvents::Publish(const FEconomyState& State){++SSWMutationCount;if(SSWSuppression>0 || !IsInGameThread())return;if(SSWBatchDepth>0){if(SSWBatchRootState==&State)SSWBatchDirty=true;}else Changed().Broadcast(State);}
