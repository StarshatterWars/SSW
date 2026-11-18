#include "ShipInsuranceSubsystem.h"
#include "TransactionAI.h"
#include "OwnedHullCondition.h"
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
        { FText Ignored; SettleLoss(Current->HullId,Ignored); }
        return;
    }
    // Mission-assigned craft never inherit an owned hull solely because their designs match.
    if (!FreeFlight && Current && !Current->bAssigned) Current=nullptr;
    if (FreeFlight)
        if (auto* Selected=Pilot.HangarShips.FindByPredicate([&](const auto& Row)
            {return Row.HullId==Pilot.SelectedHangarHull && !Row.bAssigned && !Row.bLost && Row.Design==Design;}))
        { Current=Selected; Pilot.AssignedHangarHull=Selected->HullId; }
    if (!Current || Current->Design != Design || Current->bLost)
    {
        FPlayerHangarShip NewShip;
        NewShip.HullId = FGuid::NewGuid();
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
            Craft->ApplyDockedLoadout(Current->LoadoutStations);
        bRestoreLoadout = false;
        if (int* Loadout = Craft->GetLoadout())
        {
            Current->LoadoutStations.Reset();
            for (int32 Index = 0; Index < Craft->Design()->hardpoints.size(); ++Index)
                Current->LoadoutStations.Add(Loadout[Index]);
        }
    }
    const auto* SystemCatalog = GetGameInstance()->GetSubsystem<UStarshatterSystemDesignSubsystem>();
    const auto OldComponents = Current->Components;
    Current->Components.Reset();
    Current->InstalledComponentValue = 0;
    auto& Systems = Craft->GetSystems();
    for (int32 Index = 0; Index < Systems.size(); ++Index)
    {
        auto* System = Systems[Index];
        if (!System) continue;
        auto& Parts = System->GetComponents();
        for (int32 PartIndex = 0; PartIndex < Parts.size(); ++PartIndex)
        {
            auto* Part = Parts[PartIndex];
            if (!Part) continue;
            FHangarComponent Slot;
            Slot.SystemIndex = Index;
            Slot.ComponentIndex = PartIndex;
            Slot.SystemName = UTF8_TO_TCHAR(System->GetName());
            Slot.Component = FName(UTF8_TO_TCHAR(Part->Name()));
            if (const auto* Old = OldComponents.FindByPredicate([&](const auto& Item)
                { return Item.SystemIndex == Index && Item.ComponentIndex == PartIndex; }))
            {
                Slot = *Old;
                if (SystemCatalog && Slot.Level != 3)
                    if (const auto Variant = SystemCatalog->FindComponentGrade(Slot.SystemDesign, Slot.Component, Slot.Level))
                    {
                        ComponentDesign Native;
                        Native.name = TCHAR_TO_UTF8(*Variant->Name);
                        Native.abrv = TCHAR_TO_UTF8(*Variant->Abbrev);
                        Native.base_cost = Variant->BaseCost;
                        Native.performance_multiplier = Variant->PerformanceMultiplier;
                        Native.power_demand_multiplier = Variant->PowerDemandMultiplier;
                        Native.repair_time = Variant->RepairTime;
                        Native.replace_time = Variant->ReplaceTime;
                        Native.spares = Variant->Spares;
                        Native.affects = Variant->Affects;
                        Part->ApplyGradeDesign(Native);
                    }
            }
            else if (SystemCatalog)
            {
                int32 Matches = 0;
                for (const auto& Pair : SystemCatalog->GetDesignsByName())
                    if (Pair.Value.Components.ContainsByPredicate([&](const auto& Item)
                        { return FName(*Item.Name) == Slot.Component; }))
                    {
                        ++Matches;
                        Slot.SystemDesign = Pair.Key;
                        if (Pair.Key == FName(*Slot.SystemName)) { Matches = 1; break; }
                    }
                if (Matches != 1) Slot.SystemDesign = NAME_None;
            }
            if (!Slot.ItemId.IsValid()) Slot.ItemId = FGuid::NewGuid();
            Current->Components.Add(Slot);
            const auto Variant = SystemCatalog ? SystemCatalog->FindComponentGrade(Slot.SystemDesign, Slot.Component, Slot.Level)
                : TOptional<FComponentDesign>();
            Current->InstalledComponentValue += Variant ? Variant->BaseCost : Part->Cost();
        }
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
    Current->Name = UTF8_TO_TCHAR(Craft->GetObserverName());
    if (Craft->GetRegion()) Current->Location = UTF8_TO_TCHAR(Craft->GetRegion()->GetName());
    Players->MarkDirty();
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
    Entry.HullId = FGuid::NewGuid();
    Entry.Design = Design;
    Entry.Name = Name;
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
    return Id;
}

