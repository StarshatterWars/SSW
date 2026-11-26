#include "ShipInsuranceSubsystem.h"
#include "OwnedCommandAI.h"
#include "GalaxyTrafficSubsystem.h"
#include "ShipCommerceAI.h"
#include "EconomyAI.h"
#include "SimDataEvents.h"
#include "ShipEquipment.h"
#include "CampaignFleet.h"
#include "Campaign.h"
#include "SerializedHullStockAI.h"
#include "StarterCraftOwnership.h"
#include "ShipStoreAI.h"
#include "ShipStoreSubsystem.h"
#include "OrdnanceMagazine.h"
#include "Weapon.h"
#include "WeaponGroup.h"
#include "AgentAI.h"
#include "SSWGameInstance.h"
#include "ShipyardAI.h"
#include "CarrierFighterSalesAI.h"
#include "Hangar.h"
#include "StationServices.h"
#include "StarSystem.h"
#include "EconomyRuntime.h"
#include "StationServiceCatalog.h"
#include "TransactionAI.h"
#include "OwnedHullCondition.h"
#include "OwnedHullLocation.h"
#include "PlayerHullLoss.h"
#include "StarshatterPlayerSubsystem.h"
#include "Engine/GameInstance.h"
#include "ShipDesignRegistry.h"
#include "Ship.h"
#include "ShipDesign.h"
#include "SimSystem.h"
#include "Power.h"
#include "SkillsSubsystem.h"
#include "SimComponent.h"
#include "StarshatterSystemDesignSubsystem.h"
#include "PlayerCharacter.h"
#include "Sim.h"
#include "SimRegion.h"
#include "Mission.h"

namespace
{
    constexpr int64 PolicySeconds = 12LL * 7 * 24 * 60 * 60;
    constexpr int64 MaximumValue = 1000000000000000LL;
    int64 Value(const FPlayerHangarShip& Ship)
    {
        return FMath::Clamp(Ship.HullValue, 0LL, MaximumValue) +
            FMath::Clamp(Ship.InstalledComponentValue, 0LL, MaximumValue);
    }
}

FString UShipInsuranceSubsystem::TierName(EShipInsuranceTier Tier)
{
    const auto* Enum = StaticEnum<EShipInsuranceTier>();
    return Enum->GetDisplayNameTextByValue(static_cast<int64>(Tier)).ToString();
}

FShipInsuranceQuote UShipInsuranceSubsystem::Quote(const FPlayerHangarShip& Ship, EShipInsuranceTier Tier)
{
    FShipInsuranceQuote Result;
    Result.Tier = Ship.bCashPurchased ? EShipInsuranceTier::Lifetime : Tier;
    const int32 Level = static_cast<int32>(Result.Tier);
    Result.Coverage = Result.Tier == EShipInsuranceTier::Lifetime ? 1.f :
        FMath::Clamp(0.4f + Level * 0.1f, 0.4f, 1.f);
    const int64 InsuredValue = Value(Ship);
    // EVE client insuranceConst: premium = value * (coverage - .4) * .5.
    Result.Premium = Result.Tier == EShipInsuranceTier::Lifetime ? 0 :
        (InsuredValue * FMath::Clamp(Level, 0, 6) + 19) / 20;
    Result.Payout = Result.Tier == EShipInsuranceTier::Lifetime ? InsuredValue :
        InsuredValue * FMath::Clamp(4 + Level, 4, 10) / 10;
    return Result;
}

EShipInsuranceTier UShipInsuranceSubsystem::EffectiveTier(const FPlayerHangarShip& Ship, int64 Now)
{
    if (Ship.bCashPurchased) return EShipInsuranceTier::Lifetime;
    if (Ship.Insurance.bSettled || Ship.Insurance.ExpiresUtc <= Now)
        return EShipInsuranceTier::Uninsured;
    return Ship.Insurance.Tier;
}

const TArray<FPlayerHangarShip>& UShipInsuranceSubsystem::GetShips() const
{
    static const TArray<FPlayerHangarShip> Empty;
    const auto* Players = GetGameInstance()->GetSubsystem<UStarshatterPlayerSubsystem>();
    return Players ? Players->GetPlayerInfo().HangarShips : Empty;
}

const FPlayerHangarShip* UShipInsuranceSubsystem::FindShip(FGuid Hull) const
{
    return GetShips().FindByPredicate([Hull](const auto& Entry) { return Entry.HullId == Hull; });
}

int64 UShipInsuranceSubsystem::GetBalance() const
{
    auto* Players = GetGameInstance()->GetSubsystem<UStarshatterPlayerSubsystem>();
    return Players ? Players->GetEconomy().Wallets.FindRef(FEconomyLedger::Pilot()) : 0;
}

bool UShipInsuranceSubsystem::CanInsure() const
{
    auto* Simulation = Sim::GetSim();
    const auto* Craft = Simulation ? Simulation->GetPlayerShip() : nullptr;
    return Simulation && Simulation->HasSimulationAuthority() && Craft && !Craft->IsDead() &&
        Craft->GetFlightPhase() <= EOPSMode::ALERT;
}

void UShipInsuranceSubsystem::RefreshAssignment()
{
    auto* Players = GetGameInstance()->GetSubsystem<UStarshatterPlayerSubsystem>();
    auto* Simulation = Sim::GetSim();
    auto* Craft = Simulation ? Simulation->GetPlayerShip() : nullptr;
    if (!Players || !Craft || !Craft->Design() || !Simulation->HasSimulationAuthority() || Simulation->IsNetworked()) return;
    auto& Pilot = Players->GetMutablePlayerInfo();
    StarterCraftOwnership::Normalize(Pilot);
    for (auto& Hull : Pilot.HangarShips)
    {
        const auto* Base = Craft->GetCarrier();
        const FString Dock = OwnedHullLocation::Dock(*Craft);
        if (Base && Dock.StartsWith(TEXT("Carrier / ")) && Hull.Location.Equals(UTF8_TO_TCHAR(Base->GetName()), ESearchCase::IgnoreCase)) Hull.Location = Dock;
        else OwnedHullLocation::RecoverPurchase(Pilot, Hull);
    }
    const FName Design(UTF8_TO_TCHAR(Craft->Design()->name));
    auto* Current = Pilot.HangarShips.FindByPredicate([&Pilot](const auto& Item)
        { return Item.HullId == Pilot.AssignedHangarHull; });
    const bool FreeFlight=Simulation->GetMission() && Simulation->GetMission()->IsFreeFlight();
    if (Current && FOwnedHullCondition::IsOwnedLive(*Current,Pilot.AssignedHangarHull,Design,Craft->GetCargoInventoryKey(),FreeFlight))
        FOwnedHullCondition::Capture(*Craft,*Current);
    if (Craft->IsDead() || Craft->IsDying())
    {
        // Do not attribute a newly provisioned/loaner death to a merely selected owned hull.
        if (Current && (Current->bAssigned || FOwnedHullCondition::IsOwnedLive(*Current,Pilot.AssignedHangarHull,
            Design,Craft->GetCargoInventoryKey(),FreeFlight)))
        { FText Ignored;FOwnedActionSelection W;W.Action=EOwnedCommand::Loss;W.Hull=Current->HullId;RequestOwnedAction(W,{},Ignored); }
        return;
    }
    // Mission-assigned craft never inherit an owned hull solely because their designs match.
    if (!FreeFlight && Current && !Current->bAssigned && Design!=FName(TEXT("Falcon"))) Current=nullptr;
    if (FreeFlight && Current && !Current->bAssigned &&
        !FOwnedHullCondition::IsOwnedLive(*Current, Pilot.AssignedHangarHull, Design, Craft->GetCargoInventoryKey(), true) &&
        !OwnedHullLocation::CanProvision(*Current, *Simulation->GetMission(), true)) Current = nullptr;
    if (FreeFlight)
        if (auto* Selected=Pilot.HangarShips.FindByPredicate([&](const auto& Row)
            {return Row.HullId==Pilot.SelectedHangarHull && !Row.bAssigned && !Row.bLost && !Row.bSold && Row.Design==Design && OwnedHullLocation::IsAt(Row,OwnedHullLocation::Dock(*Craft));}))
        { Current=Selected; Pilot.AssignedHangarHull=Selected->HullId; }
    // The basic trainer is the pilot's starter purchase, not another persistent
    // mission loaner each time a training assignment provisions a Falcon.
    if(Design==FName(TEXT("Falcon")))
        if(auto* Owned=Pilot.HangarShips.FindByPredicate([&](const auto& Hull)
            {return Hull.Design==Design && !Hull.bAssigned && !Hull.bLost && !Hull.bSold && !Hull.RentalId.IsValid() && Hull.HullId==Pilot.AssignedHangarHull;}))Current=Owned;
    if (!Current || Current->Design != Design || Current->bLost || Current->bSold)
    {
        FPlayerHangarShip NewShip;
        NewShip.HullId = Craft->GetHullId();
        NewShip.Design = Design;
        NewShip.bAssigned = true;
        if (const auto* Row = ShipDesignRegistry::Find(Design)) NewShip.HullValue = Row->BaseCost;
        Pilot.AssignedHangarHull = NewShip.HullId;
        Pilot.HangarShips.Add(NewShip);
        Current = &Pilot.HangarShips.Last();
    }
    const bool RestoreCondition=FreeFlight && !Current->bAssigned &&
        Craft->GetCargoInventoryKey()!=FName(*(TEXT("Ship/")+Current->HullId.ToString(EGuidFormats::Digits)));
    Craft->BindCargoInventory(Current->HullId);
    if (RestoreCondition) { bDockedConditionSaved=false; LastConditionHull=Current->HullId; }
    if (Craft->GetFlightPhase() <= EOPSMode::ALERT)
    {
        if (bRestoreLoadout && !Current->LoadoutStations.IsEmpty())
        {
            Craft->ApplyDockedLoadout(Current->LoadoutStations);
            if(FreeFlight)FOwnedHullCondition::ApplyAmmo(*Craft,*Current);
        }
        bRestoreLoadout = false;
        if (int* Loadout = Craft->GetLoadout())
        {
            Current->LoadoutStations.Reset();
            for (int32 Index = 0; Index < Craft->Design()->hardpoints.size(); ++Index)
                Current->LoadoutStations.Add(Loadout[Index]);
        }
    }
    if(auto* Equipment=Craft->GetEquipment())
    {
        Equipment->Apply(*Current);
        Equipment->Capture(*Current);
        FShipEquipment::SeparateValue(*Current);
        FShipEquipment::Revalue(*Current);
    }
    if (FreeFlight && !Current->bAssigned)
    {
        if (RestoreCondition)
        {
            if (Current->bHasCondition) FOwnedHullCondition::Apply(*Craft,*Current);
            else FOwnedHullCondition::ApplyPristine(*Craft); // Backward-compatible/new hull baseline.
        }
        FOwnedHullCondition::Capture(*Craft,*Current);
    }
    if(Current->bAssigned || Current->Name.IsEmpty())Current->Name = UTF8_TO_TCHAR(Craft->GetObserverName());
    const FString Place = OwnedHullLocation::Current(*Craft);
    if (!Place.IsEmpty()) Current->Location = Place;
    Players->MarkDirty();
    if(bNeedsMagazineIssue && !FreeFlight && Craft->GetFlightPhase()<=EOPSMode::ALERT)ProvisionMissionMagazine();
}

