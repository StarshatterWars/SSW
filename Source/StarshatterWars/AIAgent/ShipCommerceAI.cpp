#include "ShipCommerceAI.h"
#include "ShipDesignRegistry.h"
#include "ShipyardAI.h"
#include "SerializedHullStockAI.h"
#include "ShipEquipment.h"
#include "CampaignFleet.h"
#include "ShipInsuranceSubsystem.h"
#include "PlayerHullLoss.h"
#include "EconomyRuntime.h"
#include "EconomyAI.h"
#include "TransactionAI.h"
#include "SimDataEvents.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"

namespace
{
const FPlayerHangarShip* Find(const FS_PlayerGameInfo& P,FGuid Id)
{return P.HangarShips.FindByPredicate([Id](const auto& H){return H.HullId==Id;});}
FName Item(FName Design){return DataTableUtils::MakeValidName(Design.ToString().ToLower()+TEXT(" assembled hull"));}
bool EmptyHold(const FS_PlayerGameInfo& P,FGuid Id)
{
    const FName Key(*(TEXT("Ship/")+Id.ToString(EGuidFormats::Digits)));
    const auto* Hold=P.Economy.Inventories.Find(Key);
    for(const auto& R:P.Economy.Reservations)if(R.Value.Inventory==Key)return false;
    return (!Hold || (Hold->Items.IsEmpty() && Hold->ReservedOutputTonnes==0)) &&
        !P.Economy.Jobs.ContainsByPredicate([Key](const auto& J){return (J.Input==Key || J.Output==Key) && (J.State==EIndustryJobState::Queued || J.State==EIndustryJobState::Running || J.State==EIndustryJobState::Blocked);});
}
bool AtDock(const FS_PlayerGameInfo& P,const FPlayerHangarShip* H,const FShipCommerceDock& D,FText& Why)
{
    if(!D.bAuthorized || D.Location.IsEmpty() || !H || !H->HullId.IsValid() || H->bSold || H->bLost || !H->Location.Equals(D.Location,ESearchCase::IgnoreCase))
    {Why=INVTEXT("Dock at the station or carrier holding this hull.");return false;}
    if(P.AssignedHangarHull==H->HullId){Why=INVTEXT("Board another craft before selling or returning this hull.");return false;}
    if(!EmptyHold(P,H->HullId)){Why=INVTEXT("Unload cargo and clear cargo reservations before handing over the hull.");return false;}
    return true;
}
void Receipt(FEconomyState& E,FGuid Id,FGuid Hull,FName Design,FName Source,const FString& Dock)
{
    auto* R=E.Journal.FindByPredicate([Id](const auto& J){return J.Id==Id;});
    if(R){R->Reference=Hull;R->Item=Design;R->Quantity=1;R->Source=Source;R->Destination=FName(*Dock);R->Category=EWalletTransactionCategory::Shipyard;}
}
bool PutStock(FS_PlayerGameInfo& P,const FPlayerHangarShip& Hull,const FShipCommerceDock& D)
{
    auto* Store=P.Economy.Inventories.Find(D.Inventory);
    if(!Store){Store=FEconomyLedger::CreateInventory(P.Economy,D.Inventory,FEconomyRuntime::IndustryOwner(),D.System,1000000);if(Store){Store->bHullStorage=true;Store->TradingOwner=D.Dealer;}}
    if(!Store || !Store->bOperational || !Store->bHullStorage || Store->Owner!=FEconomyRuntime::IndustryOwner() ||
       (!Store->TradingOwner.IsNone() && Store->TradingOwner!=D.Dealer) || !FEconomyLedger::Deposit(P.Economy,D.Inventory,Item(Hull.Design),1,1))return false;
    auto& Serial=P.Economy.SerializedHulls.FindOrAdd(Hull.HullId);
    Serial.HullId=Hull.HullId;Serial.Design=Hull.Design;Serial.Inventory=D.Inventory;Serial.Squadron.Reset();Serial.Slot=INDEX_NONE;
    Serial.ReservedBy.Invalidate();Serial.SoldReceipt.Invalidate();Serial.bWithdrawn=false;Serial.Delivery=D.Location;
    auto StockHull=Hull;
    if(!StockHull.bEquipmentValueSeparated && !StockHull.Components.IsEmpty())FShipEquipment::SeparateValue(StockHull);
    P.Economy.UsedHullStock.Add(Hull.HullId,MoveTemp(StockHull));
    return true;
}
}
bool FShipCommerceRules::Load(FShipCommerceRules& Out,FString& Error)
{
    static TOptional<FShipCommerceRules> Cached;
    if(Cached.IsSet()){Out=Cached.GetValue();return true;}
    FString Text;TSharedPtr<FJsonObject> Root;
    if(!FFileHelper::LoadFileToString(Text,*(FPaths::ProjectContentDir()/TEXT("GameData/Ships/commerce.json"))) ||
       !FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(Text),Root) || !Root ||
       !Root->TryGetNumberField(TEXT("buyback_percent"),Out.BuybackPercent) ||
       !Root->TryGetNumberField(TEXT("daily_rental_percent"),Out.DailyRentalPercent) ||
       Out.BuybackPercent<1 || Out.BuybackPercent>100 || Out.DailyRentalPercent<1 || Out.DailyRentalPercent>100)
    {Error=TEXT("Ship commerce policy is missing or invalid; transactions are disabled.");return false;}Cached=Out;return true;
}
int64 ShipCommerceAI::SalePrice(const FPlayerHangarShip& H,const FShipCommerceRules& R)
{
    if(!H.bEquipmentValueSeparated && !H.Components.IsEmpty())
    {auto Valued=H;FShipEquipment::SeparateValue(Valued);return SalePrice(Valued,R);}
    const double Condition=H.bHasCondition?FMath::Clamp(double(H.HullCondition),0.,1.):1.;
    double Parts=0;for(const auto& C:H.Components)Parts=FMath::Min(1.e15,Parts+FMath::Clamp(C.InstalledValue,0LL,1000000000000000LL)*(C.bHasCondition?FMath::Clamp(double(C.ConditionPercent)/100.,0.,1.):1.));
    if(H.Components.IsEmpty())Parts=FMath::Max(0LL,H.InstalledComponentValue)*Condition;
    return FMath::FloorToInt64((FMath::Clamp(H.HullValue,0LL,1000000000000000LL)*Condition+Parts)*FMath::Clamp(R.BuybackPercent,1,100)/100.);
}
int64 ShipCommerceAI::RentalFee(int64 Value,int32 Days,const FShipCommerceRules& R)
{return Value>0 && Value<=1000000000000000LL && (Days==1 || Days==3 || Days==7)?FMath::Max(1LL,Value*FMath::Clamp(R.DailyRentalPercent,1,100)*Days/100):0;}
bool ShipCommerceAI::CanSell(const FS_PlayerGameInfo& P,FGuid Id,const FShipCommerceDock& D,const FShipCommerceRules& R,FText& Why)
{
    const auto* H=Find(P,Id);if(!AtDock(P,H,D,Why))return false;
    if(H->bAssigned || H->RentalId.IsValid()){Why=INVTEXT("Mission loaners and rented hulls cannot be sold.");return false;}
    const auto* Design=ShipDesignRegistry::Find(H->Design);
    if(!Design){Why=INVTEXT("Hull design is unavailable.");return false;}
    if(Design->Category==EShipCategory::Fighter)
    {
        const bool Spare=P.HangarShips.ContainsByPredicate([Id](const auto& Row)
        {
            const auto* Def=ShipDesignRegistry::Find(Row.Design);FCampaignFleetHull Readiness;Readiness.Condition=Row;
            return Row.HullId.IsValid() && Row.HullId!=Id && !Row.bLost && !Row.bSold && !Row.bAssigned && !Row.RentalId.IsValid() &&
                FCampaignFleet::CanMove(Readiness) && Def && Def->Category==EShipCategory::Fighter;
        });
        if(!Spare){Why=INVTEXT("Keep at least one owned, serviceable fighter. Rentals and mission loaners do not count.");return false;}
    }
    const int64 Price=SalePrice(*H,R);
    if(Price<=0 || D.Dealer.IsNone() || P.Economy.Wallets.FindRef(D.Dealer)<Price){Why=INVTEXT("This dealer cannot fund the buyback quote.");return false;}
    Why=CommerceCharges::Quote(Price,ECommercePurpose::MarketSale).Breakdown();return true;
}
bool ShipCommerceAI::Sell(FS_PlayerGameInfo& P,FGuid Id,const FShipCommerceDock& D,const FShipCommerceRules& R,FGuid Tx,int64 Now,TFunctionRef<bool()> Save,FText& Why)
{
    FScopedSimDataChangeBatch Batch(P.Economy);
    if(!Tx.IsValid())return false;
    if(P.Economy.CommittedTransactions.Contains(Tx))return P.Economy.Journal.ContainsByPredicate([&](const auto& J){return J.Id==Tx && J.Reference==Id && J.Reason==FName(TEXT("Owned hull sale"));});
    if(!CanSell(P,Id,D,R,Why))return false;
    const auto Before=P;const auto Hull=*Find(P,Id);const int64 Price=SalePrice(Hull,R);
    if(!PutStock(P,Hull,D) || !FEconomyLedger::Pay(P.Economy,D.Dealer,FEconomyLedger::Pilot(),Price,Tx,TEXT("Owned hull sale"),ECommercePurpose::MarketSale))
    {P=Before;Why=INVTEXT("The dealer has no free storage or credits. Nothing was sold.");return false;}
    Receipt(P.Economy,Tx,Id,Hull.Design,D.Inventory,D.Location);P.Economy.Journal.Last().TimestampUtc=Now;
    auto* Row=P.HangarShips.FindByPredicate([Id](const auto& H){return H.HullId==Id;});Row->bSold=true;Row->Insurance.bSettled=true;
    if(P.SelectedHangarHull==Id)P.SelectedHangarHull.Invalidate();
    if(!Save()){P=Before;Why=INVTEXT("Could not save sale. Hull, stock and credits restored.");return false;}
    Why=INVTEXT("Hull sold. The wallet records the price and included taxes/fees.");return true;
}
bool ShipCommerceAI::Rent(FS_PlayerGameInfo& P,const FShipDesign& Design,FGuid Id,const FShipCommerceDock& D,const FShipCommerceRules& R,int32 Days,int64 Now,FGuid Tx,FText& Why)
{
    FScopedSimDataChangeBatch Batch(P.Economy);
    if(!D.bAuthorized || D.Location.IsEmpty() || !Tx.IsValid() || Now<=0 || !ShipyardAI::Eligible(P,Design,Why))return false;
    if(P.ShipRentals.ContainsByPredicate([Tx,Id](const auto& C){return C.Id==Tx && C.HullId==Id;}))return true;
    if(P.ShipRentals.ContainsByPredicate([](const auto& C){return C.UnpaidLiability>0;})){Why=INVTEXT("Settle outstanding rental liability first.");return false;}
    const auto* Store=P.Economy.Inventories.Find(D.Inventory);
    if(!Store || !Store->bOperational || !Store->bHullStorage || Store->Owner!=FEconomyRuntime::IndustryOwner() ||
       (Store->TradingOwner.IsNone()?EconomyAI::NPCAccount(D.Inventory):Store->TradingOwner)!=D.Dealer || D.Dealer.IsNone())return false;
    if(Find(P,Id) && !Find(P,Id)->bSold && !Find(P,Id)->bLost){Why=INVTEXT("This hull is already in your custody.");return false;}
    const int64 Fee=RentalFee(Design.BaseCost,Days,R);if(Fee<=0){Why=INVTEXT("Choose a one, three or seven day rental.");return false;}
    const auto Before=P;FPlayerHangarShip Hull;
    if(const auto* Used=P.Economy.UsedHullStock.Find(Id))Hull=*Used;
    else
    {
        Hull.HullId=Id;Hull.Design=FName(*Design.ShipName);Hull.Name=Design.DisplayName.IsEmpty()?Design.ShipName:Design.DisplayName;Hull.HullValue=Design.BaseCost;
        if(!Design.Loadout.IsEmpty()){Hull.LoadoutName=Design.Loadout[0].Name;Hull.LoadoutStations=Design.Loadout[0].Stations;}
        FShipEquipment::InitializePurchase(Hull,Design);
    }
    // Existing Basic policy rates, hull only. Equipment liability is not charged.
    auto Insured=Hull;Insured.bCashPurchased=false;Insured.InstalledComponentValue=0;
    const auto Insurance=UShipInsuranceSubsystem::Quote(Insured,EShipInsuranceTier::Basic);
    const FGuid Premium=FGuid::NewGuid();
    if(P.Economy.Wallets.FindRef(FEconomyLedger::Pilot())<Fee+Insurance.Premium ||
       !SerializedHullStockAI::Reserve(P.Economy,Id,Tx,D.Inventory,FName(*Design.ShipName)) ||
       !FEconomyLedger::Withdraw(P.Economy,Store->Owner,D.Inventory,Item(FName(*Design.ShipName)),1) ||
       !FEconomyLedger::Pay(P.Economy,FEconomyLedger::Pilot(),D.Dealer,Fee,Tx,TEXT("Hull rental fee"),ECommercePurpose::Service) ||
       !FEconomyLedger::Pay(P.Economy,FEconomyLedger::Pilot(),TEXT("Insurance fund"),Insurance.Premium,Premium,TEXT("Rental insurance premium"),ECommercePurpose::Service) ||
       !SerializedHullStockAI::Deliver(P.Economy,Id,Tx,D.Location))
    {P=Before;Why=INVTEXT("Rental stock or credits changed. No charges retained.");return false;}
    Receipt(P.Economy,Tx,Id,Hull.Design,D.Inventory,D.Location);Receipt(P.Economy,Premium,Id,Hull.Design,D.Inventory,D.Location);
    Hull.bSold=false;Hull.bLost=false;Hull.bAssigned=false;Hull.bCashPurchased=false;Hull.RentalId=Tx;Hull.Location=D.Location;
    Hull.Insurance.Tier=EShipInsuranceTier::Basic;Hull.Insurance.StartsUtc=Now;Hull.Insurance.ExpiresUtc=Now+12LL*7*86400;Hull.Insurance.InsuredValue=Hull.HullValue;Hull.Insurance.bSettled=false;
    auto* Existing=P.HangarShips.FindByPredicate([Id](const auto& H){return H.HullId==Id;});if(Existing)*Existing=Hull;else P.HangarShips.Add(Hull);
    FShipRentalContract C;C.Id=Tx;C.HullId=Id;C.Lessor=D.Dealer;C.Inventory=D.Inventory;C.ReturnDock=D.Location;C.StartsUtc=Now;C.DueUtc=Now+Days*86400LL;C.HullValue=Hull.HullValue;C.Fee=Fee;C.Premium=Insurance.Premium;C.Payout=Insurance.Payout;P.ShipRentals.Add(C);
    Why=INVTEXT("Rental active. NPC retains ownership; return this hull to the issuing dock. Basic hull insurance included; no component-loss charge.");return true;
}
bool ShipCommerceAI::CanReturn(const FS_PlayerGameInfo& P,FGuid Id,const FShipCommerceDock& D,FText& Why)
{
    const auto* H=Find(P,Id);if(!AtDock(P,H,D,Why))return false;
    const auto* C=P.ShipRentals.FindByPredicate([H](const auto& R){return R.Id==H->RentalId && R.State==EShipRentalState::Active;});
    if(!C || !C->ReturnDock.Equals(D.Location,ESearchCase::IgnoreCase)){Why=INVTEXT("Return a rental to its issuing station or carrier.");return false;}
    Why=INVTEXT("Return this hull with its current equipment and condition. No automatic repairs or rental-fee refund.");return true;
}
bool ShipCommerceAI::Return(FS_PlayerGameInfo& P,FGuid Id,const FShipCommerceDock& D,FGuid Tx,int64 Now,TFunctionRef<bool()> Save,FText& Why)
{
    FScopedSimDataChangeBatch Batch(P.Economy);
    if(!Tx.IsValid())return false;
    if(P.Economy.CommittedTransactions.Contains(Tx))return P.Economy.Journal.ContainsByPredicate([&](const auto& J){return J.Id==Tx && J.Reference==Id && J.Reason==FName(TEXT("Rental hull return"));});
    if(!CanReturn(P,Id,D,Why))return false;
    const auto Before=P;const auto Hull=*Find(P,Id);
    auto* C=P.ShipRentals.FindByPredicate([&](const auto& R){return R.Id==Hull.RentalId;});
    // Return to dealer stock at the same physical dock, never to a resurrected squadron slot.
    auto Back=D;Back.Dealer=C->Lessor;
    if(!PutStock(P,Hull,Back) || !FEconomyLedger::Pay(P.Economy,FEconomyLedger::Pilot(),C->Lessor,0,Tx,TEXT("Rental hull return")))
    {P=Before;Why=INVTEXT("Dealer storage is unavailable. Rental remains in your custody.");return false;}
    C->State=EShipRentalState::Returned;Receipt(P.Economy,Tx,Id,Hull.Design,D.Inventory,D.Location);P.Economy.Journal.Last().TimestampUtc=Now;
    auto* Row=P.HangarShips.FindByPredicate([Id](const auto& H){return H.HullId==Id;});Row->bSold=true;Row->Insurance.bSettled=true;
    if(P.SelectedHangarHull==Id)P.SelectedHangarHull.Invalidate();
    if(!Save()){P=Before;Why=INVTEXT("Could not save return. Rental and stock restored.");return false;}
    Why=INVTEXT("Rental returned. Legal ownership remains with the lessor.");return true;
}
bool ShipCommerceAI::SettleRentalLoss(FS_PlayerGameInfo& P,FGuid Id,int64 Now,TFunctionRef<bool()> Save,FText& Why)
{
    FScopedSimDataChangeBatch Batch(P.Economy);
    const auto* H=Find(P,Id);if(!H || H->bLost || !H->RentalId.IsValid() || P.AssignedHangarHull!=Id)return false;
    const auto Before=P;auto* C=P.ShipRentals.FindByPredicate([H](const auto& R){return R.Id==H->RentalId && R.State==EShipRentalState::Active;});if(!C)return false;
    const int64 Payout=Now<H->Insurance.ExpiresUtc && !H->Insurance.bSettled?C->Payout:0;
    if(!P.Economy.InitializedMarkets.Contains(TEXT("Insurance fund")))
    {
        const int64 Fund=P.Economy.Wallets.FindRef(TEXT("Insurance fund"));
        if(Fund<0 || Fund>MAX_int64-1000000000000000LL || !TransactionAI::ReconcileExternalBalance(P.Economy,TEXT("Insurance fund"),Fund+1000000000000000LL,TEXT("Insurance fund capitalization"),TEXT("NPC underwriter reserves"),EWalletTransactionCategory::OpeningBalance))return false;
        P.Economy.InitializedMarkets.Add(TEXT("Insurance fund"));
    }
    C->UnpaidLiability=FMath::Max(0LL,C->HullValue-Payout);const int64 Paid=FMath::Min(C->UnpaidLiability,FMath::Max(0LL,P.Economy.Wallets.FindRef(FEconomyLedger::Pilot())));
    if(!FEconomyLedger::Pay(P.Economy,TEXT("Insurance fund"),C->Lessor,Payout,FGuid::NewGuid(),TEXT("Rental insurance claim")) ||
       !FEconomyLedger::Pay(P.Economy,FEconomyLedger::Pilot(),C->Lessor,Paid,FGuid::NewGuid(),TEXT("Rental loss liability")))
    {P=Before;return false;}
    for(int32 I=0;I<2;++I)P.Economy.Journal[P.Economy.Journal.Num()-1-I].Reference=Id;
    C->UnpaidLiability-=Paid;C->State=EShipRentalState::Lost;
    if(!FPlayerHullLoss::Finalize(P,Id) || !Save()){P=Before;Why=INVTEXT("Could not save rental loss. No settlement retained.");return false;}
    Why=FText::Format(INVTEXT("Rental loss settled. Insurance paid the owner {0} CR. Remaining hull liability: {1} CR. Components incur no extra liability."),FText::AsNumber(Payout),FText::AsNumber(C->UnpaidLiability));return true;
}
bool ShipCommerceAI::PayRentalDebt(FS_PlayerGameInfo& P,FGuid Rental,FGuid Tx,TFunctionRef<bool()> Save,FText& Why)
{
    FScopedSimDataChangeBatch Batch(P.Economy);
    auto* C=P.ShipRentals.FindByPredicate([Rental](const auto& R){return R.Id==Rental;});if(!C || C->UnpaidLiability<=0 || !Tx.IsValid())return false;
    const auto Before=P;const int64 Paid=FMath::Min(C->UnpaidLiability,FMath::Max(0LL,P.Economy.Wallets.FindRef(FEconomyLedger::Pilot())));
    if(Paid<=0 || !FEconomyLedger::Pay(P.Economy,FEconomyLedger::Pilot(),C->Lessor,Paid,Tx,TEXT("Rental liability repayment")))return false;
    P.Economy.Journal.Last().Reference=C->HullId;C->UnpaidLiability-=Paid;
    if(!Save()){P=Before;return false;}Why=INVTEXT("Rental liability payment recorded.");return true;
}
bool ShipCommerceAI::RentalMayLaunch(const FS_PlayerGameInfo& P,const FPlayerHangarShip& H,int64 Now)
{
    if(H.bSold || H.bLost)return false;if(!H.RentalId.IsValid())return true;
    return P.ShipRentals.ContainsByPredicate([&](const auto& C){return C.Id==H.RentalId && C.HullId==H.HullId && C.State==EShipRentalState::Active && Now<C.DueUtc;});
}
