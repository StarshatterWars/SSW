#pragma once
#include "CoreMinimal.h"
#include "GameStructs.h"
#include "GameStructs_products.h"
#include "ProductionEvents.h"
#include "SimDataEvents.h"
#include "ShipStoreAI.h"
#include "MarketCommandAI.h"
#include "LedgerCommandAI.h"
#include "OwnedCommandAI.h"
#include "SkillCommandAI.h"
#include "CampaignFleet.h"
#include "RepairAI.h"
#include "HAL/CriticalSection.h"
class FQueuedThreadPool;
struct FNativeBackgroundMine {FName Id,System,Region,Item;double Reserve=0,Rate=0,Capacity=0;};
struct FNativeStorePurchase
{
    FShipStoreOffer Offer;
    FString Dock;
    int32 Quantity=0;
    int64 ExpectedTotal=0;
    FGuid Receipt;
    TArray<FStoredShipEquipment> Stored;
    TSet<FName> Skins;
};
struct FNativeStorePurchaseResult
{
    bool bSucceeded=false;
    FText Reason;
    FGuid Receipt;
    TArray<FStoredShipEquipment> Stored;
    TSet<FName> Skins;
    TArray<FStoredShipEquipment> SourceStored;
    TSet<FName> SourceSkins;
};
struct FNativeFleetRepairCommand
{
    FCampaignFleetState Fleet;
    FFleetRepairContext Context;
    FFleetRepairPolicy Policy;
};
struct FNativeFleetRepairReply
{
    FCampaignFleetState Source,Fleet;
    FFleetRepairStep Step;
};
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
    TUniquePtr<FNativeStorePurchase> Purchase;
    TUniquePtr<FNativeMarketCommand> Market;
    TUniquePtr<FNativeFleetRepairCommand> Repair;
    TUniquePtr<FNativeLedgerCommand> Ledger;
    TUniquePtr<FNativeOwnedCommand> Owned;
    TUniquePtr<FNativeSkillCommand> Skill;
};
struct FNativeEconomyResult
{
    FEconomyState State;
    TArray<FProductionEventRecord> Events;
    FSimDataChange Changes;
    uint64 Revision=0;
    FString Profile;
    double NextProductionAt=0;
    bool bOffGameThread=false;
    bool bCommitted=false;
    TUniquePtr<FNativeStorePurchaseResult> Purchase;
    TUniquePtr<FNativeMarketReply> Market;
    TUniquePtr<FNativeFleetRepairReply> Repair;
    TUniquePtr<FNativeLedgerReply> Ledger;
    TUniquePtr<FNativeOwnedReply> Owned;
    TUniquePtr<FNativeSkillReply> Skill;
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
    bool IsAvailable()const{return Pool!=nullptr;}
    static FNativeEconomyResult Calculate(FNativeEconomyInput&&);
    static bool Commit(FNativeEconomyResult&&,FEconomyState&,uint64 CurrentRevision,const FString& Profile);
    static bool CommitPurchase(FNativeEconomyResult&&,FS_PlayerGameInfo&,uint64 CurrentRevision,const FString& Profile,TFunctionRef<bool()> Save,FText& Reason);
    static bool CommitSkill(FNativeEconomyResult&&,FS_PlayerGameInfo&,uint64,const FString&,TFunctionRef<bool()>,FText&);
    static bool CommitOwned(FNativeEconomyResult&&,FS_PlayerGameInfo&,uint64,const FString&,TFunctionRef<bool()>,FText&);
    static bool CommitLedger(FNativeEconomyResult&&,FEconomyState&,uint64,const FString&,TFunctionRef<bool()>,FText&);
    static bool CommitRepair(FNativeEconomyResult&&,FEconomyState&,FCampaignFleetState&,uint64,const FString&,TFunctionRef<bool()>,FText&);
    static bool CommitMarket(FNativeEconomyResult&&,FEconomyState&,uint64 CurrentRevision,const FString& Profile,TFunctionRef<bool()> Save,FText& Reason);
private:
    class FWork;
    FQueuedThreadPool* Pool=nullptr;
    mutable FCriticalSection Mutex;
    bool bBusy=false;
    TUniquePtr<FNativeEconomyResult> Result;
};
