#include "EconomyAI.h"
#include "EconomyRuntime.h"
#include "ProductionEvents.h"
#include "SimDataEvents.h"
#include "ManufacturingAI.h"
#include "TransactionAI.h"
#include "CargoCatalogSubsystem.h"
#include "GameStructs.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"
FName EconomyAI::Key(FName System,FName Item){return FName(*(System.ToString()+TEXT("/")+Item.ToString()));}
double EconomyAI::PriceFactor(double Stock,double Demand){return FMath::Clamp(.8+FMath::Max(1.0,Demand)/FMath::Max(1.0,Stock),.85,3.0);}
double EconomyAI::FreightEstimate(double Tonnes,int32 Jumps){return Jumps>0 && Jumps<MAX_int32 && FMath::IsFinite(Tonnes) && Tonnes>=0?Jumps*(25.0+5.0*Tonnes):0;}
const FRegionalMarketHistory* EconomyAI::History(const FEconomyState& S,FName System,FName Item){return S.RegionalMarkets.Find(Key(System,Item));}
FName EconomyAI::NPCAccount(FName Inventory)
{
    FString Text=Inventory.ToString();int32 Slash=INDEX_NONE;if(Text.FindLastChar('/',Slash))Text.LeftInline(Slash);
    return FName(*(TEXT("NPC/")+Text));
}
double EconomyAI::PriceTrend(const FEconomyState& State,FName System,FName Item)
{
    const auto* H=History(State,System,Item);if(!H || H->Samples.Num()<2)return 1;
    const int32 Start=FMath::Max(0,H->Samples.Num()-12);double Sum=0;int32 Count=0;
    for(int32 I=Start;I<H->Samples.Num()-1;++I)if(H->Samples[I].Price>0){Sum+=H->Samples[I].Price;++Count;}
    return Count>0?FMath::Clamp(H->Samples.Last().Price/(Sum/Count),.8,1.2):1;
}
void EconomyAI::RecordTrade(FEconomyState& State,FName System,FName Item,double Quantity,double Credits)
{
    if(System.IsNone() || Item.IsNone() || !FMath::IsFinite(Quantity) || Quantity<=0 || !FMath::IsFinite(Credits) || Credits<0)return;
    auto& Series=State.RegionalMarkets.FindOrAdd(Key(System,Item));Series.System=System;Series.Item=Item;
    Series.PendingTradeVolume+=Quantity;Series.PendingTradeValue+=Credits;
    Series.PendingTradeCount=FMath::Min(Series.PendingTradeCount+1,MAX_int32-1);
}
bool EconomyAI::Snapshot(FEconomyState& State,FName System,FName Item,double Supply,double Demand,double Base,int64 Now)
{
    if(System.IsNone() || Item.IsNone() || !FMath::IsFinite(Supply) || Supply<0 || !FMath::IsFinite(Demand) || Demand<0 || !FMath::IsFinite(Base) || Base<=0 || Now<=0)return false;
    auto& Series=State.RegionalMarkets.FindOrAdd(Key(System,Item));Series.System=System;Series.Item=Item;
    if(!Series.Samples.IsEmpty() && Now-Series.Samples.Last().TimestampUtc<300)return false;
    FRegionalPriceSample Sample;Sample.TimestampUtc=Now;Sample.Supply=Supply;Sample.Demand=Demand;
    Sample.Price=Base*PriceFactor(Supply,Demand);
    Sample.TradeVolume=Series.PendingTradeVolume;Sample.TradeValue=Series.PendingTradeValue;Sample.TradeCount=Series.PendingTradeCount;
    Series.PendingTradeVolume=0;Series.PendingTradeValue=0;Series.PendingTradeCount=0;
    Series.Samples.Add(Sample);if(Series.Samples.Num()>288)Series.Samples.RemoveAt(0,Series.Samples.Num()-288);
    return true;
}
bool EconomyAI::Advance(FEconomyState& State,const UCargoCatalogSubsystem& Catalog,double Seconds,int64 Now)
{
    TMap<FName,FS_CargoCatalogRow> Goods;Catalog.ForEachDefinition([&](FName Id,const FS_CargoCatalogRow& Row){Goods.Add(Id,Row);});
    return AdvancePlain(State,Goods,Seconds,Now);
}
bool EconomyAI::AdvancePlain(FEconomyState& State,const TMap<FName,FS_CargoCatalogRow>& Definitions,double Seconds,int64 Now)
{
    if(!FMath::IsFinite(Seconds) || Seconds<=0 || Seconds>3600 || Now<=0)return false;
    State.EconomyStepSeconds+=Seconds;if(State.EconomyStepSeconds<60 && !State.RegionalMarkets.IsEmpty())return false;
    const double Step=State.EconomyStepSeconds;State.EconomyStepSeconds=0;
    static const TMap<FName,double> DemandScales=[](){TMap<FName,double> M;FString Json;TArray<TSharedPtr<FJsonValue>> Rows;
        if(FFileHelper::LoadFileToString(Json,*(FPaths::ProjectContentDir()/TEXT("GameData/Cargo/economy.json"))) && FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(Json),Rows))
            for(const auto& V:Rows){const auto O=V->AsObject();FString Name;double Scale=1;if(O && O->TryGetStringField(TEXT("Name"),Name)){O->TryGetNumberField(TEXT("DemandScale"),Scale);M.Add(FName(*Name),FMath::Clamp(Scale,.1,10.0));}}return M;}();
    TMap<FName,const FS_CargoCatalogRow*> Goods;for(const auto& Pair:Definitions)Goods.Add(Pair.Key,&Pair.Value);
    for(auto& Pair:State.Inventories)
        if(Pair.Value.bMarket && !Pair.Value.bSupplyOnly && Pair.Value.Owner==FEconomyRuntime::IndustryOwner())
            for(const auto& Good:Goods)
                if(Good.Value->Type==ECargoType::FinishedGoods && Good.Value->Size>0 && !Good.Value->bIllicit)
                {
                    const double Scale=DemandScales.FindRef(Pair.Value.System)>0?DemandScales.FindRef(Pair.Value.System):1.0;
                    Pair.Value.DesiredStock.Add(Good.Key,FMath::Max(1.0,FMath::FloorToDouble(FMath::Min(Good.Value->PreviewQuantity,40)*Scale)));
                }
    State.NPCIncomeSeconds+=Step;const int32 IncomeHours=FMath::FloorToInt(State.NPCIncomeSeconds/3600.0);State.NPCIncomeSeconds-=IncomeHours*3600.0;
    TSet<FName> Residents;
    for(auto& Pair:State.Inventories)
    {
        auto& Inv=Pair.Value;if(Inv.Owner!=FEconomyRuntime::IndustryOwner())continue;
        if(Inv.TradingOwner.IsNone())Inv.TradingOwner=NPCAccount(Pair.Key);
        if(!State.Wallets.Contains(Inv.TradingOwner))TransactionAI::ReconcileExternalBalance(State,Inv.TradingOwner,(Inv.bHullStorage || Pair.Key.ToString().Contains(TEXT("Shipworks")) || Pair.Key.ToString().Contains(TEXT("Yard")))?10000000000LL:10000000LL,TEXT("NPC business startup capital"),TEXT("Universe startup allocation"),EWalletTransactionCategory::OpeningBalance);
        const FName Household(*(TEXT("Residents/")+Inv.System.ToString()));
        if(!State.Wallets.Contains(Household))TransactionAI::ReconcileExternalBalance(State,Household,20000000,TEXT("Civilian starting budget"),TEXT("Universe startup allocation"),EWalletTransactionCategory::OpeningBalance);
        if(IncomeHours>0 && !Residents.Contains(Household))
        {Residents.Add(Household);TransactionAI::ReconcileExternalBalance(State,Household,FMath::Min(100000000LL,State.Wallets.FindRef(Household)+IncomeHours*50000LL),TEXT("Simulated civilian wages"),TEXT("NPC income"),EWalletTransactionCategory::General);}
        for(const auto& Good:Goods)
        {
            const double Available=FEconomyLedger::Available(State,Pair.Key,Good.Key),Target=Inv.DesiredStock.FindRef(Good.Key);
            if(Available<=0 && Target<=0)continue;
            if(!Good.Value->Unit.Equals(TEXT("t"),ESearchCase::IgnoreCase))Inv.WholeUnitItems.Add(Good.Key);
            const double Trend=PriceTrend(State,Inv.System,Good.Key);
            const double Factor=PriceFactor(Available,FMath::Max(1.0,Target));
            Inv.AskPrices.Add(Good.Key,FMath::Max(1LL,FMath::CeilToInt64(Good.Value->Value*Factor*Trend)));
            if(Target>0)Inv.BidPrices.Add(Good.Key,FMath::Max(1LL,FMath::CeilToInt64(Good.Value->Value*Factor*Trend)));
        }
    }
    // Aggregate once per system/item. Adding facilities must not multiply consumption or
    // make the last visited station overwrite the regional supply indicator.
    TMap<FName,TArray<FName>> Locations;
    TMap<FName,FName> Systems,Items;
    for(const auto& Pair:State.Inventories)
    {
        const auto& Inv=Pair.Value;
        if(!Inv.bMarket || Inv.Owner!=FName(TEXT("Civilian industry")))continue;
        for(const auto& Stack:Inv.Items)
        {
            const FName Id=Key(Inv.System,Stack.Type);
            Locations.FindOrAdd(Id).AddUnique(Pair.Key);Systems.Add(Id,Inv.System);Items.Add(Id,Stack.Type);
        }
    }
    // Keep observing depleted markets too; an empty hold does not erase scarcity.
    for(const auto& Pair:State.RegionalMarkets)
        if(!Pair.Value.System.IsNone() && !Pair.Value.Item.IsNone())
        {Locations.FindOrAdd(Pair.Key);Systems.Add(Pair.Key,Pair.Value.System);Items.Add(Pair.Key,Pair.Value.Item);}
    for(const auto& Pair:Locations)
    {
        const FName Item=Items.FindChecked(Pair.Key),System=Systems.FindChecked(Pair.Key);
        const auto* Found=Goods.Find(Item);if(!Found || !*Found)continue;const auto& Row=**Found;
        if(Row.Value<=0 || Row.Size<=0 || !FMath::IsFinite(Row.Value) || !FMath::IsFinite(Row.Size))continue;
        double Supply=0;for(FName Location:Pair.Value)Supply+=FEconomyLedger::Available(State,Location,Item);
        auto& Series=State.RegionalMarkets.FindOrAdd(Pair.Key);Series.System=System;Series.Item=Item;
        const int32 Base=FMath::Max(1,FMath::Min(Row.PreviewQuantity,160>>FMath::Clamp(int32(Row.Rarity),0,7)));
        if(Series.InitialSupply<=0)Series.InitialSupply=FMath::Max(1.0,Supply);
        const double Scale=DemandScales.FindRef(System)>0?DemandScales.FindRef(System):1.0;
        // Civilian end-use is a stock sink, never a fabricated trade or a debit to the pilot.
        if(Row.Type==ECargoType::FinishedGoods)
        {
            const bool Whole=!Row.Unit.Equals(TEXT("t"),ESearchCase::IgnoreCase);
            const double Want=Series.ConsumptionRemainder+Series.InitialSupply*.01*Scale*Step/3600.0;
            double Remaining=FMath::Min(Supply,Whole?FMath::FloorToDouble(Want):Want);
            const double Requested=Remaining;
            auto Ordered=Pair.Value;Ordered.Sort([](FName A,FName B){return A.LexicalLess(B);});
            for(FName Location:Ordered)
            {
                if(State.Inventories.FindChecked(Location).bSupplyOnly)continue;
                const double Use=FMath::Min(Remaining,FEconomyLedger::Available(State,Location,Item));
                if(Use>0)
                {
                    const auto& Store=State.Inventories.FindChecked(Location);const FName Household(*(TEXT("Residents/")+System.ToString()));
                    const int64 Cost=FMath::Max(1LL,FMath::CeilToInt64(Use*Row.Value));
                    if(State.Wallets.FindRef(Household)>=Cost && State.Wallets.FindRef(Store.TradingOwner)<=MAX_int64-Cost && FEconomyLedger::Withdraw(State,Store.Owner,Location,Item,Use))
                    {FEconomyLedger::Pay(State,Household,Store.TradingOwner,Cost,FGuid::NewGuid(),TEXT("Civilian end-use purchase"),ECommercePurpose::Purchase);RecordTrade(State,System,Item,Use,double(Cost));Remaining-=Use;}
                }
                if(Remaining<=UE_SMALL_NUMBER)break;
            }
            Supply-=Requested-Remaining;
            Series.ConsumptionRemainder=Whole?FMath::Frac(Want):0;
        }
        Snapshot(State,System,Item,Supply,FMath::Max(1.0,FMath::RoundToDouble(Base*.15*Scale)),Row.Value,Now);
    }
    return true;
}