FGuid UShipInsuranceSubsystem::RegisterOwnedShip(FName Design, const FString& Name,
    int64 HullValue, int64 ComponentValue, bool bPurchasedWithCash)
{
    auto* Players = GetGameInstance()->GetSubsystem<UStarshatterPlayerSubsystem>();
    auto* Simulation = Sim::GetSim();
    if (!Players || (Simulation && !Simulation->HasSimulationAuthority()) ||
        Design.IsNone() || !ShipDesignRegistry::Find(Design) || HullValue < 0 || ComponentValue < 0) return {};
    const auto Previous = Players->GetPlayerInfo();
    auto& Entry = Players->GetMutablePlayerInfo().HangarShips.AddDefaulted_GetRef();
    Entry.HullId = FHullRegistry::Allocate();
    Entry.Design = Design;
    Entry.Name = Name;
    if (auto* Craft = Simulation ? Simulation->GetPlayerShip() : nullptr) Entry.Location = OwnedHullLocation::Dock(*Craft);
    Entry.HullValue = FMath::Min(HullValue, MaximumValue);
    Entry.InstalledComponentValue = FMath::Min(ComponentValue, MaximumValue);
    Entry.bCashPurchased = bPurchasedWithCash;
    if (const auto* Row = ShipDesignRegistry::Find(Design); Row && !Row->Loadout.IsEmpty())
    {
        const auto& DefaultLoadout = Row->Loadout[0];
        Entry.LoadoutName = DefaultLoadout.Name;
        Entry.LoadoutStations = DefaultLoadout.Stations;
    }
    if (bPurchasedWithCash)
    {
        Entry.Insurance.Tier = EShipInsuranceTier::Lifetime;
        Entry.Insurance.InsuredValue = Value(Entry);
    }
    const FGuid Id = Entry.HullId;
    if (!Players->SaveActivePlayerProgress())
    {
        Players->GetMutablePlayerInfo() = Previous;
        return {};
    }
    if(Simulation)Simulation->HullRegistry.Register(Id,Design,Name,FEconomyLedger::Pilot(),
        Simulation->GetPlayerShip()?Simulation->GetPlayerShip()->GetIFF():0);
    return Id;
}

bool UShipInsuranceSubsystem::PurchasePolicy(FGuid Hull, EShipInsuranceTier Tier, FText& Result)
{
    auto* Players = GetGameInstance()->GetSubsystem<UStarshatterPlayerSubsystem>();
    if (!Players || !CanModifyHull(Hull))
    { Result = INVTEXT("Dock to purchase insurance."); return false; }
    auto& Pilot = Players->GetMutablePlayerInfo();
    auto* Entry = Pilot.HangarShips.FindByPredicate([Hull](const auto& Item) { return Item.HullId == Hull; });
    if (!Entry || Entry->bLost || Entry->bSold || Entry->RentalId.IsValid() || Entry->bAssigned || Entry->bCashPurchased ||
        Tier < EShipInsuranceTier::Basic || Tier > EShipInsuranceTier::Platinum)
    { Result = INVTEXT("This hull cannot purchase that policy."); return false; }
    const int64 Now = FDateTime::UtcNow().ToUnixTimestamp();
    if (EffectiveTier(*Entry, Now) != EShipInsuranceTier::Uninsured)
    { Result = INVTEXT("An active policy already covers this hull."); return false; }
    const auto Offer = Quote(*Entry, Tier);
    if (Offer.Payout <= 0 || GetBalance() < Offer.Premium)
    { Result = INVTEXT("Insufficient credits for this premium."); return false; }
    const auto Previous = Pilot;
    if (!FEconomyLedger::Pay(Players->GetEconomy(), FEconomyLedger::Pilot(), TEXT("Insurance fund"), Offer.Premium, FGuid::NewGuid(), TEXT("Insurance premium"),ECommercePurpose::Service)) return false;
    auto& PremiumReceipt = Players->GetEconomy().Journal.Last(); PremiumReceipt.Item = Entry->Design;
    PremiumReceipt.Quantity = 1; PremiumReceipt.UnitPrice = double(Offer.Premium); PremiumReceipt.Reference = Hull;
    Entry->Insurance.Tier = Tier;
    Entry->Insurance.StartsUtc = Now;
    Entry->Insurance.ExpiresUtc = Now + PolicySeconds;
    Entry->Insurance.InsuredValue = Value(*Entry);
    Entry->Insurance.bSettled = false;
    if (!Players->SaveActivePlayerProgress())
    { Pilot = Previous; Result = INVTEXT("Could not save the policy. Please retry."); return false; }
    Result = INVTEXT("Insurance active for 12 weeks.");
    return true;
}

bool UShipInsuranceSubsystem::SettleLoss(FGuid Hull, FText& Result)
{
    auto* Players = GetGameInstance()->GetSubsystem<UStarshatterPlayerSubsystem>();
    if (!Players) return false;
    auto& Pilot = Players->GetMutablePlayerInfo();
    auto* Entry = Pilot.HangarShips.FindByPredicate([Hull](const auto& Item) { return Item.HullId == Hull; });
    auto* Simulation = Sim::GetSim();
    auto* Craft = Simulation ? Simulation->GetPlayerShip() : nullptr;
    if (!Entry || Entry->bLost || !Craft || (!Craft->IsDead() && !Craft->IsDying()) ||
        Pilot.AssignedHangarHull != Hull) return false;
    if(Entry->RentalId.IsValid())return ShipCommerceAI::SettleRentalLoss(Pilot,Hull,FDateTime::UtcNow().ToUnixTimestamp(),[Players](){return Players->SaveActivePlayerProgress();},Result);
    const auto Previous = Pilot;
    auto Snapshot = *Entry;
    const auto Tier = EffectiveTier(*Entry, FDateTime::UtcNow().ToUnixTimestamp());
    if (Tier != EShipInsuranceTier::Uninsured) Snapshot.HullValue = Entry->Insurance.InsuredValue;
    if (Tier != EShipInsuranceTier::Uninsured) Snapshot.InstalledComponentValue = 0;
    const int64 Payout = Entry->bAssigned ? 0 : Quote(Snapshot, Tier).Payout;
    auto& Economy = Players->GetEconomy();
    // NPC underwriter capitalization, once per career. Claims still use normal wallet transfers.
    if (!Economy.InitializedMarkets.Contains(TEXT("Insurance fund")))
    {
        const int64 Fund = Economy.Wallets.FindRef(TEXT("Insurance fund"));
        if (Fund < 0 || Fund > MAX_int64 - MaximumValue ||
            !TransactionAI::ReconcileExternalBalance(Economy, TEXT("Insurance fund"), Fund + MaximumValue,
                TEXT("Insurance fund capitalization"), TEXT("NPC underwriter reserves"), EWalletTransactionCategory::OpeningBalance))
        { Pilot = Previous; Result = INVTEXT("The insurance fund is unavailable. No claim was charged or settled."); return false; }
        Economy.InitializedMarkets.Add(TEXT("Insurance fund"));
    }
    if (!FEconomyLedger::Pay(Economy, TEXT("Insurance fund"), FEconomyLedger::Pilot(), Payout, Hull, TEXT("Insurance claim")))
    { Pilot = Previous; Result = INVTEXT("The claim could not be paid. No policy or wallet changes were kept."); return false; }
    auto& ClaimReceipt = Economy.Journal.Last(); ClaimReceipt.Item = Entry->Design; ClaimReceipt.Quantity = 1;
    ClaimReceipt.UnitPrice = double(Payout); ClaimReceipt.Reference = Hull;
    if(!FPlayerHullLoss::Finalize(Pilot,Hull))
    { Pilot=Previous;Result=INVTEXT("The lost hull could not be finalized. No claim or cargo changes were kept.");return false; }
    if (!Players->SaveActivePlayerProgress())
    { Pilot = Previous; return false; }
    Result = FText::Format(INVTEXT("Insurance paid {0} credits."), FText::AsNumber(Payout));
    return true;
}

bool UShipInsuranceSubsystem::IsSelected(FGuid Hull) const
{
    const auto* Players = GetGameInstance()->GetSubsystem<UStarshatterPlayerSubsystem>();
    return Players && Players->GetPlayerInfo().SelectedHangarHull == Hull;
}

bool UShipInsuranceSubsystem::SelectHull(FGuid Hull, FText& Result)
{
    RefreshAssignment();
    auto* Players = GetGameInstance()->GetSubsystem<UStarshatterPlayerSubsystem>();
    const auto* Entry = FindShip(Hull);
    if (!Players || !CanModifyHull(Hull) || !Entry || !ShipCommerceAI::RentalMayLaunch(Players->GetPlayerInfo(),*Entry,FDateTime::UtcNow().ToUnixTimestamp()))
    { Result = INVTEXT("Dock to select an available hull."); return false; }
    auto* Simulation = Sim::GetSim(); auto* Craft = Simulation ? Simulation->GetPlayerShip() : nullptr;
    if (!Craft || !OwnedHullLocation::IsAt(*Entry, OwnedHullLocation::Dock(*Craft)))
    { Result = FText::Format(INVTEXT("This craft is stored at {0}. Travel there to select it."), FText::FromString(Entry->Location)); return false; }
    const auto* Design = ShipDesignRegistry::Find(Entry->Design);
    const auto* PilotCharacter = PlayerCharacter::GetCurrentPlayer();
    if (!Design || !PilotCharacter || PilotCharacter->GetRank() < PlayerCharacter::HullRankRequired(Design->ShipType,Design->ShipName))
    { Result = INVTEXT("Your rank does not permit selecting this hull."); return false; }
    const auto* EmpireEnum = StaticEnum<EShipEmpire>();
    const int64 Empire = EmpireEnum->GetValueByNameString(
        StaticEnum<EEMPIRE_NAME>()->GetNameStringByValue(static_cast<int64>(PilotCharacter->GetEmpire())));
    if (Design->ShipEmpire != EShipEmpire::All && static_cast<int64>(Design->ShipEmpire) != Empire)
    { Result = INVTEXT("This hull belongs to another empire."); return false; }
    const FName RequiredSkill=USkillsSubsystem::RequiredHullSkill(Design->ShipType);
    const auto* Skills=GetGameInstance()->GetSubsystem<USkillsSubsystem>();
    if (!RequiredSkill.IsNone() && (!Skills || !Skills->HasSkill(RequiredSkill)))
    {
        const auto* Skill=Skills ? Skills->GetCatalog().Find(RequiredSkill) : nullptr;
        Result=FText::Format(INVTEXT("Rank alone is not enough. Complete {0} to select this hull."),
            Skill ? FText::FromString(Skill->DisplayName) : FText::FromName(RequiredSkill)); return false;
    }
    const auto Previous = Players->GetPlayerInfo();
    Players->GetMutablePlayerInfo().SelectedHangarHull = Hull;
    if (!Players->SaveActivePlayerProgress())
    { Players->GetMutablePlayerInfo() = Previous; Result = INVTEXT("Could not save hull selection."); return false; }
    RefreshAssignment();
    Result = INVTEXT("Hull selected for Free Verse. Missions use their assigned craft.");
    return true;
}

