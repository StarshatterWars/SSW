#include "SimDataEvents.h"
#include "GameStructs_economy.h"
namespace
{
thread_local int32 SSWSuppression=0,SSWBatchDepth=0;
thread_local uint64 SSWMutationCount=0;
thread_local const FEconomyState* SSWBatchRootState=nullptr;
thread_local bool SSWBatchDirty=false;
thread_local FSimDataChange SSWBatchChange;
thread_local FSimDataChange* SSWCapturedChange=nullptr;
void DispatchSimDataChange(const FEconomyState& State,const FSimDataChange& C)
{FSimDataEvents::Changed().Broadcast(State);FSimDataEvents::DetailedChanged().Broadcast(State,C);}
}
void FSimDataChange::Merge(const FSimDataChange& C)
{
 Domains|=C.Domains;bAllOwners|=C.bAllOwners || (C.Domains!=ESimDataDomain::None && C.Owners.IsEmpty());if(!bAllOwners){Owners.Append(C.Owners);if(Owners.Num()>256)bAllOwners=true;}if(bAllOwners)Owners.Reset();bAllInventories|=C.bAllInventories;bAllJobs|=C.bAllJobs;
 if(!bAllInventories){Inventories.Append(C.Inventories);Items.Append(C.Items);if(Inventories.Num()>256 || Items.Num()>256)bAllInventories=true;}
 if(!bAllJobs){Jobs.Append(C.Jobs);Facilities.Append(C.Facilities);if(Jobs.Num()>256 || Facilities.Num()>256)bAllJobs=true;}
 if(bAllInventories){Inventories.Reset();Items.Reset();}if(bAllJobs){Jobs.Reset();Facilities.Reset();}
}
FSSWSimDataChanged& FSimDataEvents::Changed(){static FSSWSimDataChanged Event;return Event;}
FSSWSimDataDetailedChanged& FSimDataEvents::DetailedChanged(){static FSSWSimDataDetailedChanged Event;return Event;}
FSimDataChange* FSimDataEvents::Capture(FSimDataChange* C){auto* Before=SSWCapturedChange;SSWCapturedChange=C;return Before;}
void FSimDataEvents::BeginBatch(const FEconomyState& State){if(SSWBatchDepth++==0){SSWBatchRootState=&State;SSWBatchDirty=false;SSWBatchChange={};}}
void FSimDataEvents::EndBatch()
{
 check(SSWBatchDepth>0);if(--SSWBatchDepth!=0)return;
 const auto* State=SSWBatchRootState;const bool Send=SSWBatchDirty;auto Change=MoveTemp(SSWBatchChange);SSWBatchRootState=nullptr;SSWBatchDirty=false;
 if(Send && SSWSuppression==0 && IsInGameThread())DispatchSimDataChange(*State,Change);
}
void FSimDataEvents::Suppress(bool Begin){if(Begin)++SSWSuppression;else{check(SSWSuppression>0);--SSWSuppression;}}
uint64 FSimDataEvents::CurrentThreadRevision(){return SSWMutationCount;}
void FSimDataEvents::Publish(const FEconomyState& State){Publish(State,FSimDataChange::All());}
void FSimDataEvents::Publish(const FEconomyState& State,const FSimDataChange& C)
{
 ++SSWMutationCount;if(SSWCapturedChange)SSWCapturedChange->Merge(C);
 if(SSWSuppression>0 || !IsInGameThread())return;
 if(SSWBatchDepth>0){if(SSWBatchRootState==&State){SSWBatchDirty=true;SSWBatchChange.Merge(C);}}else DispatchSimDataChange(State,C);
}
void FSimDataEvents::Inventory(const FEconomyState& S,FName Key,FName Item)
{FSimDataChange C;C.Domains=ESimDataDomain::Inventory;C.Inventories.Add(Key);if(const auto* Inv=S.Inventories.Find(Key))C.Owners.Add(Inv->Owner);else C.bAllOwners=true;if(!Item.IsNone())C.Items.Add(Item);Publish(S,C);}
void FSimDataEvents::Job(const FEconomyState& S,FGuid Id,FName Facility)
{FSimDataChange C;C.Domains=ESimDataDomain::Jobs;C.Jobs.Add(Id);C.Facilities.Add(Facility);if(const auto* Work=S.Jobs.FindByPredicate([Id](const auto& J){return J.Id==Id;}))C.Owners.Add(Work->Owner);else C.bAllOwners=true;Publish(S,C);}
void FSimDataEvents::Wallet(const FEconomyState& S,FName Owner,FName Counterparty){FSimDataChange C;C.Domains=ESimDataDomain::Wallet;if(!Owner.IsNone())C.Owners.Add(Owner);else C.bAllOwners=true;if(!Counterparty.IsNone())C.Owners.Add(Counterparty);Publish(S,C);}

bool FSimDataChange::AffectsIndustry(const FEconomyState& State,FName Owner)const
{
 if(!AffectsOwner(Owner))return false;
 if((bAllJobs && EnumHasAnyFlags(Domains,ESimDataDomain::Jobs)) || (bAllInventories && EnumHasAnyFlags(Domains,ESimDataDomain::Inventory)))return true;
 for(const auto& Work:State.Jobs)if(Work.Owner==Owner)
 {
  if(EnumHasAnyFlags(Domains,ESimDataDomain::Jobs) && (Jobs.Contains(Work.Id) || Facilities.Contains(Work.Facility)))return true;
  if(EnumHasAnyFlags(Domains,ESimDataDomain::Inventory) && (Inventories.Contains(Work.Input) || Inventories.Contains(Work.Output)))return true;
 }
 for(FGuid Id:Jobs)if(!State.Jobs.ContainsByPredicate([Id](const auto& Work){return Work.Id==Id;}))return true;
 return false;
}
