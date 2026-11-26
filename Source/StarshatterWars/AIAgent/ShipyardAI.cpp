#include "ShipyardAI.h"
#include "ShipEquipment.h"
#include "SerializedHullStockAI.h"
#include "PlayerCharacter.h"
#include "SkillsSubsystem.h"
#include "EconomyAI.h"
#include "EconomyRuntime.h"
bool ShipyardAI::Eligible(const FS_PlayerGameInfo& Pilot,const FShipDesign& Hull,FText& Reason)
{
    if(Hull.Hidden || Hull.BaseCost<=0 || (Hull.Category!=EShipCategory::Fighter && Hull.Category!=EShipCategory::Shuttle && Hull.Category!=EShipCategory::Transport && Hull.Category!=EShipCategory::CapitalShip))
    {Reason=INVTEXT("This design is not for sale.");return false;}
    if(Pilot.Rank<PlayerCharacter::HullRankRequired(Hull.ShipType,Hull.ShipName)){Reason=INVTEXT("Your rank does not permit this hull.");return false;}
    const FString Empire=StaticEnum<EEMPIRE_NAME>()->GetNameStringByValue(int64(Pilot.Empire));
    if(Hull.ShipEmpire!=EShipEmpire::All && Hull.ShipEmpire!=EShipEmpire::Neutral && StaticEnum<EShipEmpire>()->GetNameStringByValue(int64(Hull.ShipEmpire))!=Empire)
    {Reason=INVTEXT("This hull belongs to another empire.");return false;}
    const FName Skill=USkillsSubsystem::RequiredHullSkill(Hull.ShipType);
    if(!Skill.IsNone() && FSkillTrainingRules::Level(Pilot.Skills.Find(Skill))<1){Reason=INVTEXT("Train the required hull-operation skill first.");return false;}
    return true;
}
double ShipyardAI::Stock(const FEconomyState& State,FName Inventory,FName Design)
{
    const auto* Store=State.Inventories.Find(Inventory);return Store && Store->bOperational && Store->bHullStorage?FEconomyLedger::Available(State,Inventory,DataTableUtils::MakeValidName(Design.ToString().ToLower()+TEXT(" assembled hull"))):0;
}
bool ShipyardAI::Purchase(FS_PlayerGameInfo& Pilot,const FShipDesign& Hull,FName Inventory,const FString& Location,FGuid Receipt,FText& Reason,FGuid OfferedHullId)
{
    if(!Receipt.IsValid() || !Eligible(Pilot,Hull,Reason))return false;
    if(Pilot.Economy.CommittedTransactions.Contains(Receipt))
    {
        FGuid Delivered=SerializedHullStockAI::Purchased(Pilot.Economy,Receipt);
        if(!Delivered.IsValid())Delivered=Receipt; // Pre-serialized purchases.
        return (!OfferedHullId.IsValid() || OfferedHullId==Delivered) && Pilot.HangarShips.ContainsByPredicate([&](const auto& H){return H.HullId==Delivered && H.Design==FName(*Hull.ShipName);});
    }
    const auto* Store=Pilot.Economy.Inventories.Find(Inventory);
    if(!Store || Store->Owner!=FEconomyRuntime::IndustryOwner() || Stock(Pilot.Economy,Inventory,FName(*Hull.ShipName))<1)
    {Reason=INVTEXT("No completed hull is available at this shipyard.");return false;}
    if(Pilot.Economy.Wallets.FindRef(FEconomyLedger::Pilot())<Hull.BaseCost){Reason=INVTEXT("Insufficient credits.");return false;}
    const auto Before=Pilot;const FName Seller=Store->TradingOwner.IsNone()?EconomyAI::NPCAccount(Inventory):Store->TradingOwner;
    const FName Item=DataTableUtils::MakeValidName(Hull.ShipName.ToLower()+TEXT(" assembled hull"));
    if(!OfferedHullId.IsValid())
    {
        SerializedHullStockAI::Ensure(Pilot.Economy,Inventory,FName(*Hull.ShipName),FMath::FloorToInt(FEconomyLedger::Quantity(Pilot.Economy,Inventory,Item)));
        const auto Offers=SerializedHullStockAI::Available(Pilot.Economy,Inventory,FName(*Hull.ShipName),FMath::FloorToInt(Stock(Pilot.Economy,Inventory,FName(*Hull.ShipName))));
        if(!Offers.IsEmpty())OfferedHullId=Offers[0];
    }
    if(!SerializedHullStockAI::Reserve(Pilot.Economy,OfferedHullId,Receipt,Inventory,FName(*Hull.ShipName)) ||
        !FEconomyLedger::Withdraw(Pilot.Economy,Store->Owner,Inventory,Item,1) || !FEconomyLedger::Pay(Pilot.Economy,FEconomyLedger::Pilot(),Seller,Hull.BaseCost,Receipt,TEXT("Shipyard hull purchase"),ECommercePurpose::Purchase))
    {Pilot=Before;Reason=INVTEXT("The purchase could not be committed. Stock and credits retained.");return false;}
    if(!SerializedHullStockAI::Deliver(Pilot.Economy,OfferedHullId,Receipt,Location))
    {Pilot=Before;Reason=INVTEXT("The selected hull could not be delivered. Stock and credits retained.");return false;}
    auto& Entry=Pilot.HangarShips.AddDefaulted_GetRef();Entry.HullId=OfferedHullId;Entry.Design=FName(*Hull.ShipName);Entry.Name=Hull.DisplayName.IsEmpty()?Hull.ShipName:Hull.DisplayName;Entry.HullValue=Hull.BaseCost;Entry.Location=Location;
    if(!Hull.Loadout.IsEmpty()){Entry.LoadoutName=Hull.Loadout[0].Name;Entry.LoadoutStations=Hull.Loadout[0].Stations;}
    FShipEquipment::InitializePurchase(Entry,Hull);
    auto& Payment=Pilot.Economy.Journal.Last();Payment.Item=Entry.Design;Payment.Quantity=1;Payment.UnitPrice=Hull.BaseCost;Payment.Reference=Entry.HullId;Payment.Source=Inventory;Payment.Destination=FName(*Location);Payment.Category=EWalletTransactionCategory::Shipyard;
    EconomyAI::RecordTrade(Pilot.Economy,Pilot.Economy.Inventories.FindChecked(Inventory).System,Item,1,Hull.BaseCost);
    Reason=INVTEXT("Hull purchased and delivered to your hangar at this station.");return true;
}
