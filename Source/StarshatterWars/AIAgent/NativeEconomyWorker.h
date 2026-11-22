#pragma once
#include "CoreMinimal.h"
#include "GameStructs.h"
#include "GameStructs_products.h"
#include "ProductionEvents.h"
#include "HAL/CriticalSection.h"
class FQueuedThreadPool;
struct FNativeBackgroundMine {FName Id,System,Region,Item;double Reserve=0,Rate=0,Capacity=0;};
struct FNativeEconomyInput
{
    FEconomyState State;
    TMap<FName,FSSWProductionFacility> Facilities;
    TMap<FName,FSSWProductionRecipe> Recipes;
    TMap<FName,FS_CargoCatalogRow> Goods;
    TArray<FNativeBackgroundMine> Mines;
    uint64 Revision=0;
    FString Profile;
    int64 NowUtc=0;
    bool bInventoryChanged=false;
};
struct FNativeEconomyResult
{
    FEconomyState State;
    TArray<FProductionEventRecord> Events;
    uint64 Revision=0;
    FString Profile;
    double NextProductionAt=0;
    bool bOffGameThread=false;
    bool bCommitted=false;
};
// Dedicated one-thread queue. Requests and results own all their data; no actors or UObjects.
class STARSHATTERWARS_API FNativeEconomyWorker
{
public:
    FNativeEconomyWorker();
    ~FNativeEconomyWorker();
    bool Submit(FNativeEconomyInput&&,TFunction<void()> Completed);
    TUniquePtr<FNativeEconomyResult> TakeResult();
    bool IsBusy()const;
    static FNativeEconomyResult Calculate(FNativeEconomyInput&&);
    static bool Commit(FNativeEconomyResult&&,FEconomyState&,uint64 CurrentRevision,const FString& Profile);
private:
    class FWork;
    FQueuedThreadPool* Pool=nullptr;
    mutable FCriticalSection Mutex;
    bool bBusy=false;
    TUniquePtr<FNativeEconomyResult> Result;
};
