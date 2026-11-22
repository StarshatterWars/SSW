#include "ShipStoreAI.h"
#include "ShipSkinCatalog.h"
#include "Engine/DataTable.h"

FName ShipStoreAI::ComponentId(FName System,FName Component,int32 Grade)
{return FName(*FString::Printf(TEXT("Component/%s/%s/%d"),*System.ToString(),*Component.ToString(),Grade));}

int32 ShipStoreAI::Stored(const FS_PlayerGameInfo& Pilot,FName Id,const FString& Dock)
{
    int64 Total=0;
    for(const auto& Row:Pilot.StoredShipEquipment)
        if(Row.CatalogId==Id && Row.Location.Equals(Dock,ESearchCase::IgnoreCase))Total+=FMath::Max(0,Row.Quantity);
    return int32(FMath::Min(Total,int64(MAX_int32)));
}

bool ShipStoreAI::CanPurchase(const FS_PlayerGameInfo& Pilot,const FShipStoreOffer& Offer,int32 Quantity,
    const FString& Dock,bool Authority,FText& Reason)
{
    if(!Authority || Dock.IsEmpty()){Reason=INVTEXT("Dock at a friendly store to purchase equipment.");return false;}
    if(Offer.Id.IsNone() || Offer.Category==EShipStoreCategory::Hulls || Offer.Price<=0 || Quantity<1 || Quantity>1000 || Offer.Price>MAX_int64/Quantity)
    {Reason=INVTEXT("Select a valid item and quantity (1–1,000).");return false;}
    if(Offer.Category==EShipStoreCategory::Skins && (Quantity!=1 || Pilot.OwnedShipSkins.Contains(Offer.Id)))
    {Reason=INVTEXT("A skin license is purchased once per pilot.");return false;}
    if(Offer.Category!=EShipStoreCategory::Skins && Stored(Pilot,Offer.Id,Dock)>MAX_int32-Quantity)
    {Reason=INVTEXT("This storage stack is full.");return false;}
    if(Pilot.Economy.Wallets.FindRef(FEconomyLedger::Pilot())<Offer.Price*Quantity)
    {Reason=INVTEXT("Insufficient credits for the quoted quantity.");return false;}
    Reason=FText::Format(INVTEXT("TOTAL: {0} CR. Delivery to {1}; buying does not fit or equip the item."),
        FText::AsNumber(Offer.Price*Quantity),FText::FromString(Dock));return true;
}

bool ShipStoreAI::Purchase(FS_PlayerGameInfo& Pilot,const FShipStoreOffer& Offer,int32 Quantity,
    const FString& Dock,bool Authority,FGuid Receipt,TFunctionRef<bool()> Persist,FText& Reason)
{
    if(!CanPurchase(Pilot,Offer,Quantity,Dock,Authority,Reason))return false;
    if(!Receipt.IsValid() || Pilot.Economy.CommittedTransactions.Contains(Receipt))
    {Reason=INVTEXT("This purchase was already processed or has an invalid receipt.");return false;}
    const auto Before=Pilot;
    if(!FEconomyLedger::Pay(Pilot.Economy,FEconomyLedger::Pilot(),FName(*(TEXT("Equipment vendor / ")+Dock)),
        Offer.Price*Quantity,Receipt,TEXT("Ship Store purchase"),ECommercePurpose::Purchase))return false;
    auto& Transaction=Pilot.Economy.Journal.Last();Transaction.Category=EWalletTransactionCategory::Market;
    Transaction.Item=Offer.Id;Transaction.Quantity=Quantity;Transaction.UnitPrice=double(Offer.Price);
    if(Offer.Category==EShipStoreCategory::Skins)Pilot.OwnedShipSkins.Add(Offer.Id);
    else
    {
        // New items stack only with new items; damaged removed components retain their condition.
        auto* Row=Pilot.StoredShipEquipment.FindByPredicate([&](const auto& Item)
        {return Item.CatalogId==Offer.Id && Item.Location.Equals(Dock,ESearchCase::IgnoreCase) && !Item.ComponentState.bHasCondition;});
        if(!Row)
        {
            auto& New=Pilot.StoredShipEquipment.AddDefaulted_GetRef();Row=&New;
            Row->CatalogId=Offer.Id;Row->Category=Offer.Category;Row->Location=Dock;
            Row->ComponentState.SystemDesign=Offer.SystemDesign;Row->ComponentState.Component=Offer.Component;
            Row->ComponentState.Level=Offer.Grade;
        }
        Row->Quantity+=Quantity;
    }
    if(!Persist()){Pilot=Before;Reason=INVTEXT("Purchase could not be saved. Credits and inventory restored.");return false;}
    Reason=Offer.Category==EShipStoreCategory::Skins?INVTEXT("Skin license purchased. Apply it through Player Logbook → Skins."):
        INVTEXT("Purchase saved in local dock storage. Components are installed through Hangar → Fit.");
    return true;
}