void EconomyAI::AdvanceFacilities(FEconomyState& State,const TMap<FName,FSSWProductionFacility>& Facilities,const TMap<FName,FSSWProductionRecipe>& Recipes,double Seconds)
{
    if(!FMath::IsFinite(Seconds) || Seconds<=0)return;
    for(const auto& Pair:Facilities)
    {
        const auto& Site=Pair.Value;
        if(Site.FacilityType==ESSWProductionFacilityType::Storage)
        {
            const FName Stock=FEconomyRuntime::Warehouse(Site.System,Site.Facility,TEXT("Stock"));
            FEconomyRuntime::EnsureWarehouse(State,Stock,FName(*Site.System),Site.CapacityTonnes);
            if(auto* Inventory=State.Inventories.Find(Stock))
            {
                Inventory->bOperational=!State.OfflineFacilities.Contains(Pair.Key);
                Inventory->AcceptedItems.Reset();for(FName Item:Site.AllowedItems)Inventory->AcceptedItems.Add(DataTableUtils::MakeValidName(Item.ToString()));
            }
            continue;
        }
        const auto* Recipe=Recipes.Find(Site.Recipe);
        if(!Recipe || (!Recipe->ShipDesign.IsNone() && Site.FacilityType!=ESSWProductionFacilityType::Shipyard) || Site.System.IsEmpty() || Site.Facility.IsEmpty() || Site.CapacityTonnes<=0)continue;
        const FName Input=FEconomyRuntime::Warehouse(Site.System,Site.Facility,TEXT("Input"));
        const FName Output=FEconomyRuntime::Warehouse(Site.System,Site.Facility,TEXT("Output"));
        FEconomyRuntime::EnsureWarehouse(State,Input,FName(*Site.System),Site.CapacityTonnes);
        FEconomyRuntime::EnsureWarehouse(State,Output,FName(*Site.System),Site.CapacityTonnes,true);
        auto& In=State.Inventories.FindChecked(Input);auto& Out=State.Inventories.FindChecked(Output);
        In.bOperational=Out.bOperational=!State.OfflineFacilities.Contains(Pair.Key);
        Out.bSupplyOnly=true;Out.bHullStorage=Site.FacilityType==ESSWProductionFacilityType::Shipyard;
        for(const auto& Material:Recipe->Inputs)In.DesiredStock.Add(Material.Item,FMath::Min(Material.Quantity*20,Site.CapacityTonnes/FMath::Max(.001,Material.UnitTonnes)));
        if(Recipe->Inputs.IsEmpty())for(FName Mineral:ManufacturingAI::MineralKeys(*Recipe))
            In.DesiredStock.Add(Mineral,FMath::Min(Recipe->InputTonnes*5,Site.CapacityTonnes));
        const FName Product=DataTableUtils::MakeValidName(Recipe->CargoItem);
        const double Target=Recipe->OutputQuantity>0?Recipe->OutputQuantity*20:Recipe->OutputTonnes*20;
        const bool Overproduced=FEconomyLedger::Available(State,Output,Product)>Target && PriceTrend(State,FName(*Site.System),Product)<1;
        const bool HasJob=State.Jobs.ContainsByPredicate([&](const auto& J){return J.Facility==(Site.bSurface?Pair.Key:FName(*Site.Facility)) && J.State!=EIndustryJobState::Completed && J.State!=EIndustryJobState::Cancelled;});
        if(!Overproduced || HasJob)ManufacturingAI::AdvanceLive(State,Site.bSurface?Pair.Key:FName(*Site.Facility),Input,Output,Site.Recipe,*Recipe,In.bOperational,Seconds);
    }
}
bool EconomyAI::AdvanceMine(FEconomyState& State,FName Deposit,FName Inventory,FName System,FName Mineral,double Initial,double Rate,double Capacity,double Seconds)
{
    if(Deposit.IsNone() || Mineral.IsNone() || !FMath::IsFinite(Initial) || Initial<=0 || !FMath::IsFinite(Rate) || Rate<=0 || !FMath::IsFinite(Seconds) || Seconds<=0 || !FMath::IsFinite(Capacity) || Capacity<=0)return false;
    auto* Hold=FEconomyLedger::CreateInventory(State,Inventory,FEconomyRuntime::IndustryOwner(),System,Capacity);
    if(!Hold || Hold->Owner!=FEconomyRuntime::IndustryOwner() || Hold->System!=System)return false;
    Hold->bSupplyOnly=true;
    double& Remaining=State.BackgroundOreRemaining.FindOrAdd(Deposit,Initial);
    const double Amount=FMath::Min3(FMath::Max(0.0,Remaining),Rate*Seconds,FMath::Max(0.0,Hold->CapacityTonnes-FEconomyLedger::Used(*Hold)));
    if(Amount<=0 || !FEconomyLedger::Deposit(State,Inventory,Mineral,Amount,1))return false;
    Remaining-=Amount;return true;
}