bool UShipInsuranceSubsystem::ValidateFitting(FGuid Hull, int32 Slot, int32 Level, FText& Result) const
{
    auto* Simulation = Sim::GetSim();
    auto* Craft = Simulation ? Simulation->GetPlayerShip() : nullptr;
    const auto* Entry = FindShip(Hull);
    const auto* Catalog = GetGameInstance()->GetSubsystem<UStarshatterSystemDesignSubsystem>();
    if (!Craft || !Entry || Entry->RentalId.IsValid() || !Catalog || !CanModifyHull(Hull) || !Entry->Components.IsValidIndex(Slot))
    { Result = INVTEXT("Dock with this hull to fit a component."); return false; }
    const auto* Base=Craft->GetCarrier();
    if (!Base || !Base->Design() || !FStationServiceCatalog::Get().HasService(
        FName(UTF8_TO_TCHAR(Base->Design()->name)),FName(UTF8_TO_TCHAR(Base->GetName())),ESSWStationService::Refit))
    { Result=INVTEXT("Component refits require a shipyard fitting service.");return false; }
    const auto& Fitted = Entry->Components[Slot];
    const auto Variant = Catalog->FindComponentGrade(Fitted.SystemDesign, Fitted.Component, Level);
    auto& Systems = Craft->GetSystems();
    if (!Variant || Fitted.SystemIndex < 0 || Fitted.SystemIndex >= Systems.size() || !Systems[Fitted.SystemIndex])
    { Result = INVTEXT("Component is incompatible with this slot."); return false; }
    auto* Target = Systems[Fitted.SystemIndex]; auto& Parts = Target->GetComponents();
    if (Fitted.ComponentIndex < 0 || Fitted.ComponentIndex >= Parts.size() || !Parts[Fitted.ComponentIndex]) return false;
    const auto* Current = Parts[Fitted.ComponentIndex];
    const auto Calibrated=FShipEquipment::GradeDesign(Fitted.SystemDesign,Fitted.Component,Level);
    if(!Calibrated){Result=INVTEXT("Component calibration is unavailable.");return false;}
    const double NewPerformance = FMath::Pow(Calibrated->performance_multiplier / Current->PerformanceMultiplier(), 1.0 / Parts.size());
    const double NewDemand = FMath::Pow(Calibrated->power_demand_multiplier / Current->PowerDemandMultiplier(), 1.0 / Parts.size());
    auto& Reactors = Craft->GetReactors();
    for (int32 Index = 0; Index < Reactors.size(); ++Index)
    {
        auto* Reactor = Reactors[Index]; if (!Reactor) continue;
        const double BeforeSupply = Reactor->GetRatedOutput();
        const double AfterSupply = BeforeSupply * (Reactor == Target ? NewPerformance : 1.0);
        double BeforeDemand = 0, AfterDemand = 0;
        auto& Clients = Reactor->Clients();
        for (int32 Client = 0; Client < Clients.size(); ++Client)
            if (Clients[Client])
            {
                const double Demand = Clients[Client]->GetRatedDemand();
                BeforeDemand += Demand; AfterDemand += Demand * (Clients[Client] == Target ? NewDemand : 1.0);
            }
        if (AfterSupply - AfterDemand + UE_SMALL_NUMBER < FMath::Min(0.0, BeforeSupply - BeforeDemand))
        { Result = INVTEXT("This fitting exceeds the reactor power budget."); return false; }
    }
    Result = INVTEXT("Compatible fitting. Power budget valid."); return true;
}

int32 UShipInsuranceSubsystem::StoredComponentCount(FGuid Hull,int32 Slot,int32 Level) const
{
    const auto* Players=GetGameInstance()->GetSubsystem<UStarshatterPlayerSubsystem>();
    auto* Simulation=Sim::GetSim();const auto* Craft=Simulation?Simulation->GetPlayerShip():nullptr;
    const auto* Entry=FindShip(Hull);
    if(!Players || !Craft || !Entry || !Entry->Components.IsValidIndex(Slot))return 0;
    const auto& Part=Entry->Components[Slot];
    return ShipStoreAI::Stored(Players->GetPlayerInfo(),ShipStoreAI::ComponentId(Part.SystemDesign,Part.Component,Level),OwnedHullLocation::Dock(*Craft));
}

bool UShipInsuranceSubsystem::FitComponent(FGuid Hull, int32 Slot, int32 Level, FText& Result)
{
    auto* Players = GetGameInstance()->GetSubsystem<UStarshatterPlayerSubsystem>();
    auto* Catalog = GetGameInstance()->GetSubsystem<UStarshatterSystemDesignSubsystem>();
    auto* Simulation = Sim::GetSim();
    auto* Craft = Simulation ? Simulation->GetPlayerShip() : nullptr;
    if (!Players || !Catalog || !Craft || !CanModifyHull(Hull) ||
        Players->GetPlayerInfo().AssignedHangarHull != Hull)
    { Result = INVTEXT("Dock with the assigned hull to change its fitting."); return false; }
    auto& Pilot = Players->GetMutablePlayerInfo();
    auto* Entry = Pilot.HangarShips.FindByPredicate([Hull](const auto& Item) { return Item.HullId == Hull; });
    if (!Entry || Entry->bLost || !Entry->Components.IsValidIndex(Slot)) return false;
    const auto& Fitted = Entry->Components[Slot];
    const auto Variant = Catalog->FindComponentGrade(Fitted.SystemDesign, Fitted.Component, Level);
    const auto* Design = Catalog->FindDesign(Fitted.SystemDesign);
    const auto* Base = Design ? Design->Components.FindByPredicate([&](const auto& Item)
        { return FName(*Item.Name) == Fitted.Component; }) : nullptr;
    auto& Systems = Craft->GetSystems();
    if (!Variant || !Base || Fitted.Level == Level || Fitted.SystemIndex < 0 ||
        Fitted.SystemIndex >= Systems.size() || !Systems[Fitted.SystemIndex])
    { Result = INVTEXT("Choose a compatible component level."); return false; }
    auto& Components = Systems[Fitted.SystemIndex]->GetComponents();
    if (Fitted.ComponentIndex < 0 || Fitted.ComponentIndex >= Components.size() || !Components[Fitted.ComponentIndex])
        return false;
    if (!ValidateFitting(Hull, Slot, Level, Result)) return false;
    const auto Previous = Pilot;
    FPlayerHangarShip PreviousCondition; FOwnedHullCondition::Capture(*Craft,PreviousCondition);
    SimComponent* NativePart=Components[Fitted.ComponentIndex];
    const SimComponent PreviousPart(*NativePart);
    FHangarComponent Incoming;
    if(!ShipStoreAI::TakeComponent(Pilot,Fitted.SystemDesign,Fitted.Component,Level,OwnedHullLocation::Dock(*Craft),Incoming))
    {Result=INVTEXT("Purchase this component grade in Ship Store and keep it at this dock before fitting.");return false;}
    FOwnedHullCondition::Capture(*Craft,*Entry);
    const FHangarComponent Removed=Entry->Components[Slot];
    if(!Entry->bAssigned)ShipStoreAI::StoreComponent(Pilot,Removed,OwnedHullLocation::Dock(*Craft));
    Incoming.SystemIndex=Removed.SystemIndex;Incoming.ComponentIndex=Removed.ComponentIndex;Incoming.SystemName=Removed.SystemName;
    if(!Incoming.bHasCondition)
    {
        Incoming.bHasCondition=true;Incoming.ConditionPercent=100;
        Incoming.ConditionStatus=static_cast<int32>(SYSTEM_STATUS::NOMINAL);
        Incoming.MaintenanceSeconds=0;Incoming.JerryCount=0;Incoming.SpareCount=Variant->Spares;
    }
    Entry->Components[Slot]=Incoming;
    const auto Calibrated=FShipEquipment::GradeDesign(Incoming.SystemDesign,Incoming.Component,Level);
    if(!Calibrated){Pilot=Previous;Result=INVTEXT("Component calibration is unavailable.");return false;}
    const ComponentDesign Native=*Calibrated;
    Incoming.InstalledValue=Variant->BaseCost;Entry->Components[Slot]=Incoming;
    NativePart->InstallDesign(Native);
    FOwnedHullCondition::ApplyComponent(*NativePart,Incoming);
    Systems[Incoming.SystemIndex]->CalcStatus();
    if (!Entry->bAssigned) FOwnedHullCondition::Capture(*Craft,*Entry);
    if (!Players->SaveActivePlayerProgress())
    {
        Pilot=Previous; *NativePart=PreviousPart; FOwnedHullCondition::Apply(*Craft,PreviousCondition);
        Result=INVTEXT("Could not save this fitting. Stored equipment, fitting and condition restored."); return false;
    }
    Result = INVTEXT("Owned component fitted. No second purchase charge; removed owned equipment stays at this dock.");
    RefreshAssignment();
    return true;
}

void UShipInsuranceSubsystem::BeginMission()
{
    bRestoreLoadout = true;
    bNeedsMagazineIssue = true;
    RefreshAssignment();
}

bool UShipInsuranceSubsystem::CanRefitHull(FGuid Hull, FText* Reason) const
{
    const auto Denied = [Reason](const FText& Text) { if (Reason) *Reason = Text; return false; };
    auto* Instance = GetGameInstance();
    const auto* Players = Instance ? Instance->GetSubsystem<UStarshatterPlayerSubsystem>() : nullptr;
    auto* Simulation = Sim::GetSim();
    const auto* Craft = Simulation ? Simulation->GetPlayerShip() : nullptr;
    if (!Players || !Craft || !CanModifyHull(Hull) || (FindShip(Hull) && FindShip(Hull)->RentalId.IsValid()))
        return Denied(INVTEXT("Dock with an available owned hull to refit components."));
    if (Players->GetPlayerInfo().AssignedHangarHull != Hull)
        return Denied(INVTEXT("Select and board this hull while docked before refitting it."));
    const auto* Base = Craft->GetCarrier();
    if (!Base || !Base->Design() || !FStationServiceCatalog::Get().HasService(
        FName(UTF8_TO_TCHAR(Base->Design()->name)), FName(UTF8_TO_TCHAR(Base->GetName())), ESSWStationService::Refit))
        return Denied(INVTEXT("Component refits require a shipyard fitting service."));
    if (Reason) *Reason = INVTEXT("Shipyard fitting service available. Select a component and grade to check its power budget.");
    return true;
}

