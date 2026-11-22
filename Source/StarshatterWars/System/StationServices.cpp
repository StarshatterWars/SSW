#include "StationServices.h"
#include "ProductionCatalog.h"
#include "EconomyAI.h"
#include "StationServicesAI.h"
#include "OwnedHullCondition.h"
#include "EconomyRuntime.h"
#include "CargoCatalogSubsystem.h"
#include "GameStructs.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"
#include "StarshatterPlayerSubsystem.h"
#include "Ship.h"
#include "GameStructs_products.h"
#include "Engine/DataTable.h"
#include "ShipDesign.h"
#include "SimSystem.h"
#include "SimComponent.h"
#include "Sim.h"
#include "SimRegion.h"
#include "StarSystem.h"
#include "Engine/GameInstance.h"

bool FStationServices::IsDocked()
{
    auto* Simulation = Sim::GetSim(); const auto* Craft = Simulation ? Simulation->GetPlayerShip() : nullptr;
    const auto* Base = Craft ? Craft->GetCarrier() : nullptr;
    return Simulation && !Simulation->IsNetworked() && Simulation->HasSimulationAuthority() && Craft && !Craft->IsDead() && !Craft->IsDying() &&
        Craft->GetFlightPhase() <= EOPSMode::ALERT && Base && StationServicesAI::FactionAllows(Craft,Base) && !Base->IsDead() && !Base->IsDying() && !Base->HasNetworkPilot();
}
bool FStationServices::Offers(ESSWStationService Service)
{
    if(!IsDocked())return false;
    const auto* Base=Sim::GetSim()->GetPlayerShip()->GetCarrier();
    return Base && Base->Design() && FStationServiceCatalog::Get().HasService(
        FName(UTF8_TO_TCHAR(Base->Design()->name)),FName(UTF8_TO_TCHAR(Base->GetName())),Service);
}
namespace
{
const FSSWProductionFacility* DockedFacility()
{
    if (!FStationServices::IsDocked()) return nullptr;
    auto* Craft = Sim::GetSim()->GetPlayerShip();
    for(const auto& Pair:SSWProductionCatalog::Facilities())
    {
        const auto* Site=&Pair.Value;
        auto* Region = Craft->GetRegion();
        if (Site->Facility == UTF8_TO_TCHAR(Craft->GetCarrier()->GetName()) && Region && Region->GetSystem() &&
            Site->System == UTF8_TO_TCHAR(Region->GetSystem()->GetName())) return Site;
    }
    return nullptr;
}
}
bool FStationServices::HasIndustry() { return Offers(ESSWStationService::Manufacturing) && DockedFacility() != nullptr; }
const FSSWProductionFacility* FStationServices::Facility() { return HasIndustry() ? DockedFacility() : nullptr; }
FName FStationServices::Inventory(const TCHAR* Compartment)
{
    if (!IsDocked()) return NAME_None;
    auto* Craft = Sim::GetSim()->GetPlayerShip(); auto* Region = Craft->GetRegion();
    const FString System = Region && Region->GetSystem() ? UTF8_TO_TCHAR(Region->GetSystem()->GetName()) : TEXT("");
    return FEconomyRuntime::Warehouse(System, UTF8_TO_TCHAR(Craft->GetCarrier()->GetName()), Compartment);
}
namespace
{
const FS_CargoCatalogRow* CatalogItem(UGameInstance* Instance,FName Item)
{
    auto* Catalog=Instance?Instance->GetSubsystem<UCargoCatalogSubsystem>():nullptr;
    const FS_CargoCatalogRow* Result=nullptr;
    if(Catalog)Catalog->ForEachDefinition([&](FName Key,const FS_CargoCatalogRow& Row){if(Key==Item)Result=&Row;});
    return Result;
}
bool WholeCargoUnits(const FS_CargoCatalogRow* Item)
{ return Item && !Item->Unit.Equals(TEXT("t"),ESearchCase::IgnoreCase); }
bool TransferContext(UGameInstance* Instance,FName Item,bool ToStation,const TCHAR* Compartment,FText& Reason)
{
    if(!Instance || !FStationServices::IsDocked() || Item.IsNone() || Item.ToString().StartsWith(TEXT("Courier/")))
    {Reason=INVTEXT("Dock at an NPC base. Sealed courier packages must be delivered through Contracts.");return false;}
    if(!FStationServices::Offers(ESSWStationService::Cargo))
    {Reason=INVTEXT("This dock has no cargo transfer service.");return false;}
    const FString Area(Compartment);
    if(Area!=TEXT("Player Storage") && Area!=TEXT("Player Input") && Area!=TEXT("Player Output"))
    {Reason=INVTEXT("Choose a player storage compartment.");return false;}
    if(Area!=TEXT("Player Storage") && !FStationServices::HasIndustry())
    {Reason=INVTEXT("This base has no production service.");return false;}
    if(ToStation && Area==TEXT("Player Output"))
    {Reason=INVTEXT("Facility output is collection-only.");return false;}
    return Instance->GetSubsystem<UStarshatterPlayerSubsystem>()!=nullptr;
}
struct FMarketRule { double DemandScale=1;bool Restricted=false,Found=false; };
FMarketRule MarketRule(const FString& System,FName Item,bool Illicit)
{
    FMarketRule Result;
    // Authored market policy is immutable for this game session, like the ingested catalog.
    static const TArray<TSharedPtr<FJsonValue>> Entries=[]()
    {
        FString Json;TArray<TSharedPtr<FJsonValue>> Loaded;
        if(FFileHelper::LoadFileToString(Json,*(FPaths::ProjectContentDir()/TEXT("GameData/Cargo/economy.json"))))
            FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(Json),Loaded);
        return Loaded;
    }();
    FString Exclusive;bool LocalIllicit=false;
    for(const auto& Entry:Entries)
    {
        const auto& Object=Entry->AsObject();if(!Object)continue;
        FString Name;if(!Object->TryGetStringField(TEXT("Name"),Name))continue;
        if(Name.Equals(System,ESearchCase::IgnoreCase))
        {Result.Found=true;Object->TryGetNumberField(TEXT("DemandScale"),Result.DemandScale);Object->TryGetBoolField(TEXT("bIllicitTrade"),LocalIllicit);}
        const TArray<TSharedPtr<FJsonValue>>* Goods;
        if(Object->TryGetArrayField(TEXT("ExclusiveSupply"),Goods))
            for(const auto& Good:*Goods)if(DataTableUtils::MakeValidName(Good->AsString())==Item)Exclusive=Name;
    }
    Result.Restricted=!Exclusive.IsEmpty()?!Exclusive.Equals(System,ESearchCase::IgnoreCase):Illicit&&!LocalIllicit;
    return Result;
}
}
double FStationServices::CargoTransferLimit(const FEconomyState& State,FName Source,FName Destination,FName Item,double MissingCapacity,bool WholeUnits)
{
    if(Source==Destination || Item.IsNone() || Item.ToString().StartsWith(TEXT("Courier/")))return 0;
    const auto* From=State.Inventories.Find(Source);const auto* To=State.Inventories.Find(Destination);
    if(!From || From->Owner!=FEconomyLedger::Pilot() || (To && To->Owner!=FEconomyLedger::Pilot()))return 0;
    const auto* Stack=From->Items.FindByPredicate([Item](const auto& Entry){return Entry.Type==Item;});
    if(!Stack || !FMath::IsFinite(Stack->UnitTonnes) || Stack->UnitTonnes<=0)return 0;
    const double Free=To?To->CapacityTonnes-FEconomyLedger::Used(*To):MissingCapacity;
    double Limit=FMath::Min(FEconomyLedger::Available(State,Source,Item),FMath::Max(0.0,Free)/Stack->UnitTonnes);
    if(!FMath::IsFinite(Limit))return 0;
    return FMath::Max(0.0,WholeUnits?FMath::FloorToDouble(Limit+1.e-9):Limit);
}
double FStationServices::CargoTransferLimit(UGameInstance* Instance,FName Item,bool ToStation,const TCHAR* Compartment,FText& Reason)
{
    if(!TransferContext(Instance,Item,ToStation,Compartment,Reason))return 0;
    auto* Craft=Sim::GetSim()->GetPlayerShip();Craft->SyncCargoInventory();
    const auto& State=Sim::GetSim()->ReadEconomy();const auto* Site=DockedFacility();
    const FName Store=Inventory(Compartment),Hold=Craft->GetCargoInventoryKey();
    const double Capacity=Site && FString(Compartment)!=TEXT("Player Storage")?Site->CapacityTonnes:1000000;
    const double Limit=CargoTransferLimit(State,ToStation?Hold:Store,ToStation?Store:Hold,Item,ToStation?Capacity:0,WholeCargoUnits(CatalogItem(Instance,Item)));
    Reason=Limit>UE_SMALL_NUMBER?FText::Format(INVTEXT("Maximum transferable: {0}"),FText::AsNumber(Limit)):INVTEXT("No unreserved cargo or destination space available.");
    return Limit;
}
bool FStationServices::MoveCargo(UGameInstance* Instance,FName Item,bool ToStation,const TCHAR* Compartment,FText& Reason)
{
    const double Quantity=CargoTransferLimit(Instance,Item,ToStation,Compartment,Reason);
    return Quantity>UE_SMALL_NUMBER && MoveCargo(Instance,Item,ToStation,Compartment,Quantity,Reason);
}
bool FStationServices::MoveCargo(UGameInstance* Instance,FName Item,bool ToStation,const TCHAR* Compartment,double Quantity,FText& Reason)
{
    if(!FMath::IsFinite(Quantity) || Quantity<=0){Reason=INVTEXT("Enter a positive cargo quantity.");return false;}
    const double Limit=CargoTransferLimit(Instance,Item,ToStation,Compartment,Reason);
    if(Quantity>Limit+1.e-9 || Quantity<=UE_SMALL_NUMBER)
    {
        if(Limit>UE_SMALL_NUMBER)Reason=FText::Format(INVTEXT("Requested quantity exceeds the transferable maximum of {0}."),FText::AsNumber(Limit));
        return false;
    }
    if(WholeCargoUnits(CatalogItem(Instance,Item)) && !FMath::IsNearlyEqual(Quantity,FMath::RoundToDouble(Quantity),1.e-9))
    {Reason=INVTEXT("This cargo is transferred in whole units.");return false;}
    auto* Players=Instance->GetSubsystem<UStarshatterPlayerSubsystem>();auto* Craft=Sim::GetSim()->GetPlayerShip();
    auto& State=Sim::GetSim()->GetEconomy();const auto Before=State;
    const FName Store=Inventory(Compartment),Hold=Craft->GetCargoInventoryKey();const auto* ShipInventory=State.Inventories.Find(Hold);
    if(!Players || !ShipInventory)return false;
    const auto* Site=DockedFacility();
    if(!FEconomyLedger::CreateInventory(State,Store,FEconomyLedger::Pilot(),ShipInventory->System,
        Site && FString(Compartment)!=TEXT("Player Storage")?Site->CapacityTonnes:1000000) ||
        !FEconomyLedger::Transfer(State,FEconomyLedger::Pilot(),ToStation?Hold:Store,ToStation?Store:Hold,Item,Quantity))
    {State=Before;Reason=INVTEXT("Cargo changed or the destination is full. Transfer cancelled.");return false;}
    Craft->ApplyCargoInventoryToManifest();
    if(!Players->SaveActivePlayerProgress())
    {State=Before;Craft->ApplyCargoInventoryToManifest();Reason=INVTEXT("Could not save the transfer. Cargo was restored.");return false;}
    Craft->ApplyCargoInventoryToManifest();Reason=FText::Format(INVTEXT("Transferred {0} to the selected inventory."),FText::AsNumber(Quantity));return true;
}
bool FStationServices::QuoteMarket(UGameInstance* Instance,FName Item,double Quantity,bool Buy,FStationMarketQuote& Quote)
{
    Quote=FStationMarketQuote{};Quote.Item=Item;Quote.Quantity=Quantity;Quote.bBuy=Buy;
    if(!Instance || !IsDocked()){Quote.Reason=INVTEXT("Dock at an NPC base to buy or sell cargo.");return false;}
    if(!Offers(ESSWStationService::Market)){Quote.Reason=INVTEXT("This dock has no marketplace service.");return false;}
    const auto* Goods=CatalogItem(Instance,Item);
    if(!Goods || Item.IsNone() || Item.ToString().StartsWith(TEXT("Courier/")) || Goods->Type==ECargoType::Passengers || (!FMath::IsFinite(Goods->Size) || Goods->Size<=0) || !FMath::IsFinite(Goods->Value) || Goods->Value<=0)
    {Quote.Reason=INVTEXT("This item is not available for cargo trading.");return false;}
    auto* Craft=Sim::GetSim()->GetPlayerShip();Craft->SyncCargoInventory();
    auto* Region=Craft->GetRegion();const FString System=Region && Region->GetSystem()?UTF8_TO_TCHAR(Region->GetSystem()->GetName()):TEXT("");
    const auto Rule=MarketRule(System,Item,Goods->bIllicit);
    if(!Rule.Found || Rule.Restricted){Quote.Reason=INVTEXT("This item is restricted in the local market.");return false;}
    if (const auto* Catalog = Instance->GetSubsystem<UCargoCatalogSubsystem>())
        FEconomyRuntime::InitializeMarkets(Sim::GetSim()->GetEconomy(), *Catalog);
    const auto& State=Sim::GetSim()->ReadEconomy();const FName Hold=Craft->GetCargoInventoryKey();
    const FName Market=FEconomyRuntime::Warehouse(System,TEXT("NPC Market"),TEXT("Stock"));
    Quote.Source=Buy?Market:Hold;Quote.Destination=Buy?Hold:Market;Quote.Buyer=Buy?FEconomyLedger::Pilot():FEconomyRuntime::IndustryOwner();
    Quote.Seller=Buy?FEconomyRuntime::IndustryOwner():FEconomyLedger::Pilot();Quote.Unit=Goods->Unit;
    Quote.Available=FEconomyLedger::Available(State,Market,Item);Quote.InHold=FEconomyLedger::Available(State,Hold,Item);
    Quote.Balance=State.Wallets.FindRef(FEconomyLedger::Pilot());
    const int32 Base=FMath::Max(1,FMath::Min(Goods->PreviewQuantity,160>>FMath::Clamp(int32(Goods->Rarity),0,7)));
    const double Demand=FMath::Max(1.0,FMath::RoundToDouble(Base*.15*Rule.DemandScale));
    const double Ask=Goods->Value*EconomyAI::PriceFactor(Quote.Available,Demand);
    Quote.UnitPrice=Buy?Ask:Ask*.8;
    const auto* Destination=State.Inventories.Find(Quote.Destination);
    const double Free=Destination?FMath::Max(0.0,Destination->CapacityTonnes-FEconomyLedger::Used(*Destination)):0;
    const double Funds=double(State.Wallets.FindRef(Quote.Buyer));
    Quote.Maximum=FMath::Max(0.0,FMath::Min3(Buy?Quote.Available:Quote.InHold,Free/Goods->Size,Funds/Quote.UnitPrice));
    if(WholeCargoUnits(Goods))Quote.Maximum=FMath::FloorToDouble(Quote.Maximum+1.e-9);
    if(!FMath::IsFinite(Quantity) || Quantity<=0 || !FMath::IsFinite(Quote.UnitPrice) || Quote.UnitPrice<=0)
    {Quote.Reason=INVTEXT("Enter a positive trade quantity.");return false;}
    if(WholeCargoUnits(Goods) && !FMath::IsNearlyEqual(Quantity,FMath::RoundToDouble(Quantity),1.e-9))
    {Quote.Reason=INVTEXT("This item trades in whole units.");return false;}
    const double Total=Quantity*Quote.UnitPrice;
    if(!FMath::IsFinite(Total) || Total>9.e15){Quote.Reason=INVTEXT("Trade value is too large.");return false;}
    Quote.TotalCredits=Buy?FMath::CeilToInt64(Total):FMath::FloorToInt64(Total);
    if(Quote.TotalCredits<1){Quote.Reason=INVTEXT("Quantity is too small for a whole-credit trade.");return false;}
    if((Buy?Quote.Available:Quote.InHold)+1.e-9<Quantity){Quote.Reason=Buy?INVTEXT("Insufficient unreserved NPC stock."):INVTEXT("Insufficient unreserved cargo in the ship.");return false;}
    if(Free+1.e-9<Quantity*Goods->Size){Quote.Reason=INVTEXT("The destination has insufficient cargo space.");return false;}
    if(State.Wallets.FindRef(Quote.Buyer)<Quote.TotalCredits){Quote.Reason=Buy?INVTEXT("Insufficient credits."):INVTEXT("The NPC market has insufficient credits for this purchase.");return false;}
    Quote.bAllowed=true;const auto Charges=CommerceCharges::Quote(Quote.TotalCredits,ECommercePurpose::MarketSale);
    Quote.Reason=FText::Format(Buy?INVTEXT("BUY {0} {1} FOR {2} CR"):INVTEXT("SELL {0} {1} FOR {2} CR"),FText::AsNumber(Quantity),FText::FromString(Quote.Unit).ToUpper(),FText::AsNumber(Buy?Quote.TotalCredits:Charges.Base));
    Quote.Reason=FText::Format(INVTEXT("{0}\n{1}"),Quote.Reason,Charges.Breakdown());return true;
}
bool FStationServices::CommitMarketTrade(FEconomyState& State,const FStationMarketQuote& Quote,FGuid Receipt,TFunctionRef<bool()> Persist,FText& Reason)
{
    if(!Quote.bAllowed || Quote.Item.IsNone() || Quote.Item.ToString().StartsWith(TEXT("Courier/")) || !FMath::IsFinite(Quote.Quantity) || Quote.Quantity<=0 || Quote.TotalCredits<1)
    {Reason=INVTEXT("A valid market quote is required.");return false;}
    const auto Before=State;
    if(!FEconomyLedger::Trade(State,Quote.Buyer,Quote.Seller,Quote.Source,Quote.Destination,Quote.Item,Quote.Quantity,Quote.TotalCredits,Receipt,ECommercePurpose::MarketSale))
    {State=Before;Reason=INVTEXT("Trade unavailable: stock, space, credits or receipt changed. Nothing was exchanged.");return false;}
    if(!Persist())
    {State=Before;Reason=INVTEXT("Could not save the trade. Cargo and credits were restored.");return false;}
    Reason=INVTEXT("Trade completed and saved.");return true;
}
bool FStationServices::TradeMarket(UGameInstance* Instance,FName Item,double Quantity,bool Buy,int64 ExpectedTotal,FGuid Receipt,FText& Reason)
{
    FStationMarketQuote Quote;if(!QuoteMarket(Instance,Item,Quantity,Buy,Quote)){Reason=Quote.Reason;return false;}
    if(Quote.TotalCredits!=ExpectedTotal){Reason=INVTEXT("The market quote changed. Review the new price and try again.");return false;}
    auto* Players=Instance->GetSubsystem<UStarshatterPlayerSubsystem>();if(!Players)return false;
    const bool Saved=CommitMarketTrade(Sim::GetSim()->GetEconomy(),Quote,Receipt,[Players](){Sim::GetSim()->GetPlayerShip()->ApplyCargoInventoryToManifest();return Players->SaveActivePlayerProgress();},Reason);
    if(!Saved){Sim::GetSim()->GetPlayerShip()->ApplyCargoInventoryToManifest();return false;}
    Sim::GetSim()->GetPlayerShip()->ApplyCargoInventoryToManifest();return true;
}
int64 FStationServices::RepairQuote(Ship& Craft)
{
    if (!Craft.Design()) return 0;
    double Cost = FMath::Max(0.0, double(Craft.Design()->integrity) - Craft.GetIntegrity());
    auto& Systems = Craft.GetSystems();
    for (int Index = 0; Index < Systems.size(); ++Index)
    {
        auto* System = Systems[Index]; if (!System) continue;
        auto& Parts = System->GetComponents();
        for (int PartIndex = 0; PartIndex < Parts.size(); ++PartIndex)
        {
            const auto* Part = Parts[PartIndex]; if (!Part) continue;
            const double Damage = FMath::Clamp(1.0 - Part->Availability() / 100.0, 0.0, 1.0);
            if (Damage > 0 || Part->IsJerried() || Part->GetStatus() != SYSTEM_STATUS::NOMINAL)
                Cost += FMath::Max(1.0, double(Part->Cost()) * FMath::Max(Damage, Part->IsJerried() ? .1 : .01));
        }
        if (Parts.size() == 0 && System->GetAvailability() < 100)
            Cost += FMath::Max(1.0, System->GetMaxValue() * (1.0 - System->GetAvailability() / 100.0));
    }
    return FMath::IsFinite(Cost) ? FMath::CeilToInt64(FMath::Clamp(Cost, 0.0, 1.e15)) : MAX_int64;
}
int64 FStationServices::FullRepairCost()
{
    return Offers(ESSWStationService::Repair) ? RepairQuote(*Sim::GetSim()->GetPlayerShip()) : 0;
}
bool FStationServices::RepairAll(UGameInstance* Instance, FText& Reason)
{
    const int64 Cost = FullRepairCost();
    if (!Instance || !IsDocked() || Cost <= 0) { Reason = INVTEXT("Dock with a damaged fighter or ship to use the repair service."); return false; }
    return FOwnedHullCondition::RepairAndSave(Instance,*Sim::GetSim()->GetPlayerShip(),Cost,Reason);
}
