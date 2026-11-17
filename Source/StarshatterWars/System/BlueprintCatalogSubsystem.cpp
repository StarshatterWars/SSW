#include "BlueprintCatalogSubsystem.h"
#include "StarshatterPlayerSubsystem.h"
#include "Engine/GameInstance.h"
#include "EconomyRuntime.h"
#include "ManufacturingAI.h"
#include "StationServices.h"
#include "Ship.h"
#include "Sim.h"
#include "SimRegion.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "JsonObjectConverter.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"

namespace
{
bool DockedNPC()
{
    auto* Simulation = Sim::GetSim(); const auto* Craft = Simulation ? Simulation->GetPlayerShip() : nullptr;
    const auto* Station = Craft ? Craft->GetCarrier() : nullptr;
    return Simulation && !Simulation->IsNetworked() && Simulation->HasSimulationAuthority() && Craft && !Craft->IsDead() && !Craft->IsDying() &&
        Craft->GetFlightPhase() <= EOPSMode::ALERT && Station && !Station->HasNetworkPilot() && !Station->IsDead() && !Station->IsDying();
}
}
void UBlueprintCatalogSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{ Super::Initialize(Collection); LoadCatalog(); }
bool UBlueprintCatalogSubsystem::LoadCatalog()
{
    FString Json; TArray<TSharedPtr<FJsonValue>> Rows;
    if (!FFileHelper::LoadFileToString(Json, *(FPaths::ProjectContentDir() / TEXT("GameData/Production/blueprints.json"))) ||
        !FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(Json), Rows)) return false;
    TMap<FName, FBlueprintOffer> Imported;
    const auto* Recipes = LoadObject<UDataTable>(nullptr, TEXT("/Game/Game/DT_ProductionRecipes.DT_ProductionRecipes"));
    for (const auto& Value : Rows)
    {
        const TSharedPtr<FJsonObject>* Object = nullptr; FBlueprintOffer Row;
        if (!Value->TryGetObject(Object) || !Object || !Object->IsValid() || !FJsonObjectConverter::JsonObjectToUStruct(Object->ToSharedRef(), &Row) ||
            Row.Recipe.IsNone() || Row.DisplayName.IsEmpty() || Imported.Contains(Row.Recipe) || Row.OriginalPrice <= 0 ||
            Row.OriginalPrice > 1000000000000LL || Row.CopyPrice <= 0 || Row.CopyPrice >= Row.OriginalPrice || Row.CopyRuns <= 0 || Row.CopyRuns > 1000000 ||
            !Recipes || !Recipes->FindRow<FSSWProductionRecipe>(Row.Recipe, TEXT("Blueprint catalog"), false)) return false;
        Imported.Add(Row.Recipe, Row);
    }
    Offers = MoveTemp(Imported); UE_LOG(LogTemp, Display, TEXT("[Blueprints] Loaded %d NPC blueprint offers."), Offers.Num());
    return !Offers.IsEmpty();
}
FGuid UBlueprintCatalogSubsystem::PurchaseLicense(FEconomyState& State, const FBlueprintOffer& Offer, bool Original)
{
    const int64 Cost = Original ? Offer.OriginalPrice : Offer.CopyPrice;
    if (Offer.Recipe.IsNone() || Cost <= 0 || (!Original && Offer.CopyRuns <= 0)) return {};
    const FGuid Receipt = FGuid::NewGuid();
    if (!FEconomyLedger::Pay(State, FEconomyLedger::Pilot(), TEXT("NPC blueprint publisher"), Cost, Receipt, TEXT("Blueprint purchase"))) return {};
    const FGuid Blueprint = FEconomyLedger::GrantBlueprint(State, FEconomyLedger::Pilot(), Offer.Recipe, Original, Offer.CopyRuns);
    State.BlueprintReceipts.Add(Blueprint, Receipt); return Blueprint;
}
bool UBlueprintCatalogSubsystem::CanPurchase(FName Recipe, bool Original, FText& Reason) const
{
    const auto* Offer = Offers.Find(Recipe);
    if (!Offer) { Reason = INVTEXT("Blueprint offer unavailable."); return false; }
    if (!DockedNPC()) { Reason = INVTEXT("Dock at an NPC base to purchase blueprints."); return false; }
    const auto* Players = GetGameInstance()->GetSubsystem<UStarshatterPlayerSubsystem>();
    const auto& Pilot = Players->GetPlayerInfo();
    const int64 Balance = Pilot.Economy.Wallets.FindRef(FEconomyLedger::Pilot()) + (Pilot.Economy.bLegacyWalletMigrated ? 0 : Pilot.InsuranceCredits);
    if (Balance < (Original ? Offer->OriginalPrice : Offer->CopyPrice)) { Reason = INVTEXT("Insufficient credits for this blueprint."); return false; }
    Reason = INVTEXT("Originals allow unlimited runs. Copies have a limited number of runs."); return true;
}
bool UBlueprintCatalogSubsystem::Purchase(FName Recipe, bool Original, FText& Reason)
{
    if (!CanPurchase(Recipe, Original, Reason)) return false;
    auto* Players = GetGameInstance()->GetSubsystem<UStarshatterPlayerSubsystem>(); auto& State = Players->GetEconomy();
    const auto Before = Players->GetPlayerInfo();
    const FGuid Id = PurchaseLicense(State, Offers.FindChecked(Recipe), Original);
    if (!Id.IsValid() || !Players->SaveActivePlayerProgress())
    { Players->GetMutablePlayerInfo() = Before; Reason = INVTEXT("Could not save the blueprint purchase. No credits were charged."); return false; }
    Reason = INVTEXT("Blueprint purchased from the NPC publisher and added to your library."); return true;
}
FGuid UBlueprintCatalogSubsystem::BeginPlayerJob(const FSSWProductionFacility& Site, FGuid Blueprint, int32 Runs, FText& Reason)
{
    auto* Simulation = Sim::GetSim(); auto* Craft = Simulation ? Simulation->GetPlayerShip() : nullptr;
    if (!DockedNPC() || !Craft || !Craft->GetCarrier() || Site.Facility != UTF8_TO_TCHAR(Craft->GetCarrier()->GetName()))
    { Reason = INVTEXT("Dock at this production facility to install a job."); return {}; }
    auto* Players = GetGameInstance()->GetSubsystem<UStarshatterPlayerSubsystem>(); auto& State = Players->GetEconomy();
    const auto Before = State;
    const auto* BP = State.Blueprints.Find(Blueprint);
    if (!BP || BP->Owner != FEconomyLedger::Pilot() || BP->Recipe != Site.Recipe || !State.BlueprintReceipts.Contains(Blueprint))
    { Reason = INVTEXT("Purchase the correct blueprint from an NPC publisher first."); return {}; }
    const auto* Table = LoadObject<UDataTable>(nullptr, TEXT("/Game/Game/DT_ProductionRecipes.DT_ProductionRecipes"));
    const auto* Recipe = Table ? Table->FindRow<FSSWProductionRecipe>(Site.Recipe, TEXT("Player industry"), false) : nullptr;
    if (!Recipe) return {};
    const FName Input = FEconomyRuntime::Warehouse(Site.System, Site.Facility, TEXT("Player Input"));
    const FName Output = FEconomyRuntime::Warehouse(Site.System, Site.Facility, TEXT("Player Output"));
    FEconomyLedger::CreateInventory(State, Input, FEconomyLedger::Pilot(), FName(*Site.System), Site.CapacityTonnes);
    FEconomyLedger::CreateInventory(State, Output, FEconomyLedger::Pilot(), FName(*Site.System), Site.CapacityTonnes);
    TArray<FCommodityAmount> Materials, Products;
    if (!ManufacturingAI::BuildBatch(State, Input, *Recipe, Materials, Products))
    { Reason = INVTEXT("Deliver every required material to the facility input inventory first."); return {}; }
    const FGuid Id = FEconomyLedger::QueueBlueprintJob(State, FEconomyLedger::Pilot(), FName(*Site.Facility), Input, Output, Blueprint, Runs, Materials, Products, Recipe->SecondsPerBatch);
    if (!Id.IsValid()) { Reason = INVTEXT("Blueprint is busy or has insufficient licensed runs."); return {}; }
    if (!Players->SaveActivePlayerProgress()) { State = Before; Reason = INVTEXT("Could not save the job. No materials were consumed."); return {}; }
    Reason = INVTEXT("Manufacturing job installed."); return Id;
}