bool UShipInsuranceSubsystem::CanModifyHull(FGuid Hull) const
{
    if (!CanInsure()) return false;
    const auto* Entry = FindShip(Hull);
    if (!Entry || Entry->bLost || Entry->bSold) return false;
    auto* Live = Sim::GetSim(); auto* Craft = Live ? Live->GetPlayerShip() : nullptr;
    if (!Craft || !OwnedHullLocation::IsAt(*Entry, OwnedHullLocation::Dock(*Craft))) return false;
    auto* Simulation = Sim::GetSim();
    const auto* Mission = Simulation ? Simulation->GetMission() : nullptr;
    const auto* Players = GetGameInstance()->GetSubsystem<UStarshatterPlayerSubsystem>();
    return Players && (!Mission || Mission->IsFreeFlight() || Players->GetPlayerInfo().AssignedHangarHull == Hull);
}

namespace
{
TArray<int32> CurrentStations(Ship& Craft)
{
    TArray<int32> Out;
    if(Craft.Design())for(int32 Index=0;Index<Craft.Design()->hardpoints.size();++Index)Out.Add(Craft.GetLoadout()?Craft.GetLoadout()[Index]:-1);
    return Out;
}
void EmptyMissiles(Ship& Craft)
{
    for(ListIter<WeaponGroup> Groups=Craft.GetWeapons();++Groups;)
        for(ListIter<Weapon> Weapons=Groups->GetWeapons();++Weapons;)
            if(!Weapons->IsPrimary() && Weapons->Ammo()>=0)Weapons->SetAmmo(0);
}
}
void UShipInsuranceSubsystem::ProvisionMissionMagazine()
{
    auto* P=GetGameInstance()->GetSubsystem<UStarshatterPlayerSubsystem>();auto* Simulation=Sim::GetSim();
    auto* Craft=Simulation?Simulation->GetPlayerShip():nullptr;
    if(!P || !Craft || !Craft->Design() || !Craft->GetCarrier() || !Simulation->HasSimulationAuthority() || Simulation->IsNetworked())return;
    bNeedsMagazineIssue=false;
    auto& Pilot=P->GetMutablePlayerInfo();
    auto* Hull=Pilot.HangarShips.FindByPredicate([&](const auto& Row){return Row.HullId==Pilot.AssignedHangarHull;});if(!Hull)return;
    int64 Accepted=0;
    if(auto* Game=Cast<USSWGameInstance>(GetGameInstance()))
        if(const auto* NativePilot=PlayerCharacter::GetCurrentPlayer())
            if(const auto* Assignment=Game->GetTrainingAgent().Find(AgentAI::PilotKey(*NativePilot)))Accepted=Assignment->AcceptedUtc;
    const FName Issue(*FString::Printf(TEXT("Magazine issue/%s/%d/%lld/%s"),*Pilot.Name,
        Simulation->GetMission()?Simulation->GetMission()->GetIdentity():0,Accepted,*Hull->HullId.ToString(EGuidFormats::Digits)));
    if(Pilot.Economy.InitializedMarkets.Contains(Issue))
    {
        if(Hull->OrdnanceIssueKey==Issue)FOwnedHullCondition::ApplyAmmo(*Craft,*Hull);
        Craft->SetMagazineIssue(true);return;
    }
    const auto Before=Pilot;
    const FName Magazine=OrdnanceMagazine::Ensure(Pilot.Economy,*Craft->GetCarrier(),LastLoadoutReason);
    TMap<FName,int32> Wanted;
    if(Magazine.IsNone() || !OrdnanceMagazine::Required(*Craft,CurrentStations(*Craft),Wanted,LastLoadoutReason) ||
        !OrdnanceMagazine::Exchange(Pilot,OwnedHullLocation::Dock(*Craft),Magazine,{},Wanted,LastLoadoutReason))
    {EmptyMissiles(*Craft);return;}
    if(!Craft->ApplyDockedLoadout(CurrentStations(*Craft)))
    {Pilot=Before;EmptyMissiles(*Craft);return;}
    // The profile array may have been copied by rollback; reacquire its element.
    Hull=Pilot.HangarShips.FindByPredicate([&](const auto& Row){return Row.HullId==Pilot.AssignedHangarHull;});
    Hull->OrdnanceIssueKey=Issue;Pilot.Economy.InitializedMarkets.Add(Issue);FOwnedHullCondition::CaptureAmmo(*Craft,*Hull);
    Craft->SetMagazineIssue(true);P->MarkDirty();
    if(!P->SaveActivePlayerProgress()){Pilot=Before;EmptyMissiles(*Craft);LastLoadoutReason=INVTEXT("Magazine issue could not be saved. Missiles were not issued.");}
}

bool UShipInsuranceSubsystem::ApplyStoredLoadout(const TArray<int32>& Choices,const FString& Name,FText& Reason)
{
    auto* P=GetGameInstance()->GetSubsystem<UStarshatterPlayerSubsystem>();auto* Simulation=Sim::GetSim();
    auto* Craft=Simulation?Simulation->GetPlayerShip():nullptr;
    if(!P || !Craft || !Simulation->HasSimulationAuthority() || Simulation->IsNetworked() || !CanModifyHull(P->GetPlayerInfo().AssignedHangarHull))
    {Reason=INVTEXT("Dock with the active assigned fighter to change missile stores.");LastLoadoutReason=Reason;return false;}
    auto& Pilot=P->GetMutablePlayerInfo();const auto Before=Pilot;const auto PreviousStations=CurrentStations(*Craft);
    FPlayerHangarShip PreviousCondition;FOwnedHullCondition::Capture(*Craft,PreviousCondition);
    const bool Issued=Simulation->GetMission() && !Simulation->GetMission()->IsFreeFlight();
    FName Magazine;
    if(Issued && Craft->GetCarrier())Magazine=OrdnanceMagazine::Ensure(Pilot.Economy,*Craft->GetCarrier(),Reason);
    TMap<FName,int32> Wanted;
    if((Issued && Magazine.IsNone()) || !OrdnanceMagazine::Required(*Craft,Choices,Wanted,Reason) ||
        !OrdnanceMagazine::Exchange(Pilot,OwnedHullLocation::Dock(*Craft),Magazine,OrdnanceMagazine::Loaded(*Craft),Wanted,Reason))
    {Pilot=Before;LastLoadoutReason=Reason;return false;}
    if(!Craft->ApplyDockedLoadout(Choices)){Pilot=Before;return false;}
    auto Health=PreviousCondition;
    // Preserve gun ammo and system damage; only the reconciled missile stores are refilled.
    for(ListIter<WeaponGroup> Groups=Craft->GetWeapons();++Groups;)
        for(ListIter<Weapon> Weapons=Groups->GetWeapons();++Weapons;)
            if(!Weapons->IsPrimary()){Health.SavedWeaponAmmo.Remove(Weapons->GetIndex());Health.SavedWeaponTypes.Remove(Weapons->GetIndex());}
    FOwnedHullCondition::Apply(*Craft,Health);
    auto* Hull=Pilot.HangarShips.FindByPredicate([&](const auto& Row){return Row.HullId==Pilot.AssignedHangarHull;});
    if(Hull){Hull->LoadoutStations=Choices;Hull->LoadoutName=Name;FOwnedHullCondition::CaptureAmmo(*Craft,*Hull);}
    if(!Hull || !P->SaveActivePlayerProgress())
    {
        Pilot=Before;Craft->ApplyDockedLoadout(PreviousStations);FOwnedHullCondition::Apply(*Craft,PreviousCondition);
        Reason=INVTEXT("Loadout could not be saved. Aircraft and missile stores restored.");LastLoadoutReason=Reason;return false;
    }
    LastLoadoutReason=Reason;return true;
}

FText UShipInsuranceSubsystem::LoadoutStoresText() const
{
    auto* Simulation=Sim::GetSim();auto* Craft=Simulation?Simulation->GetPlayerShip():nullptr;
    const auto* Players=GetGameInstance()->GetSubsystem<UStarshatterPlayerSubsystem>();
    if(!Craft || !Players)return INVTEXT("MISSILE STORES — LIVE SESSION REQUIRED");
    const bool Issued=Simulation->GetMission() && !Simulation->GetMission()->IsFreeFlight();
    const FName Magazine=Issued && Craft->GetCarrier()?OrdnanceMagazine::Key(*Craft->GetCarrier()):NAME_None;
    FString Text=Issued?TEXT("BASE MAGAZINE"):TEXT("PLAYER MISSILE STORES");
    Text+=TEXT(" / ")+OwnedHullLocation::Dock(*Craft);
    for(const auto& Pair:OrdnanceMagazine::Loaded(*Craft))
    {
        const double Available=Issued?FEconomyLedger::Available(Simulation->ReadEconomy(),Magazine,Pair.Key):ShipStoreAI::Stored(Players->GetPlayerInfo(),Pair.Key,OwnedHullLocation::Dock(*Craft));
        Text+=FString::Printf(TEXT("\n%s: %d LOADED / %.0f STORED"),*Pair.Key.ToString().ToUpper(),Pair.Value,Available);
    }
    if(!LastLoadoutReason.IsEmpty())Text+=TEXT("\n")+LastLoadoutReason.ToString();
    return FText::FromString(Text);
}

bool UShipInsuranceSubsystem::RememberLoadout(const TArray<int32>& Stations, const FString& Name)
{
    auto* Players = GetGameInstance()->GetSubsystem<UStarshatterPlayerSubsystem>();
    if (!Players || !CanModifyHull(Players->GetPlayerInfo().AssignedHangarHull)) return false;
    const auto Previous = Players->GetPlayerInfo();
    auto& Pilot = Players->GetMutablePlayerInfo();
    auto* Hull = Pilot.HangarShips.FindByPredicate([&Pilot](const auto& Entry)
        { return Entry.HullId == Pilot.AssignedHangarHull; });
    if (!Hull) return false;
    Hull->LoadoutStations = Stations;
    Hull->LoadoutName = Name;
    if (!Players->SaveActivePlayerProgress())
    { Pilot = Previous; return false; }
    return true;
}

