#pragma once
#include "CoreMinimal.h"
struct FEconomyState;
enum class ESimDataDomain:uint8 { None=0,Inventory=1,Jobs=2,Wallet=4,Other=8,All=15 };
ENUM_CLASS_FLAGS(ESimDataDomain);
struct FSimDataChange
{
    ESimDataDomain Domains=ESimDataDomain::None;
    TSet<FName> Owners,Inventories,Items,Facilities;TSet<FGuid> Jobs;
    bool bAllInventories=false,bAllJobs=false,bAllOwners=false;
    bool AffectsOwner(FName Owner)const{return bAllOwners || Owners.IsEmpty() || Owners.Contains(Owner);}
    void Merge(const FSimDataChange& Other);
    bool AffectsIndustry(const FEconomyState&,FName Owner)const;
    bool Affects(ESimDataDomain Interest,FName Inventory=NAME_None)const
    {return EnumHasAnyFlags(Domains,Interest) && (Inventory.IsNone() || !EnumHasAnyFlags(Interest,ESimDataDomain::Inventory) || bAllInventories || Inventories.Contains(Inventory));}
    static FSimDataChange All(){FSimDataChange C;C.Domains=ESimDataDomain::All;C.bAllOwners=C.bAllInventories=C.bAllJobs=true;return C;}
};
DECLARE_MULTICAST_DELEGATE_OneParam(FSSWSimDataChanged,const FEconomyState&);
DECLARE_MULTICAST_DELEGATE_TwoParams(FSSWSimDataDetailedChanged,const FEconomyState&,const FSimDataChange&);
// Legacy/global notifications retain the safety revision. Details only filter presentation work.
class STARSHATTERWARS_API FSimDataEvents
{
public:
 static FSSWSimDataChanged& Changed();
 static FSSWSimDataDetailedChanged& DetailedChanged();
 static uint64 CurrentThreadRevision();
 static void Suppress(bool Begin);
 static FSimDataChange* Capture(FSimDataChange*);
 static void Publish(const FEconomyState& State);
 static void Publish(const FEconomyState&,const FSimDataChange&);
 static void Inventory(const FEconomyState&,FName Inventory,FName Item=NAME_None);
 static void Job(const FEconomyState&,FGuid Job,FName Facility);
 static void Wallet(const FEconomyState&,FName Owner=NAME_None,FName Counterparty=NAME_None);
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