bool UShipInsuranceSubsystem::PurchasePolicy(FGuid Hull, EShipInsuranceTier Tier, FText& Result)
{
    auto* Players = GetGameInstance()->GetSubsystem<UStarshatterPlayerSubsystem>();
    if (!Players || !CanModifyHull(Hull))
    { Result = INVTEXT("Dock to purchase insurance."); return false; }
    auto& Pilot = Players->GetMutablePlayerInfo();
    auto* Entry = Pilot.HangarShips.FindByPredicate([Hull](const auto& Item) { return Item.HullId == Hull; });
    if (!Entry || Entry->bLost || Entry->bAssigned || Entry->bCashPurchased ||
        Tier < EShipInsuranceTier::Basic || Tier > EShipInsuranceTier::Platinum)
    { Result = INVTEXT("This hull cannot purchase that policy."); return false; }
    const int64 Now = FDateTime::UtcNow().ToUnixTimestamp();
    if (EffectiveTier(*Entry, Now) != EShipInsuranceTier::Uninsured)
    { Result = INVTEXT("An active policy already covers this hull."); return false; }
    const auto Offer = Quote(*Entry, Tier);
    if (Offer.Payout <= 0 || GetBalance() < Offer.Premium)
    { Result = INVTEXT("Insufficient credits for this premium."); return false; }
    const auto Previous = Pilot;
    if (!FEconomyLedger::Pay(Players->GetEconomy(), FEconomyLedger::Pilot(), TEXT("Insurance fund"), Offer.Premium, FGuid::NewGuid(), TEXT("Insurance premium"))) return false;
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
    Entry->bLost = true;
    Entry->Insurance.bSettled = true;
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
    if (!Players || !CanModifyHull(Hull) || !Entry || Entry->bLost)
    { Result = INVTEXT("Dock to select an available hull."); return false; }
    const auto* Design = ShipDesignRegistry::Find(Entry->Design);
    const auto* PilotCharacter = PlayerCharacter::GetCurrentPlayer();
    if (!Design || !PilotCharacter || PilotCharacter->GetRank() < PlayerCharacter::CommandRankRequired(Design->ShipType))
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
    if (!Craft || !Entry || !Catalog || !CanModifyHull(Hull) || !Entry->Components.IsValidIndex(Slot))
    { Result = INVTEXT("Dock with this hull to fit a component."); return false; }
    const auto& Fitted = Entry->Components[Slot];
    const auto Variant = Catalog->FindComponentGrade(Fitted.SystemDesign, Fitted.Component, Level);
    auto& Systems = Craft->GetSystems();
    if (!Variant || Fitted.SystemIndex < 0 || Fitted.SystemIndex >= Systems.size() || !Systems[Fitted.SystemIndex])
    { Result = INVTEXT("Component is incompatible with this slot."); return false; }
    auto* Target = Systems[Fitted.SystemIndex]; auto& Parts = Target->GetComponents();
    if (Fitted.ComponentIndex < 0 || Fitted.ComponentIndex >= Parts.size() || !Parts[Fitted.ComponentIndex]) return false;
    const auto* Current = Parts[Fitted.ComponentIndex];
    const double NewPerformance = FMath::Pow(Variant->PerformanceMultiplier / Current->PerformanceMultiplier(), 1.0 / Parts.size());
    const double NewDemand = FMath::Pow(Variant->PowerDemandMultiplier / Current->PowerDemandMultiplier(), 1.0 / Parts.size());
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
    const int64 Price = Variant->BaseCost;
    if (GetBalance() < Price)
    { Result = INVTEXT("Insufficient credits for this refit."); return false; }
    const auto Previous = Pilot;
    FPlayerHangarShip PreviousCondition; FOwnedHullCondition::Capture(*Craft,PreviousCondition);
    SimComponent* NativePart=Components[Fitted.ComponentIndex];
    const SimComponent PreviousPart(*NativePart);
    if (!FEconomyLedger::Pay(Players->GetEconomy(), FEconomyLedger::Pilot(), TEXT("Shipyard"), Price, FGuid::NewGuid(), TEXT("Component refit"))) return false;
    Entry->Components[Slot].Level = Level;
    Entry->Components[Slot].ItemId = FGuid::NewGuid();
    Entry->Components[Slot].bHasCondition=true;
    Entry->Components[Slot].ConditionPercent=100;
    Entry->Components[Slot].ConditionStatus=static_cast<int32>(SYSTEM_STATUS::NOMINAL);
    Entry->Components[Slot].MaintenanceSeconds=0;
    Entry->Components[Slot].JerryCount=0;
    Entry->Components[Slot].SpareCount=Variant->Spares;
    ComponentDesign Native;
    Native.name = TCHAR_TO_UTF8(*Variant->Name);
    Native.abrv = TCHAR_TO_UTF8(*Variant->Abbrev);
    Native.base_cost = Variant->BaseCost;
                        Native.performance_multiplier = Variant->PerformanceMultiplier;
                        Native.power_demand_multiplier = Variant->PowerDemandMultiplier;
                        Native.repair_time = Variant->RepairTime;
    Native.replace_time = Variant->ReplaceTime;
    Native.spares = Variant->Spares;
    Native.affects = Variant->Affects;
    NativePart->InstallDesign(Native);
    if (!Entry->bAssigned) FOwnedHullCondition::Capture(*Craft,*Entry);
    if (!Players->SaveActivePlayerProgress())
    {
        Pilot=Previous; *NativePart=PreviousPart; FOwnedHullCondition::Apply(*Craft,PreviousCondition);
        Result=INVTEXT("Could not save this fitting. Component, condition and credits restored."); return false;
    }
    Result = INVTEXT("Component refitted.");
    RefreshAssignment();
    return true;
}

void UShipInsuranceSubsystem::BeginMission()
{
    bRestoreLoadout = true;
    RefreshAssignment();
}

bool UShipInsuranceSubsystem::CanModifyHull(FGuid Hull) const
{
    if (!CanInsure()) return false;
    const auto* Entry = FindShip(Hull);
    if (!Entry || Entry->bLost) return false;
    auto* Simulation = Sim::GetSim();
    const auto* Mission = Simulation ? Simulation->GetMission() : nullptr;
    const auto* Players = GetGameInstance()->GetSubsystem<UStarshatterPlayerSubsystem>();
    return Players && (!Mission || Mission->IsFreeFlight() || Players->GetPlayerInfo().AssignedHangarHull == Hull);
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