void UShipInsuranceSubsystem::Tick(float DeltaTime)
{
    ConditionElapsed+=DeltaTime;
    if (ConditionElapsed<0.5f)return;
    ConditionElapsed=0;
    auto* Players=GetGameInstance()->GetSubsystem<UStarshatterPlayerSubsystem>();
    auto* Simulation=Sim::GetSim();
    auto* Craft=Simulation?Simulation->GetPlayerShip():nullptr;
    if (!Players || !Craft || !Craft->Design() || !Simulation->HasSimulationAuthority() || Simulation->IsNetworked())return;
    const auto& View=Players->GetPlayerInfo();
    const auto* Eligible=View.HangarShips.FindByPredicate([&](const auto& Row){return FOwnedHullCondition::IsOwnedLive(Row,
        View.AssignedHangarHull,FName(UTF8_TO_TCHAR(Craft->Design()->name)),Craft->GetCargoInventoryKey(),
        Simulation->GetMission() && Simulation->GetMission()->IsFreeFlight());});
    if (!Eligible) { bDockedConditionSaved=false; LastConditionHull.Invalidate(); return; }
    auto& Pilot=Players->GetMutablePlayerInfo();
    auto* Owned=Pilot.HangarShips.FindByPredicate([&](const auto& Row){return Row.HullId==View.AssignedHangarHull;});
    const bool Docked=Craft->GetFlightPhase()<=EOPSMode::ALERT && !Craft->IsDead() && !Craft->IsDying();
    if (LastConditionHull!=Owned->HullId) {LastConditionHull=Owned->HullId;bDockedConditionSaved=false;}
    if (!Docked)bDockedConditionSaved=false;
    if (Docked && !bDockedConditionSaved)
    {
        const auto Previous=Pilot;
        FOwnedHullCondition::Capture(*Craft,*Owned);
        if (Players->SaveActivePlayerProgress())bDockedConditionSaved=true;
        else Pilot=Previous; // Retried while docked; never discard damage or charge anything.
    }
    else FOwnedHullCondition::Capture(*Craft,*Owned);
}

namespace
{
FName DockedHullStock(FName Design,bool Carrier)
{
    auto* S=Sim::GetSim();const auto* Craft=S?S->GetPlayerShip():nullptr;const auto* Base=Craft?Craft->GetCarrier():nullptr;
    if(!Base || !Craft->GetRegion() || !Craft->GetRegion()->GetSystem())return NAME_None;
    const FName System(UTF8_TO_TCHAR(Craft->GetRegion()->GetSystem()->GetName()));
    return Carrier?CarrierFighterSalesAI::Inventory(System,UTF8_TO_TCHAR(Base->GetName()),Design):FEconomyRuntime::Warehouse(System.ToString(),UTF8_TO_TCHAR(Base->GetName()),TEXT("Output"));
}
TOptional<FCarrierFighterOffer> DockedCarrierOffer(const FS_PlayerGameInfo& P,FName Design)
{
    auto* S=Sim::GetSim();const auto* Craft=S?S->GetPlayerShip():nullptr;const auto* Base=Craft?Craft->GetCarrier():nullptr;
    return Base && FStationServices::Offers(ESSWStationService::HullSales)?CarrierFighterSalesAI::Offer(P,*Base,Design):TOptional<FCarrierFighterOffer>();
}
FShipCommerceDock CommerceDock(const FS_PlayerGameInfo& P,FName Design,FGuid StockHull=FGuid())
{
    FShipCommerceDock D;auto* S=Sim::GetSim();const auto* Craft=S?S->GetPlayerShip():nullptr;const auto* Base=Craft?Craft->GetCarrier():nullptr;
    if(!S || !S->HasSimulationAuthority() || S->IsNetworked() || !Craft || Craft->IsDead() || Craft->IsDying() || Craft->GetFlightPhase()>EOPSMode::ALERT || !Base || !Craft->GetRegion() || !Craft->GetRegion()->GetSystem())return D;
    D.bAuthorized=FStationServices::Offers(ESSWStationService::HullSales) || FStationServices::Offers(ESSWStationService::Refit);
    D.Location=OwnedHullLocation::Dock(*Craft);D.System=FName(UTF8_TO_TCHAR(Craft->GetRegion()->GetSystem()->GetName()));
    const FString Identity=D.Location.StartsWith(TEXT("Carrier / "))?Base->GetHullId().ToString(EGuidFormats::Digits):D.Location;
    D.Inventory=FName(*(TEXT("Dealer/")+Identity+TEXT("/")+Design.ToString()));
    FName Source=DockedHullStock(Design,DockedCarrierOffer(P,Design).IsSet());
    if(StockHull.IsValid())if(const auto* Serial=P.Economy.SerializedHulls.Find(StockHull)){Source=Serial->Inventory;D.Inventory=Source;}
    const auto* Store=P.Economy.Inventories.Find(Source);
    D.Dealer=Store && !Store->TradingOwner.IsNone()?Store->TradingOwner:EconomyAI::NPCAccount(Source);
    return D;
}
void SyncCommerceTitle(const FS_PlayerGameInfo& P,FGuid Id,FName Owner,FName Possessor,const FString& Reason)
{
    auto* S=Sim::GetSim();const auto* Hull=P.HangarShips.FindByPredicate([Id](const auto& H){return H.HullId==Id;});if(!S || !Hull)return;
    S->HullRegistry.Register(Id,Hull->Design,Hull->Name,Owner,P.Empire+1);
    S->HullRegistry.Transfer(Id,Owner,Possessor,FDateTime::UtcNow().ToUnixTimestamp(),Reason);
}
int64 StandardRentalHullValue(const FShipDesign& Design)
{
    // A catalog quote must not construct a native Ship on every Slate attribute evaluation.
    static TMap<FName,TPair<int64,int64>> Values;
    const FName Key(*Design.ShipName);
    if(const auto* Cached=Values.Find(Key);Cached && Cached->Key==Design.BaseCost)return Cached->Value;
    FPlayerHangarShip Valued;Valued.HullValue=Design.BaseCost;FShipEquipment::InitializePurchase(Valued,Design);
    Values.Add(Key,TPair<int64,int64>(Design.BaseCost,Valued.HullValue));return Valued.HullValue;
}
}
double UShipInsuranceSubsystem::HullStock(FName Design)const
{
    const auto* P=GetGameInstance()->GetSubsystem<UStarshatterPlayerSubsystem>();if(!P)return 0;
    const auto Offer=DockedCarrierOffer(P->GetPlayerInfo(),Design);
    if(Offer.IsSet())
    {
        auto* S=Sim::GetSim();auto* Craft=S?S->GetPlayerShip():nullptr;auto* Base=Craft?Craft->GetCarrier():nullptr;
        const auto D=CommerceDock(P->GetPlayerInfo(),Design);
        return (Base && Base->GetHangar()?CarrierFighterSalesAI::Available(P->GetPlayerInfo().Economy,DockedHullStock(Design,true),*Base->GetHangar(),Offer.GetValue()):0)+ShipyardAI::Stock(P->GetPlayerInfo().Economy,D.Inventory,Design);
    }
    const auto D=CommerceDock(P->GetPlayerInfo(),Design);
    return ShipyardAI::Stock(P->GetPlayerInfo().Economy,DockedHullStock(Design,false),Design)+ShipyardAI::Stock(P->GetPlayerInfo().Economy,D.Inventory,Design);
}
bool UShipInsuranceSubsystem::PrepareHullOffers(const TArray<FName>& Designs,FText& Reason)
{
    auto* P=GetGameInstance()->GetSubsystem<UStarshatterPlayerSubsystem>();
    if(!P)return false;
    auto& Pilot=P->GetMutablePlayerInfo();const auto Before=Pilot;bool Changed=false;
    for(const FName Design:Designs)
    {
        if(HullStock(Design)<1)continue;
        const FName Inventory=DockedHullStock(Design,DockedCarrierOffer(Pilot,Design).IsSet());
        const FName Item=DataTableUtils::MakeValidName(Design.ToString().ToLower()+TEXT(" assembled hull"));
        TArray<FGuid> Preferred;
        const auto CarrierOffer=DockedCarrierOffer(Pilot,Design);
        if(CarrierOffer.IsSet())if(auto* Story=Campaign::GetCampaign())
        {
            FCampaignFleet::Prepare(*Story);
            for(const auto& Pair:Story->GetPersistentFleet().Hulls)
                if(Pair.Value.Design==Design && !Pair.Value.bDestroyed && !Pair.Value.bTransferred && CarrierOffer.GetValue().Squadrons.ContainsByPredicate([&](const auto& Squad){return Squad.Key.Equals(Pair.Value.Assignment,ESearchCase::IgnoreCase);}))Preferred.Add(Pair.Key);
        }
        Changed|=SerializedHullStockAI::Ensure(Pilot.Economy,Inventory,Design,FMath::FloorToInt(FEconomyLedger::Quantity(Pilot.Economy,Inventory,Item)),&Preferred);
        const auto Offer=DockedCarrierOffer(Pilot,Design);
        auto* S=Sim::GetSim();auto* Craft=S?S->GetPlayerShip():nullptr;auto* Base=Craft?Craft->GetCarrier():nullptr;
        if(Offer.IsSet() && Base && Base->GetHangar())Changed|=CarrierFighterSalesAI::BindHullOffers(Pilot.Economy,Inventory,*Base->GetHangar(),Offer.GetValue());
    }
    if(!Changed || P->SaveActivePlayerProgress())return true;
    Pilot=Before;Reason=INVTEXT("Could not save serialized hull offers.");return false;
}
TArray<FGuid> UShipInsuranceSubsystem::HullOffers(FName Design)const
{
    const auto* P=GetGameInstance()->GetSubsystem<UStarshatterPlayerSubsystem>();if(!P)return {};
    const auto Offer=DockedCarrierOffer(P->GetPlayerInfo(),Design);
    if(Offer.IsSet())
    {
        auto* S=Sim::GetSim();auto* Craft=S?S->GetPlayerShip():nullptr;auto* Base=Craft?Craft->GetCarrier():nullptr;
        auto Ids=Base && Base->GetHangar()?CarrierFighterSalesAI::HullOffers(P->GetPlayerInfo().Economy,DockedHullStock(Design,true),*Base->GetHangar(),Offer.GetValue()):TArray<FGuid>();
        const auto D=CommerceDock(P->GetPlayerInfo(),Design);Ids.Append(SerializedHullStockAI::Available(P->GetPlayerInfo().Economy,D.Inventory,Design,FMath::FloorToInt(ShipyardAI::Stock(P->GetPlayerInfo().Economy,D.Inventory,Design))));return Ids;
    }
    auto Ids=SerializedHullStockAI::Available(P->GetPlayerInfo().Economy,DockedHullStock(Design,false),Design,FMath::FloorToInt(HullStock(Design)));
    const auto D=CommerceDock(P->GetPlayerInfo(),Design);Ids.Append(SerializedHullStockAI::Available(P->GetPlayerInfo().Economy,D.Inventory,Design,FMath::FloorToInt(ShipyardAI::Stock(P->GetPlayerInfo().Economy,D.Inventory,Design))));return Ids;
}
bool UShipInsuranceSubsystem::CanPurchaseHull(FName Design,FText& Reason,FGuid OfferedHullId)const
{
    const auto* P=GetGameInstance()->GetSubsystem<UStarshatterPlayerSubsystem>();const auto* Hull=ShipDesignRegistry::Find(Design);
    const auto Offer=P?DockedCarrierOffer(P->GetPlayerInfo(),Design):TOptional<FCarrierFighterOffer>();
    if(!P || !Hull || (!FStationServices::Offers(ESSWStationService::Refit) && !Offer.IsSet()))
    {Reason=INVTEXT("Dock at a friendly carrier with this design in its OOB, or a shipyard with completed hulls.");return false;}
    if(OfferedHullId.IsValid() && !HullOffers(Design).Contains(OfferedHullId)){Reason=INVTEXT("This specific hull is no longer available.");return false;}
    if(!ShipyardAI::Eligible(P->GetPlayerInfo(),*Hull,Reason))return false;
    if(HullOffers(Design).IsEmpty()){Reason=INVTEXT("No unassigned squadron stock or completed hull of this design is available.");return false;}
    const int64 Price=HullPurchasePrice(Design,OfferedHullId);
    if(GetBalance()<Price){Reason=FText::Format(INVTEXT("Required: {0} CR. Balance: {1} CR."),FText::AsNumber(Price),FText::AsNumber(GetBalance()));return false;}
    Reason=FText::Format(INVTEXT("Purchase for {0} CR. Available: {1}."),FText::AsNumber(Price),FText::AsNumber(HullStock(Design)));return true;
}
bool UShipInsuranceSubsystem::PurchaseHull(FName Design,FText& Reason,FGuid OfferedHullId)
{
    if(!CanPurchaseHull(Design,Reason,OfferedHullId))return false;
    auto* P=GetGameInstance()->GetSubsystem<UStarshatterPlayerSubsystem>();const auto* Hull=ShipDesignRegistry::Find(Design);const auto Before=P->GetPlayerInfo();
    auto& Pilot=P->GetMutablePlayerInfo();auto* Craft=Sim::GetSim()->GetPlayerShip();auto* Base=Craft->GetCarrier();
    const auto Offer=DockedCarrierOffer(Pilot,Design);
    if(OfferedHullId.IsValid() && Pilot.Economy.UsedHullStock.Contains(OfferedHullId))
    {
        const auto Dock=CommerceDock(Pilot,Design,OfferedHullId);auto Priced=*Hull;Priced.BaseCost=HullPurchasePrice(Design,OfferedHullId);
        if(!ShipyardAI::Purchase(Pilot,Priced,Dock.Inventory,Dock.Location,FGuid::NewGuid(),Reason,OfferedHullId))return false;
        if(!P->SaveActivePlayerProgress()){Pilot=Before;return false;}SyncCommerceTitle(Pilot,OfferedHullId,FEconomyLedger::Pilot(),FEconomyLedger::Pilot(),TEXT("Used hull purchase"));return true;
    }
    if(Offer.IsSet())
    {
        const FGuid Receipt=FGuid::NewGuid();
        const bool Bought=Base->GetHangar() && CarrierFighterSalesAI::Purchase(Pilot,*Hull,*Base->GetHangar(),Offer.GetValue(),DockedHullStock(Design,true),OwnedHullLocation::Dock(*Craft),Receipt,[P](){return P->SaveActivePlayerProgress();},Reason,OfferedHullId);
        if(Bought)SyncCommerceTitle(Pilot,SerializedHullStockAI::Purchased(Pilot.Economy,Receipt),FEconomyLedger::Pilot(),FEconomyLedger::Pilot(),TEXT("Carrier hull purchase"));return Bought;
    }
    if(!ShipyardAI::Purchase(Pilot,*Hull,DockedHullStock(Design,false),OwnedHullLocation::Dock(*Craft),FGuid::NewGuid(),Reason,OfferedHullId))return false;
    if(!P->SaveActivePlayerProgress()){Pilot=Before;Reason=INVTEXT("Could not save the purchase. Hull stock and credits restored.");return false;}return true;
}
bool UShipInsuranceSubsystem::PrepareCarrierFighterSales(FText& Why)
{
    if(!FStationServices::Offers(ESSWStationService::HullSales))return false;
    auto* P=GetGameInstance()->GetSubsystem<UStarshatterPlayerSubsystem>();auto* S=Sim::GetSim();auto* Craft=S?S->GetPlayerShip():nullptr;auto* Base=Craft?Craft->GetCarrier():nullptr;
    if(!P || !Base || !Craft->GetRegion() || !Craft->GetRegion()->GetSystem())return false;
    return CarrierFighterSalesAI::Prepare(P->GetMutablePlayerInfo(),*Base,FName(UTF8_TO_TCHAR(Craft->GetRegion()->GetSystem()->GetName())),[P](){return P->SaveActivePlayerProgress();},Why);
}
int64 UShipInsuranceSubsystem::HullPurchasePrice(FName Design,FGuid HullId) const
{
    const auto* Hull=ShipDesignRegistry::Find(Design);const auto* P=GetGameInstance()->GetSubsystem<UStarshatterPlayerSubsystem>();
    if(P)
    {
        if(const auto* Used=P->GetPlayerInfo().Economy.UsedHullStock.Find(HullId))
        {FShipCommerceRules Rules;Rules.BuybackPercent=100;return FMath::Max(1LL,ShipCommerceAI::SalePrice(*Used,Rules));}
        const auto Offer=DockedCarrierOffer(P->GetPlayerInfo(),Design);if(Offer.IsSet())return Offer.GetValue().Price;
    }
    return Hull?Hull->BaseCost:0;
}