bool ShipStoreAI::TakeComponent(FS_PlayerGameInfo& Pilot,FName System,FName Component,int32 Grade,
    const FString& Dock,FHangarComponent& Out)
{
    const FName Id=ComponentId(System,Component,Grade);
    auto* Row=Pilot.StoredShipEquipment.FindByPredicate([&](const auto& Item)
    {return Item.Category==EShipStoreCategory::Components && Item.CatalogId==Id && Item.Quantity>0 && Item.Location.Equals(Dock,ESearchCase::IgnoreCase);});
    if(!Row || Dock.IsEmpty())return false;
    Out=Row->ComponentState;Out.ItemId=FGuid::NewGuid();--Row->Quantity;return true;
}

void ShipStoreAI::StoreComponent(FS_PlayerGameInfo& Pilot,const FHangarComponent& Part,const FString& Dock)
{
    auto& Row=Pilot.StoredShipEquipment.AddDefaulted_GetRef();Row.CatalogId=ComponentId(Part.SystemDesign,Part.Component,Part.Level);
    Row.Category=EShipStoreCategory::Components;Row.Location=Dock;Row.Quantity=1;Row.ComponentState=Part;
}

bool ShipStoreAI::TransferMiningDrone(FS_PlayerGameInfo& Pilot,const FString& Dock,FName HoldKey,int32 Quantity,
    bool Authority,TFunctionRef<bool()> Persist,FText& Reason)
{
    const FName Id(TEXT("Drone/MiningDrone"));const auto* Hold=Pilot.Economy.Inventories.Find(HoldKey);
    if(!Authority || Dock.IsEmpty() || Quantity<1 || Quantity>1000 || !Hold || Hold->Owner!=FEconomyLedger::Pilot() || Stored(Pilot,Id,Dock)<Quantity)
    {Reason=INVTEXT("Dock with an owned cargo hold and enough locally stored mining drones.");return false;}
    const auto Before=Pilot;
    if(!FEconomyLedger::Deposit(Pilot.Economy,HoldKey,DataTableUtils::MakeValidName(TEXT("mining drone")),Quantity,.5))
    {Reason=INVTEXT("The cargo hold cannot accept those mining drones (0.5 t each).");return false;}
    int32 Remaining=Quantity;
    for(auto& Row:Pilot.StoredShipEquipment)
        if(Row.CatalogId==Id && Row.Location.Equals(Dock,ESearchCase::IgnoreCase))
        {const int32 Count=FMath::Min(FMath::Max(0,Row.Quantity),Remaining);Row.Quantity-=Count;Remaining-=Count;}
    if(!Persist()){Pilot=Before;Reason=INVTEXT("Transfer could not be saved. Cargo and dock inventory restored.");return false;}
    Reason=INVTEXT("Mining drones transferred to cargo. Load them into an eligible bay through Mining Drone Controls.");return true;
}
