#include "ProductionEvents.h"
FProductionStateEvent& ProductionEvents::Changed(){static FProductionStateEvent Event;return Event;}

namespace {thread_local TArray<FProductionEventRecord>* ProductionSink=nullptr;}
TArray<FProductionEventRecord>* ProductionEvents::Capture(TArray<FProductionEventRecord>* Sink){auto* Previous=ProductionSink;ProductionSink=Sink;return Previous;}
void ProductionEvents::Emit(FName Site,FGuid Job,EProductionEvent Phase,const FText& Reason)
{
    if(ProductionSink){ProductionSink->Add({Site,Job,Phase,Reason});return;}
    check(IsInGameThread());Changed().Broadcast(Site,Job,Phase,Reason);
}