bool UShipInsuranceSubsystem::CanSellHull(FGuid Id,FText& Why)const
{
    const auto* P=GetGameInstance()->GetSubsystem<UStarshatterPlayerSubsystem>();const auto* H=FindShip(Id);FShipCommerceRules Rules;FString Error;
    if(!P || !H || !FShipCommerceRules::Load(Rules,Error)){Why=FText::FromString(Error);return false;}
    return ShipCommerceAI::CanSell(P->GetPlayerInfo(),Id,CommerceDock(P->GetPlayerInfo(),H->Design),Rules,Why);
}
bool UShipInsuranceSubsystem::SellHull(FGuid Id,FText& Why)
{
    if(!CanSellHull(Id,Why))return false;
    auto* P=GetGameInstance()->GetSubsystem<UStarshatterPlayerSubsystem>();const auto* H=FindShip(Id);FShipCommerceRules Rules;FString Error;if(!FShipCommerceRules::Load(Rules,Error))return false;
    const auto D=CommerceDock(P->GetPlayerInfo(),H->Design);
    if(!ShipCommerceAI::Sell(P->GetMutablePlayerInfo(),Id,D,Rules,FGuid::NewGuid(),FDateTime::UtcNow().ToUnixTimestamp(),[P](){return P->SaveActivePlayerProgress();},Why))return false;
    SyncCommerceTitle(P->GetPlayerInfo(),Id,D.Dealer,D.Dealer,TEXT("Owned hull sale"));return true;
}
bool UShipInsuranceSubsystem::CanReturnRental(FGuid Id,FText& Why)const
{
    const auto* P=GetGameInstance()->GetSubsystem<UStarshatterPlayerSubsystem>();const auto* H=FindShip(Id);
    return P && H && ShipCommerceAI::CanReturn(P->GetPlayerInfo(),Id,CommerceDock(P->GetPlayerInfo(),H->Design),Why);
}
bool UShipInsuranceSubsystem::ReturnRental(FGuid Id,FText& Why)
{
    if(!CanReturnRental(Id,Why))return false;
    auto* P=GetGameInstance()->GetSubsystem<UStarshatterPlayerSubsystem>();const auto D=CommerceDock(P->GetPlayerInfo(),FindShip(Id)->Design);
    const auto* C=P->GetPlayerInfo().ShipRentals.FindByPredicate([Id](const auto& R){return R.HullId==Id && R.State==EShipRentalState::Active;});if(!C)return false;const FName Owner=C->Lessor;
    if(!ShipCommerceAI::Return(P->GetMutablePlayerInfo(),Id,D,FGuid::NewGuid(),FDateTime::UtcNow().ToUnixTimestamp(),[P](){return P->SaveActivePlayerProgress();},Why))return false;
    SyncCommerceTitle(P->GetPlayerInfo(),Id,Owner,Owner,TEXT("Rental return"));return true;
}
bool UShipInsuranceSubsystem::CanRentHull(FName Design,FGuid Id,int32 Days,FText& Why)const
{
    const auto* P=GetGameInstance()->GetSubsystem<UStarshatterPlayerSubsystem>();const auto* Hull=ShipDesignRegistry::Find(Design);FShipCommerceRules Rules;FString Error;
    if(!P || !Hull || !FShipCommerceRules::Load(Rules,Error)){Why=FText::FromString(Error);return false;}
    const auto& Pilot=P->GetPlayerInfo();const auto Dock=CommerceDock(Pilot,Design,Id);
    if(!Dock.bAuthorized || !HullOffers(Design).Contains(Id) || !ShipyardAI::Eligible(Pilot,*Hull,Why))return false;
    if(Pilot.ShipRentals.ContainsByPredicate([](const auto& C){return C.UnpaidLiability>0;})){Why=INVTEXT("Repay outstanding rental liability first.");return false;}
    FPlayerHangarShip Valued;Valued.HullValue=StandardRentalHullValue(*Hull);
    if(const auto* Used=Pilot.Economy.UsedHullStock.Find(Id))Valued=*Used;Valued.bCashPurchased=false;Valued.InstalledComponentValue=0;
    const auto Insurance=Quote(Valued,EShipInsuranceTier::Basic);const int64 Fee=ShipCommerceAI::RentalFee(Hull->BaseCost,Days,Rules);
    Why=FText::Format(INVTEXT("{0} DAY RENTAL: {1} CR + {2} CR mandatory Basic hull insurance = {3} CR. Return here by the due date. Hull-loss liability up to {4} CR; no component-loss charge."),FText::AsNumber(Days),FText::AsNumber(Fee),FText::AsNumber(Insurance.Premium),FText::AsNumber(Fee+Insurance.Premium),FText::AsNumber(FMath::Max(0LL,Valued.HullValue-Insurance.Payout)));
    if(Fee<=0 || GetBalance()<Fee+Insurance.Premium){Why=FText::Format(INVTEXT("Insufficient credits. {0}"),Why);return false;}return true;
}
bool UShipInsuranceSubsystem::RentHull(FName Design,FGuid Id,int32 Days,FText& Why)
{
    if(!CanRentHull(Design,Id,Days,Why))return false;
    auto* P=GetGameInstance()->GetSubsystem<UStarshatterPlayerSubsystem>();auto& Pilot=P->GetMutablePlayerInfo();FScopedSimDataChangeBatch Batch(Pilot.Economy);const auto Before=Pilot;const auto* Hull=ShipDesignRegistry::Find(Design);
    const auto D=CommerceDock(Pilot,Design,Id);FShipCommerceRules Rules;FString Error;if(!FShipCommerceRules::Load(Rules,Error))return false;
    const auto Offer=DockedCarrierOffer(Pilot,Design);const int64 Now=FDateTime::UtcNow().ToUnixTimestamp();
    if(Offer.IsSet() && !Pilot.Economy.UsedHullStock.Contains(Id))
    {
        auto* Base=Sim::GetSim()->GetPlayerShip()->GetCarrier();
        if(!Base->GetHangar() || !CarrierFighterSalesAI::Rent(Pilot,*Hull,*Base->GetHangar(),Offer.GetValue(),D,Rules,Id,Days,Now,[P](){return P->SaveActivePlayerProgress();},Why))return false;
    }
    else
    {
        if(!ShipCommerceAI::Rent(Pilot,*Hull,Id,D,Rules,Days,Now,FGuid::NewGuid(),Why))return false;
        if(!P->SaveActivePlayerProgress()){Pilot=Before;Why=INVTEXT("Could not save rental; stock and credits restored.");return false;}
    }
    SyncCommerceTitle(Pilot,Id,D.Dealer,FEconomyLedger::Pilot(),TEXT("Rental custody - NPC title retained"));return true;
}
bool UShipInsuranceSubsystem::PayRentalDebt(FGuid Rental,FText& Why)
{
    auto* P=GetGameInstance()->GetSubsystem<UStarshatterPlayerSubsystem>();
    return P && CanInsure() && ShipCommerceAI::PayRentalDebt(P->GetMutablePlayerInfo(),Rental,FGuid::NewGuid(),[P](){return P->SaveActivePlayerProgress();},Why);
}