void EconomyAI::AdvanceProduction(FEconomyState& State,const TMap<FName,FSSWProductionFacility>& Facilities,const TMap<FName,FSSWProductionRecipe>& Recipes)
{
    const double Now=State.EconomyClockSeconds;
    if(!FMath::IsFinite(Now) || Now<0)return;
    if(ProductionState!=&State){ProductionState=&State;bProductionDirty=true;NextProductionDeadline=0;}
    if(!bProductionDirty && (NextProductionDeadline<=0 || Now<NextProductionDeadline))return;
    bProductionDirty=false;NextProductionDeadline=0;
    FScopedSimDataChangeBatch Changes(State);
    for(const auto& Pair:Facilities)
    {
        const auto& Site=Pair.Value;const bool Operational=!State.OfflineFacilities.Contains(Pair.Key);
        if(Site.FacilityType==ESSWProductionFacilityType::Storage)
        {
            const FName Key=FEconomyRuntime::Warehouse(Site.System,Site.Facility,TEXT("Stock"));
            FEconomyRuntime::EnsureWarehouse(State,Key,FName(*Site.System),Site.CapacityTonnes);
            if(auto* Inventory=State.Inventories.Find(Key))
            {if(Inventory->bOperational!=Operational)FSimDataEvents::Publish(State);Inventory->bOperational=Operational;Inventory->AcceptedItems.Reset();for(FName Item:Site.AllowedItems)Inventory->AcceptedItems.Add(DataTableUtils::MakeValidName(Item.ToString()));}
            continue;
        }
        const auto* Recipe=Recipes.Find(Site.Recipe);
        if(!Recipe || (!Recipe->ShipDesign.IsNone() && Site.FacilityType!=ESSWProductionFacilityType::Shipyard))continue;
        const FName Facility=Site.bSurface?Pair.Key:FName(*Site.Facility);
        const FName Input=FEconomyRuntime::Warehouse(Site.System,Site.Facility,TEXT("Input")),Output=FEconomyRuntime::Warehouse(Site.System,Site.Facility,TEXT("Output"));
        FEconomyRuntime::EnsureWarehouse(State,Input,FName(*Site.System),Site.CapacityTonnes);
        FEconomyRuntime::EnsureWarehouse(State,Output,FName(*Site.System),Site.CapacityTonnes);
        auto* In=State.Inventories.Find(Input);auto* Out=State.Inventories.Find(Output);if(!In || !Out)continue;
        if(In->bOperational!=Operational || Out->bOperational!=Operational)FSimDataEvents::Publish(State);
        In->bOperational=Out->bOperational=Operational;Out->bSupplyOnly=true;Out->bHullStorage=Site.FacilityType==ESSWProductionFacilityType::Shipyard;
        for(const auto& Material:Recipe->Inputs)In->DesiredStock.Add(Material.Item,FMath::Min(Material.Quantity*20,Site.CapacityTonnes/FMath::Max(.001,Material.UnitTonnes)));
        if(Recipe->Inputs.IsEmpty())for(FName Mineral:ManufacturingAI::MineralKeys(*Recipe))In->DesiredStock.Add(Mineral,FMath::Min(Recipe->InputTonnes*5,Site.CapacityTonnes));
        const auto Active=[&]()->FIndustryJob*{return State.Jobs.FindByPredicate([&](const auto& Job){return Job.Facility==Facility && Job.State!=EIndustryJobState::Completed && Job.State!=EIndustryJobState::Cancelled;});};
        auto* Work=Active();
        if(!Operational)
        {
            if(Work && Work->CompletionAt>0)
            {
                Work->Remaining=FMath::Max(0.0,Work->CompletionAt-Now);Work->CompletionAt=0;Work->LastProgressAt=0;
                ProductionEvents::Emit(Facility,Work->Id,EProductionEvent::Stopped,INVTEXT("Facility offline; reserved materials retained."));FSimDataEvents::Publish(State);
            }
            continue;
        }
        if(Work && Work->State==EIndustryJobState::Running && Work->CompletionAt>0 && Now>=Work->CompletionAt)
        {
            const FGuid Id=Work->Id;
            const double Elapsed=FMath::Max(UE_SMALL_NUMBER,Now-Work->LastProgressAt);
            const int32 Finished=FEconomyLedger::AdvanceJobs(State,Elapsed,[Facility](FName Other){return Other==Facility;});
            Work=Active();
            ProductionEvents::Emit(Facility,Id,Finished>0?EProductionEvent::Completed:EProductionEvent::Blocked,Finished>0?INVTEXT("Batch completed."):INVTEXT("Output could not be committed; waiting for storage changes."));
            if(Work){Work->CompletionAt=0;Work->LastProgressAt=Now;}
        }
        Work=Active();
        if(!Work || Work->State!=EIndustryJobState::Running)
        {
            ManufacturingAI::AdvanceLive(State,Facility,Input,Output,Site.Recipe,*Recipe,true,UE_SMALL_NUMBER);
            Work=Active();
        }
        else if(Work->Remaining<=0 && Work->CompletionAt<=0)
        {
            const FGuid Id=Work->Id;
            if(FEconomyLedger::AdvanceJobs(State,UE_SMALL_NUMBER,[Facility](FName Other){return Other==Facility;})>0)
                ProductionEvents::Emit(Facility,Id,EProductionEvent::Completed,INVTEXT("Output committed after storage became available."));
            Work=Active();
        }
        if(Work && Work->State==EIndustryJobState::Running && Work->Remaining>0)
        {
            if(Work->CompletionAt<=0)
            {
                Work->LastProgressAt=Now;Work->CompletionAt=Now+Work->Remaining;
                ProductionEvents::Emit(Facility,Work->Id,EProductionEvent::Started,INVTEXT("Batch scheduled."));FSimDataEvents::Publish(State);
            }
            if(NextProductionDeadline<=0 || Work->CompletionAt<NextProductionDeadline)NextProductionDeadline=Work->CompletionAt;
        }
    }
}
