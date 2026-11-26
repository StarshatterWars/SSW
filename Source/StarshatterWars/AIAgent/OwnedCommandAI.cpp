#include "OwnedCommandAI.h"
#include "ShipStoreAI.h"
#include "OrdnanceMagazine.h"
#include "SerializedHullStockAI.h"
#include "EconomyRuntime.h"
#include "EconomyAI.h"
#include "CampaignFleet.h"
#include "ShipInsuranceSubsystem.h"
#include "PlayerHullLoss.h"
#include "TransactionAI.h"
#include "SimDataEvents.h"

FNativeOwnedState FNativeOwnedState::Capture(const FS_PlayerGameInfo& P)
{return {P.HangarShips,P.ShipRentals,P.StoredShipEquipment,P.OwnedShipSkins,P.AssignedHangarHull,P.SelectedHangarHull};}
void FNativeOwnedState::Apply(FS_PlayerGameInfo& P)const
{P.HangarShips=Hulls;P.ShipRentals=Rentals;P.StoredShipEquipment=Stores;P.OwnedShipSkins=Skins;P.AssignedHangarHull=Assigned;P.SelectedHangarHull=Selected;}
bool FNativeOwnedState::Matches(const FS_PlayerGameInfo& P)const
{
    check(IsInGameThread());
    if(Assigned!=P.AssignedHangarHull || Selected!=P.SelectedHangarHull || Hulls.Num()!=P.HangarShips.Num() || Rentals.Num()!=P.ShipRentals.Num() || Stores.Num()!=P.StoredShipEquipment.Num() || Skins.Num()!=P.OwnedShipSkins.Num())return false;
    for(int32 I=0;I<Hulls.Num();++I)if(!FPlayerHangarShip::StaticStruct()->CompareScriptStruct(&Hulls[I],&P.HangarShips[I],0))return false;
    for(int32 I=0;I<Rentals.Num();++I)if(!FShipRentalContract::StaticStruct()->CompareScriptStruct(&Rentals[I],&P.ShipRentals[I],0))return false;
    for(int32 I=0;I<Stores.Num();++I)if(!FStoredShipEquipment::StaticStruct()->CompareScriptStruct(&Stores[I],&P.StoredShipEquipment[I],0))return false;
    for(const auto Skin:Skins)if(!P.OwnedShipSkins.Contains(Skin))return false;return true;
}
namespace
{
FPlayerHangarShip* Hull(FS_PlayerGameInfo& P,FGuid Id){return P.HangarShips.FindByPredicate([Id](const auto& H){return H.HullId==Id;});}
FName HullItem(FName Design){return DataTableUtils::MakeValidName(Design.ToString().ToLower()+TEXT(" assembled hull"));}
void Annotate(FEconomyState& E,FGuid Id,const FPlayerHangarShip& H,FName Source,const FString& Destination,int64 Now)
{if(auto* J=E.Journal.FindByPredicate([Id](const auto& R){return R.Id==Id;})){J->Reference=H.HullId;J->Item=H.Design;J->Quantity=1;J->Source=Source;J->Destination=FName(*Destination);J->TimestampUtc=Now;J->Category=EWalletTransactionCategory::Shipyard;}}
bool EmptyHold(const FS_PlayerGameInfo& P,FGuid Id)
{
    const FName Hold(*(TEXT("Ship/")+Id.ToString(EGuidFormats::Digits)));const auto* Inv=P.Economy.Inventories.Find(Hold);
    for(const auto& Pair:P.Economy.Reservations)if(Pair.Value.Inventory==Hold)return false;
    return (!Inv || (Inv->Items.IsEmpty() && Inv->ReservedOutputTonnes==0)) && !P.Economy.Jobs.ContainsByPredicate([Hold](const auto& J){return (J.Input==Hold || J.Output==Hold) && (J.State==EIndustryJobState::Queued || J.State==EIndustryJobState::Running || J.State==EIndustryJobState::Blocked);});
}
bool PutHull(FS_PlayerGameInfo& P,const FPlayerHangarShip& H,const FShipCommerceDock& D)
{
    auto* Store=P.Economy.Inventories.Find(D.Inventory);
    if(!Store){Store=FEconomyLedger::CreateInventory(P.Economy,D.Inventory,FEconomyRuntime::IndustryOwner(),D.System,1000000);if(Store){Store->bHullStorage=true;Store->TradingOwner=D.Dealer;}}
    if(!Store || !Store->bOperational || !Store->bHullStorage || Store->Owner!=FEconomyRuntime::IndustryOwner() || (!Store->TradingOwner.IsNone() && Store->TradingOwner!=D.Dealer) || !FEconomyLedger::Deposit(P.Economy,D.Inventory,HullItem(H.Design),1,1))return false;
    auto& Serial=P.Economy.SerializedHulls.FindOrAdd(H.HullId);Serial.HullId=H.HullId;Serial.Design=H.Design;Serial.Inventory=D.Inventory;Serial.Squadron.Reset();Serial.Slot=INDEX_NONE;
    Serial.ReservedBy.Invalidate();Serial.SoldReceipt.Invalidate();Serial.bWithdrawn=false;Serial.Delivery=D.Location;P.Economy.UsedHullStock.Add(H.HullId,H);return true;
}
bool Acquire(FS_PlayerGameInfo& P,const FNativeOwnedCommand& C,bool Rental,FText& Why)
{
    const FName Pilot=FEconomyLedger::Pilot();auto& E=P.Economy;const auto* Store=E.Inventories.Find(C.Dock.Inventory);auto H=C.PreparedHull;
    if(!C.Dock.bAuthorized || C.Dock.Location.IsEmpty() || H.HullId!=C.Hull || H.Design.IsNone() || !Store || !Store->bOperational || !Store->bHullStorage || Store->Owner!=FEconomyRuntime::IndustryOwner() ||
        (Store->TradingOwner.IsNone()?EconomyAI::NPCAccount(C.Dock.Inventory):Store->TradingOwner)!=C.Dock.Dealer || C.Dock.Dealer.IsNone() || (Hull(P,C.Hull) && !Hull(P,C.Hull)->bSold && !Hull(P,C.Hull)->bLost))return false;
    int64 Premium=0,Fee=C.Cost;FShipInsuranceQuote Insurance;
    if(Rental)
    {
        if(P.ShipRentals.ContainsByPredicate([](const auto& R){return R.UnpaidLiability>0;}))return false;
        auto Value=H;Value.bCashPurchased=false;Value.InstalledComponentValue=0;Insurance=UShipInsuranceSubsystem::Quote(Value,EShipInsuranceTier::Basic);Premium=Insurance.Premium;
        Fee=ShipCommerceAI::RentalFee(C.BaseCost,C.Days,C.Rules);if(Fee<=0 || Fee+Premium!=C.Cost)return false;
    }
    if(C.Cost<=0 || E.Wallets.FindRef(Pilot)<C.Cost || !SerializedHullStockAI::Reserve(E,C.Hull,C.Receipt,C.Dock.Inventory,H.Design) ||
        !FEconomyLedger::Withdraw(E,Store->Owner,C.Dock.Inventory,HullItem(H.Design),1) ||
        !FEconomyLedger::Pay(E,Pilot,C.Dock.Dealer,Fee,C.Receipt,Rental?TEXT("Hull rental fee"):TEXT("Shipyard hull purchase"),Rental?ECommercePurpose::Service:ECommercePurpose::Purchase))return false;
    const FGuid PremiumId=FGuid::NewGuid();
    if(Rental && !FEconomyLedger::Pay(E,Pilot,TEXT("Insurance fund"),Premium,PremiumId,TEXT("Rental insurance premium"),ECommercePurpose::Service))return false;
    if(!SerializedHullStockAI::Deliver(E,C.Hull,C.Receipt,C.Dock.Location))return false;
    H.Location=C.Dock.Location;H.bSold=false;H.bLost=false;H.bAssigned=false;H.bCashPurchased=false;H.RentalId.Invalidate();H.Insurance={};
    if(Rental)
    {
        H.RentalId=C.Receipt;H.Insurance.Tier=EShipInsuranceTier::Basic;H.Insurance.StartsUtc=C.NowUtc;H.Insurance.ExpiresUtc=C.NowUtc+12LL*7*86400;H.Insurance.InsuredValue=H.HullValue;
        FShipRentalContract R;R.Id=C.Receipt;R.HullId=C.Hull;R.Lessor=C.Dock.Dealer;R.Inventory=C.Dock.Inventory;R.ReturnDock=C.Dock.Location;R.StartsUtc=C.NowUtc;R.DueUtc=C.NowUtc+C.Days*86400LL;
        R.HullValue=H.HullValue;R.Fee=Fee;R.Premium=Premium;R.Payout=Insurance.Payout;P.ShipRentals.Add(R);Annotate(E,PremiumId,H,C.Dock.Inventory,C.Dock.Location,C.NowUtc);
    }
    else H.SelectedSkin=NAME_None;
    Annotate(E,C.Receipt,H,C.Dock.Inventory,C.Dock.Location,C.NowUtc);
    if(auto* Old=Hull(P,C.Hull))*Old=H;else P.HangarShips.Add(H);
    if(!Rental){E.UsedHullStock.Remove(C.Hull);EconomyAI::RecordTrade(E,C.Dock.System,HullItem(H.Design),1,C.Cost);}
    Why=Rental?INVTEXT("Rental saved. NPC title retained; return this hull to its issuing dock."):INVTEXT("Hull purchased and delivered to your hangar.");return true;
}
bool Sell(FS_PlayerGameInfo& P,const FNativeOwnedCommand& C,FText& Why)
{
    auto* H=Hull(P,C.Hull);if(!H || !C.Dock.bAuthorized || H->bAssigned || H->RentalId.IsValid() || H->bSold || H->bLost || P.AssignedHangarHull==C.Hull || !H->Location.Equals(C.Dock.Location,ESearchCase::IgnoreCase) || !EmptyHold(P,C.Hull) || !C.Categories.Contains(H->Design))return false;
    if(C.Categories.FindChecked(H->Design)==EShipCategory::Fighter)
    {
        const bool Spare=P.HangarShips.ContainsByPredicate([&](const auto& Other)
        {FCampaignFleetHull Readiness;Readiness.Condition=Other;const auto* Type=C.Categories.Find(Other.Design);return Other.HullId.IsValid() && Other.HullId!=C.Hull && !Other.bAssigned && !Other.bLost && !Other.bSold && !Other.RentalId.IsValid() && FCampaignFleet::CanMove(Readiness) && Type && *Type==EShipCategory::Fighter;});
        if(!Spare){Why=INVTEXT("Keep at least one owned, serviceable fighter.");return false;}
    }
    if(C.PreparedHull.HullId!=C.Hull || C.PreparedHull.Design!=H->Design)return false;*H=C.PreparedHull;const auto Sold=*H;
    // The host resolved legacy equipment values before submitting this value snapshot.
    if(!H->bEquipmentValueSeparated && !H->Components.IsEmpty())return false;
    const int64 Price=ShipCommerceAI::SalePrice(*H,C.Rules);
    if(Price<=0 || Price!=C.Cost || !PutHull(P,Sold,C.Dock) || !FEconomyLedger::Pay(P.Economy,C.Dock.Dealer,FEconomyLedger::Pilot(),Price,C.Receipt,TEXT("Owned hull sale"),ECommercePurpose::MarketSale))return false;
    H=Hull(P,C.Hull);H->bSold=true;H->Insurance.bSettled=true;if(P.SelectedHangarHull==C.Hull)P.SelectedHangarHull.Invalidate();Annotate(P.Economy,C.Receipt,Sold,C.Dock.Inventory,C.Dock.Location,C.NowUtc);
    Why=INVTEXT("Hull sale saved; included taxes and fees recorded in Wallet.");return true;
}
}
FNativeOwnedReply OwnedCommandAI::Execute(FEconomyState& State,FNativeOwnedCommand&& C)
{
    FNativeOwnedReply Out;Out.Source=C.Source;Out.bLoss=C.Action==EOwnedCommand::Loss;
    if(!C.Receipt.IsValid() || State.CommittedTransactions.Contains(C.Receipt) || C.NowUtc<0 || C.NowUtc>253402300799LL-12LL*7*86400){Out.Reason=INVTEXT("Invalid or replayed ownership request.");return Out;}
    FS_PlayerGameInfo P;P.Economy=State;C.Source.Apply(P);FScopedSimDataChangeBatch Batch(P.Economy);
    auto* H=Hull(P,C.Hull);
    switch(C.Action)
    {
    case EOwnedCommand::Purchase:case EOwnedCommand::Rent:Out.bSucceeded=Acquire(P,C,C.Action==EOwnedCommand::Rent,Out.Reason);break;
    case EOwnedCommand::Sell:Out.bSucceeded=Sell(P,C,Out.Reason);break;
    case EOwnedCommand::Return:
        if(H && C.PreparedHull.HullId==C.Hull && C.PreparedHull.Design==H->Design && (C.PreparedHull.bEquipmentValueSeparated || C.PreparedHull.Components.IsEmpty())){*H=C.PreparedHull;Out.bSucceeded=ShipCommerceAI::Return(P,C.Hull,C.Dock,C.Receipt,C.NowUtc,[]{return true;},Out.Reason);}break;
    case EOwnedCommand::Debt:Out.bSucceeded=ShipCommerceAI::PayRentalDebt(P,C.Hull,C.Receipt,[]{return true;},Out.Reason);break;
    case EOwnedCommand::Policy:
        if(H && !C.Dock.Location.IsEmpty() && H->Location.Equals(C.Dock.Location,ESearchCase::IgnoreCase) && !H->bLost && !H->bSold && !H->bAssigned && !H->bCashPurchased && !H->RentalId.IsValid() && C.Tier>=EShipInsuranceTier::Basic && C.Tier<=EShipInsuranceTier::Platinum && UShipInsuranceSubsystem::EffectiveTier(*H,C.NowUtc)==EShipInsuranceTier::Uninsured)
        {
            const auto Quote=UShipInsuranceSubsystem::Quote(*H,C.Tier);
            if(Quote.Premium==C.Cost && Quote.Payout>0 && FEconomyLedger::Pay(P.Economy,FEconomyLedger::Pilot(),TEXT("Insurance fund"),C.Cost,C.Receipt,TEXT("Insurance premium"),ECommercePurpose::Service))
            {
                H->Insurance.Tier=C.Tier;H->Insurance.StartsUtc=C.NowUtc;H->Insurance.ExpiresUtc=C.NowUtc+12LL*7*86400;H->Insurance.InsuredValue=FMath::Clamp(H->HullValue,0LL,1000000000000000LL)+FMath::Clamp(H->InstalledComponentValue,0LL,1000000000000000LL);H->Insurance.bSettled=false;
                auto& J=P.Economy.Journal.Last();J.Item=H->Design;J.Quantity=1;J.UnitPrice=C.Cost;J.Reference=C.Hull;Out.bSucceeded=true;
            }
        }break;
    case EOwnedCommand::TransferDrone:Out.bSucceeded=ShipStoreAI::TransferMiningDrone(P,C.Dock.Location,C.Hold,C.Quantity,true,[]{return true;},Out.Reason);break;
    case EOwnedCommand::Loadout:
        Out.bSucceeded=H && P.AssignedHangarHull==C.Hull && OrdnanceMagazine::Exchange(P,C.Dock.Location,C.Magazine,C.Loaded,C.Wanted,Out.Reason);
        if(Out.bSucceeded){H=Hull(P,C.Hull);H->LoadoutStations=C.Stations;H->LoadoutName=C.Loadout;}break;
    case EOwnedCommand::Fit:
        if(H && P.AssignedHangarHull==C.Hull && !H->RentalId.IsValid() && H->Components.IsValidIndex(C.Slot))
        {
            const auto Removed=H->Components[C.Slot];FHangarComponent Incoming;
            if(ShipStoreAI::TakeComponent(P,Removed.SystemDesign,Removed.Component,C.Grade,C.Dock.Location,Incoming))
            {
                if(!H->bAssigned)ShipStoreAI::StoreComponent(P,Removed,C.Dock.Location);
                Incoming.SystemIndex=Removed.SystemIndex;Incoming.ComponentIndex=Removed.ComponentIndex;Incoming.SystemName=Removed.SystemName;
                if(!Incoming.bHasCondition){Incoming.bHasCondition=true;Incoming.ConditionPercent=100;Incoming.ConditionStatus=3;Incoming.MaintenanceSeconds=0;Incoming.JerryCount=0;Incoming.SpareCount=C.Quantity;}
                Incoming.InstalledValue=C.Cost;H=Hull(P,C.Hull);H->Components[C.Slot]=Incoming;Out.bSucceeded=true;
            }
        }break;
    case EOwnedCommand::Repair:Out.bSucceeded=C.Cost>0 && FEconomyLedger::Pay(P.Economy,FEconomyLedger::Pilot(),TEXT("Station repair service"),C.Cost,C.Receipt,TEXT("Full ship repairs"),ECommercePurpose::Service);break;
    case EOwnedCommand::Loss:
        if(C.bValidatedDeath && H && !H->bLost && P.AssignedHangarHull==C.Hull)
        {
            if(H->RentalId.IsValid())Out.bSucceeded=ShipCommerceAI::SettleRentalLoss(P,C.Hull,C.NowUtc,[]{return true;},Out.Reason);
            else
            {
                const auto Tier=UShipInsuranceSubsystem::EffectiveTier(*H,C.NowUtc);auto Insured=*H;
                if(Tier!=EShipInsuranceTier::Uninsured){Insured.HullValue=H->Insurance.InsuredValue;Insured.InstalledComponentValue=0;}
                const int64 Payout=H->bAssigned?0:UShipInsuranceSubsystem::Quote(Insured,Tier).Payout;
                auto& E=P.Economy;
                if(!E.InitializedMarkets.Contains(TEXT("Insurance fund")))
                {
                    const int64 Fund=E.Wallets.FindRef(TEXT("Insurance fund"));
                    if(Fund<0 || Fund>MAX_int64-1000000000000000LL || !TransactionAI::ReconcileExternalBalance(E,TEXT("Insurance fund"),Fund+1000000000000000LL,TEXT("Insurance fund capitalization"),TEXT("NPC underwriter reserves"),EWalletTransactionCategory::OpeningBalance))break;
                    E.InitializedMarkets.Add(TEXT("Insurance fund"));
                }
                if(FEconomyLedger::Pay(E,TEXT("Insurance fund"),FEconomyLedger::Pilot(),Payout,C.Receipt,TEXT("Insurance claim")))
                {
                    auto& J=E.Journal.Last();J.Item=H->Design;J.Quantity=1;J.UnitPrice=Payout;J.Reference=C.Hull;
                    Out.bSucceeded=FPlayerHullLoss::Finalize(P,C.Hull);Out.Reason=FText::Format(INVTEXT("Insurance paid {0} credits. Lost hull and cargo recorded."),FText::AsNumber(Payout));
                }
            }
        }break;
    }
    if(!Out.bSucceeded){if(Out.Reason.IsEmpty())Out.Reason=INVTEXT("Ownership request rejected. Hulls, credits and equipment retained.");return Out;}
    if(!P.Economy.CommittedTransactions.Contains(C.Receipt)){P.Economy.CommittedTransactions.Add(C.Receipt);FSimDataEvents::Publish(P.Economy);}
    State=MoveTemp(P.Economy);Out.Owned=FNativeOwnedState::Capture(P);if(Out.Reason.IsEmpty())Out.Reason=INVTEXT("OWNERSHIP CHANGE SAVED.");return Out;
}