bool UShipInsuranceSubsystem::RequestOwnedAction(const FOwnedActionSelection& Selection,TFunction<void(bool,const FText&,FGuid)>&& Completed,FText& Why)
{
    check(IsInGameThread());auto W=Selection;auto* P=GetGameInstance()->GetSubsystem<UStarshatterPlayerSubsystem>();auto* S=Sim::GetSim();auto* Craft=S?S->GetPlayerShip():nullptr;
    auto* Domain=GetGameInstance()->GetSubsystem<UGalaxyTrafficSubsystem>();
    if(!P || !P->HasLoaded() || !S || !S->HasSimulationAuthority() || S->IsNetworked() || !Craft || !Craft->Design() || !Domain)
    {Why=INVTEXT("The authoritative pilot service is unavailable.");return false;}
    if(W.Action==EOwnedCommand::Repair || W.Action==EOwnedCommand::Loadout || W.Action==EOwnedCommand::TransferDrone)W.Hull=P->GetPlayerInfo().AssignedHangarHull;
    const bool Loss=W.Action==EOwnedCommand::Loss;
    if(Loss && bLossPending){Why=INVTEXT("Insurance settlement is processing.");return false;}
    if(!Loss && (!FStationServices::IsDocked() || Craft->IsDead() || Craft->IsDying())){Why=INVTEXT("Dock with a live craft to use this service.");return false;}
    P->GetEconomy();FOwnedHullCondition::CaptureActive(P->GetMutablePlayerInfo());
    const auto& Pilot=P->GetPlayerInfo();const auto* Owned=Pilot.HangarShips.FindByPredicate([&](const auto& H){return H.HullId==W.Hull;});
    FNativeOwnedCommand C;C.Action=W.Action;C.Hull=W.Hull;C.Receipt=FGuid::NewGuid();C.NowUtc=FDateTime::UtcNow().ToUnixTimestamp();C.Days=W.Days;C.Quantity=W.Quantity;C.Slot=W.Slot;C.Grade=W.Grade;C.Tier=W.Tier;C.Stations=W.Stations;C.Loadout=W.Loadout;
    C.Dock.Location=OwnedHullLocation::Dock(*Craft);C.Hold=Craft->GetCargoInventoryKey();
    TOptional<FPlayerHangarShip> OriginalHull,OriginalStock;TOptional<FSerializedHullStock> OriginalSerial;
    if(Owned)OriginalHull=*Owned;if(const auto* Stock=Pilot.Economy.UsedHullStock.Find(W.Hull))OriginalStock=*Stock;
    if(const auto* Serial=Pilot.Economy.SerializedHulls.Find(W.Hull))OriginalSerial=*Serial;
    FString Error;int32 ReserveSquadron=INDEX_NONE,ReserveSlot=INDEX_NONE;TOptional<ComponentDesign> Calibration;
    FPlayerHangarShip BeforeLive;FOwnedHullCondition::Capture(*Craft,BeforeLive);const auto BeforeStations=CurrentStations(*Craft);
    if(W.Action==EOwnedCommand::Purchase || W.Action==EOwnedCommand::Rent)
    {
        if(!W.Hull.IsValid() || !(W.Action==EOwnedCommand::Rent?CanRentHull(W.Design,W.Hull,W.Days,Why):CanPurchaseHull(W.Design,Why,W.Hull)))return false;
        const auto* Def=ShipDesignRegistry::Find(W.Design);if(!Def || !FShipCommerceRules::Load(C.Rules,Error))return false;
        C.Dock=CommerceDock(Pilot,W.Design,W.Hull);C.BaseCost=Def->BaseCost;
        if(OriginalStock.IsSet())C.PreparedHull=OriginalStock.GetValue();
        else
        {
            C.PreparedHull.HullId=W.Hull;C.PreparedHull.Design=W.Design;C.PreparedHull.Name=Def->DisplayName.IsEmpty()?Def->ShipName:Def->DisplayName;
            C.PreparedHull.HullValue=W.Action==EOwnedCommand::Purchase?HullPurchasePrice(W.Design,W.Hull):Def->BaseCost;
            if(!Def->Loadout.IsEmpty()){C.PreparedHull.LoadoutName=Def->Loadout[0].Name;C.PreparedHull.LoadoutStations=Def->Loadout[0].Stations;}
            FShipEquipment::InitializePurchase(C.PreparedHull,*Def);
        }
        C.Cost=HullPurchasePrice(W.Design,W.Hull);
        if(W.Action==EOwnedCommand::Rent)
        {auto Valued=C.PreparedHull;Valued.bCashPurchased=false;Valued.InstalledComponentValue=0;C.Cost=ShipCommerceAI::RentalFee(Def->BaseCost,W.Days,C.Rules)+Quote(Valued,EShipInsuranceTier::Basic).Premium;}
        if(!OriginalStock.IsSet() && DockedCarrierOffer(Pilot,W.Design).IsSet())
        {
            auto* H=Craft->GetCarrier()?Craft->GetCarrier()->GetHangar():nullptr;if(!H || !OriginalSerial.IsSet())return false;
            for(int32 I=0;I<H->NumSquadrons();++I)if(OriginalSerial->Squadron.Equals(UTF8_TO_TCHAR(H->SquadronName(I).data()),ESearchCase::IgnoreCase))ReserveSquadron=I;
            ReserveSlot=OriginalSerial->Slot;if(ReserveSquadron==INDEX_NONE || ReserveSlot==INDEX_NONE)return false;
        }
    }
    else if(W.Action==EOwnedCommand::Sell || W.Action==EOwnedCommand::Return)
    {
        if(!Owned || !(W.Action==EOwnedCommand::Sell?CanSellHull(W.Hull,Why):CanReturnRental(W.Hull,Why)) || !FShipCommerceRules::Load(C.Rules,Error))return false;
        C.Dock=CommerceDock(Pilot,Owned->Design);C.PreparedHull=*Owned;FShipEquipment::SeparateValue(C.PreparedHull);
        C.Cost=ShipCommerceAI::SalePrice(C.PreparedHull,C.Rules);for(const auto& Pair:ShipDesignRegistry::GetAll())C.Categories.Add(Pair.Key,Pair.Value.Category);
    }
    else if(W.Action==EOwnedCommand::Policy)
    {
        if(!Owned || !CanModifyHull(W.Hull) || Owned->bAssigned || Owned->bCashPurchased || Owned->bLost || Owned->bSold || Owned->RentalId.IsValid() || W.Tier<EShipInsuranceTier::Basic || W.Tier>EShipInsuranceTier::Platinum || EffectiveTier(*Owned,C.NowUtc)!=EShipInsuranceTier::Uninsured)
        {Why=INVTEXT("This hull cannot buy that insurance policy.");return false;}
        C.Cost=Quote(*Owned,W.Tier).Premium;if(Quote(*Owned,W.Tier).Payout<=0 || GetBalance()<C.Cost)return false;
    }
    else if(W.Action==EOwnedCommand::Debt)
    {if(!CanInsure() || !Pilot.ShipRentals.ContainsByPredicate([&](const auto& R){return R.Id==W.Hull && R.UnpaidLiability>0;}))return false;}
    else if(W.Action==EOwnedCommand::TransferDrone)
    {auto* Store=GetGameInstance()->GetSubsystem<UShipStoreSubsystem>();if(!Store || !Store->CanTransferMiningDrone(W.Quantity,Why))return false;}
    else if(W.Action==EOwnedCommand::Fit)
    {
        if(!Owned || !ValidateFitting(W.Hull,W.Slot,W.Grade,Why) || !CanRefitHull(W.Hull,&Why))return false;
        const auto& Fitted=Owned->Components[W.Slot];Calibration=FShipEquipment::GradeDesign(Fitted.SystemDesign,Fitted.Component,W.Grade);
        const auto* Catalog=GetGameInstance()->GetSubsystem<UStarshatterSystemDesignSubsystem>();const auto Variant=Catalog?Catalog->FindComponentGrade(Fitted.SystemDesign,Fitted.Component,W.Grade):TOptional<FComponentDesign>();
        if(!Calibration.IsSet() || !Variant || Fitted.Level==W.Grade)return false;C.Cost=Variant->BaseCost;C.Quantity=Variant->Spares;
    }
    else if(W.Action==EOwnedCommand::Loadout)
    {
        if(!Owned || !CanModifyHull(W.Hull) || W.Hull!=Pilot.AssignedHangarHull || !OrdnanceMagazine::Required(*Craft,W.Stations,C.Wanted,Why))return false;
        if(S->GetMission() && !S->GetMission()->IsFreeFlight())
        {if(!Craft->GetCarrier())return false;C.Magazine=OrdnanceMagazine::Ensure(P->GetEconomy(),*Craft->GetCarrier(),Why);if(C.Magazine.IsNone())return false;}
        C.Loaded=OrdnanceMagazine::Loaded(*Craft);
    }
    else if(W.Action==EOwnedCommand::Repair)
    {C.Cost=FStationServices::FullRepairCost();if(C.Cost<=0 || GetBalance()<C.Cost || !FStationServices::Offers(ESSWStationService::Repair))return false;}
    else if(Loss)
    {
        if(!Owned || Owned->bLost || Pilot.AssignedHangarHull!=W.Hull || (!Craft->IsDead() && !Craft->IsDying()))return false;
        const bool Free=S->GetMission() && S->GetMission()->IsFreeFlight();
        if(!Owned->bAssigned && !FOwnedHullCondition::IsOwnedLive(*Owned,W.Hull,FName(UTF8_TO_TCHAR(Craft->Design()->name)),Craft->GetCargoInventoryKey(),Free))return false;
        C.bValidatedDeath=true;
    }
    else return false;
    const FGuid Physical=Craft->GetHullId();const FGuid Base=Craft->GetCarrier()?Craft->GetCarrier()->GetHullId():FGuid();const FString Dock=OwnedHullLocation::Dock(*Craft);
    const TWeakObjectPtr<UShipInsuranceSubsystem> Weak(this);const uint64 Epoch=S->GetSimulationEpoch();
    const auto CurrentCraft=[Weak,Physical,Epoch]()->Ship*
    {auto* Live=Sim::GetSim();auto* V=Live?Live->GetPlayerShip():nullptr;return Weak.IsValid() && Live && Live->GetSimulationEpoch()==Epoch && V && V->GetHullId()==Physical?V:nullptr;};
    const bool RuntimeChange=W.Action==EOwnedCommand::Fit || W.Action==EOwnedCommand::Loadout || W.Action==EOwnedCommand::Repair;
    const auto Validate=[Weak,W,C,OriginalHull,OriginalStock,OriginalSerial,CurrentCraft,Base,Dock,BeforeLive,RuntimeChange,Loss](FText& Reason)
    {
        auto* Service=Weak.Get();auto* Players=Service?Service->GetGameInstance()->GetSubsystem<UStarshatterPlayerSubsystem>():nullptr;if(!Players || !Players->HasLoaded())return false;
        const auto& Live=Players->GetPlayerInfo();const auto* H=Service->FindShip(W.Hull);
        if(OriginalHull.IsSet()!=bool(H) || (H && !FPlayerHangarShip::StaticStruct()->CompareScriptStruct(&OriginalHull.GetValue(),H,0))){Reason=INVTEXT("The selected hull changed. Review the updated details.");return false;}
        if(Loss)return C.bValidatedDeath && H && !H->bLost && Live.AssignedHangarHull==W.Hull;
        auto* V=CurrentCraft();if(!V || !FStationServices::IsDocked() || !V->GetCarrier() || V->GetCarrier()->GetHullId()!=Base || OwnedHullLocation::Dock(*V)!=Dock)return false;
        if(RuntimeChange){FPlayerHangarShip Health;FOwnedHullCondition::Capture(*V,Health);if(!FPlayerHangarShip::StaticStruct()->CompareScriptStruct(&BeforeLive,&Health,0)){Reason=INVTEXT("The live fitting or condition changed.");return false;}}
        if(W.Action==EOwnedCommand::Purchase || W.Action==EOwnedCommand::Rent)
        {
            const auto* Stock=Live.Economy.UsedHullStock.Find(W.Hull);const auto* Serial=Live.Economy.SerializedHulls.Find(W.Hull);
            if(OriginalStock.IsSet()!=bool(Stock) || (Stock && !FPlayerHangarShip::StaticStruct()->CompareScriptStruct(&OriginalStock.GetValue(),Stock,0)) || OriginalSerial.IsSet()!=bool(Serial) || (Serial && !FSerializedHullStock::StaticStruct()->CompareScriptStruct(&OriginalSerial.GetValue(),Serial,0)))return false;
            if(W.Action==EOwnedCommand::Purchase)return Service->HullPurchasePrice(W.Design,W.Hull)==C.Cost && Service->CanPurchaseHull(W.Design,Reason,W.Hull);
            return Service->CanRentHull(W.Design,W.Hull,W.Days,Reason);
        }
        if(W.Action==EOwnedCommand::Sell)return Service->CanSellHull(W.Hull,Reason);
        if(W.Action==EOwnedCommand::Return)return Service->CanReturnRental(W.Hull,Reason);
        if(W.Action==EOwnedCommand::Policy)return H && Service->CanModifyHull(W.Hull) && EffectiveTier(*H,FDateTime::UtcNow().ToUnixTimestamp())==EShipInsuranceTier::Uninsured && Quote(*H,W.Tier).Premium==C.Cost && Service->GetBalance()>=C.Cost;
        if(W.Action==EOwnedCommand::Debt)return Service->CanInsure();
        if(W.Action==EOwnedCommand::TransferDrone){const auto* Store=Service->GetGameInstance()->GetSubsystem<UShipStoreSubsystem>();return Store && Store->CanTransferMiningDrone(W.Quantity,Reason);}
        if(W.Action==EOwnedCommand::Fit)return Service->CanRefitHull(W.Hull,&Reason) && Service->ValidateFitting(W.Hull,W.Slot,W.Grade,Reason);
        if(W.Action==EOwnedCommand::Loadout){TMap<FName,int32> Loaded=OrdnanceMagazine::Loaded(*V),Wanted;return Loaded.OrderIndependentCompareEqual(C.Loaded) && OrdnanceMagazine::Required(*V,W.Stations,Wanted,Reason) && Wanted.OrderIndependentCompareEqual(C.Wanted);}
        return W.Action==EOwnedCommand::Repair && FStationServices::FullRepairCost()==C.Cost && C.Cost>0 && Service->GetBalance()>=C.Cost;
    };
    if(Loss)bLossPending=true;
    const bool Queued=Domain->QueueOwnedCommand(MoveTemp(C),Loss?INVTEXT("INSURANCE LOSS SETTLEMENT"):INVTEXT("HANGAR / EQUIPMENT TRANSACTION"),TFunction<bool(FText&)>(Validate),
        [Weak,W,CurrentCraft,BeforeLive,BeforeStations,Calibration,ReserveSquadron,ReserveSlot,RuntimeChange,Loss]()
        {
            auto* Service=Weak.Get();auto* Players=Service?Service->GetGameInstance()->GetSubsystem<UStarshatterPlayerSubsystem>():nullptr;if(!Players)return false;
            auto* V=CurrentCraft();auto* BaseShip=V?V->GetCarrier():nullptr;auto* Bay=BaseShip?BaseShip->GetHangar():nullptr;
            bool Transferred=false;TOptional<SimComponent> OldPart;SimComponent* Part=nullptr;
            if(ReserveSquadron!=INDEX_NONE){if(!Bay || !Bay->TransferStoredReserve(ReserveSquadron,ReserveSlot))return false;Transferred=true;}
            bool Applied=true;auto& Info=Players->GetMutablePlayerInfo();auto* H=Info.HangarShips.FindByPredicate([&](const auto& Row){return Row.HullId==W.Hull;});
            if(W.Action==EOwnedCommand::Fit)
            {
                if(!V || !H || !H->Components.IsValidIndex(W.Slot) || !Calibration.IsSet())Applied=false;
                else
                {
                    const auto Incoming=H->Components[W.Slot];auto& Systems=V->GetSystems();
                    if(Incoming.SystemIndex<0 || Incoming.SystemIndex>=Systems.size() || !Systems[Incoming.SystemIndex] || Incoming.ComponentIndex<0 || Incoming.ComponentIndex>=Systems[Incoming.SystemIndex]->GetComponents().size())Applied=false;
                    else
                    {
                        Part=Systems[Incoming.SystemIndex]->GetComponents()[Incoming.ComponentIndex];if(!Part)Applied=false;
                        else{OldPart=*Part;Part->InstallDesign(Calibration.GetValue());FOwnedHullCondition::ApplyComponent(*Part,Incoming);Systems[Incoming.SystemIndex]->CalcStatus();FOwnedHullCondition::Capture(*V,*H);}
                    }
                }
            }
            else if(W.Action==EOwnedCommand::Loadout)
            {
                Applied=V && H && V->ApplyDockedLoadout(W.Stations);
                if(Applied){auto Health=BeforeLive;for(ListIter<WeaponGroup> Groups=V->GetWeapons();++Groups;)for(ListIter<Weapon> Weapons=Groups->GetWeapons();++Weapons;)if(!Weapons->IsPrimary()){Health.SavedWeaponAmmo.Remove(Weapons->GetIndex());Health.SavedWeaponTypes.Remove(Weapons->GetIndex());}FOwnedHullCondition::Apply(*V,Health);FOwnedHullCondition::CaptureAmmo(*V,*H);}
            }
            else if(W.Action==EOwnedCommand::Repair){Applied=V && V->RepairAtStation();if(Applied && H)FOwnedHullCondition::Capture(*V,*H);}
            if(V)V->ApplyCargoInventoryToManifest();
            if(Applied && Players->SaveActivePlayerProgress())return true;
            if(Transferred && Bay)Bay->TransferStoredReserve(ReserveSquadron,ReserveSlot,true);
            if(RuntimeChange && V){if(Part && OldPart.IsSet())*Part=OldPart.GetValue();if(W.Action==EOwnedCommand::Loadout)V->ApplyDockedLoadout(BeforeStations);FOwnedHullCondition::Apply(*V,BeforeLive);}
            return false;
        },[Weak,W,CurrentCraft,Done=MoveTemp(Completed),Loss](bool Saved,const FText& Reason,FGuid Object)
        {
            auto* Service=Weak.Get();if(Service && Loss)Service->bLossPending=false;if(Service && W.Action==EOwnedCommand::Loadout)Service->LastLoadoutReason=Reason;
            if(auto* V=CurrentCraft())V->ApplyCargoInventoryToManifest();
            if(Service && Saved)
            {
                const auto* Players=Service->GetGameInstance()->GetSubsystem<UStarshatterPlayerSubsystem>();
                if(Players)
                {
                    const auto& Info=Players->GetPlayerInfo();FName Owner=FEconomyLedger::Pilot(),Custody=Owner;
                    if(W.Action==EOwnedCommand::Sell || W.Action==EOwnedCommand::Return || W.Action==EOwnedCommand::Rent)
                    {for(const auto& Pair:Info.Economy.SerializedHulls)if(Pair.Key==W.Hull){const auto* Store=Info.Economy.Inventories.Find(Pair.Value.Inventory);if(Store)Owner=Store->TradingOwner.IsNone()?EconomyAI::NPCAccount(Pair.Value.Inventory):Store->TradingOwner;break;}if(W.Action!=EOwnedCommand::Rent)Custody=Owner;}
                    if(W.Action==EOwnedCommand::Purchase || W.Action==EOwnedCommand::Rent || W.Action==EOwnedCommand::Sell || W.Action==EOwnedCommand::Return)SyncCommerceTitle(Info,W.Hull,Owner,Custody,TEXT("Saved hull commerce"));
                }
                if(auto* Story=Campaign::GetCampaign())Story->GetPersistentFleet().bDirty=true;
            }
            if(Done)Done(Saved,Reason,Object);
        },Why);
    if(!Queued && Loss)bLossPending=false;return Queued;
}
void UShipInsuranceSubsystem::FlushLossBeforeTransition()
{
    auto* P=GetGameInstance()->GetSubsystem<UStarshatterPlayerSubsystem>();auto* V=Sim::GetSim()?Sim::GetSim()->GetPlayerShip():nullptr;
    if(P && V && (V->IsDead() || V->IsDying()) && P->GetPlayerInfo().AssignedHangarHull.IsValid() && !bLossPending)
    {FOwnedActionSelection W;W.Action=EOwnedCommand::Loss;W.Hull=P->GetPlayerInfo().AssignedHangarHull;FText Why;RequestOwnedAction(W,{},Why);}
    if(auto* Domain=GetGameInstance()->GetSubsystem<UGalaxyTrafficSubsystem>())Domain->FlushCriticalOwned();
}
