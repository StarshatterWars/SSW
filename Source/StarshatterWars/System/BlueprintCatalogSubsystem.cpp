#include "BlueprintCatalogSubsystem.h"
#include "ProductionCatalog.h"
#include "StarshatterPlayerSubsystem.h"
#include "TransactionAI.h"
#include "Engine/GameInstance.h"
#include "EconomyRuntime.h"
#include "ManufacturingAI.h"
#include "ReprocessingAI.h"
#include "StationServices.h"
#include "SkillsSubsystem.h"
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
    const auto& Recipes = SSWProductionCatalog::Recipes();
    for (const auto& Value : Rows)
    {
        const TSharedPtr<FJsonObject>* Object = nullptr; FBlueprintOffer Row;
        if (!Value->TryGetObject(Object) || !Object || !Object->IsValid() || !FJsonObjectConverter::JsonObjectToUStruct(Object->ToSharedRef(), &Row) ||
            Row.Recipe.IsNone() || Row.DisplayName.IsEmpty() || Imported.Contains(Row.Recipe) || Row.OriginalPrice <= 0 ||
            Row.OriginalPrice > 1000000000000LL || Row.CopyPrice <= 0 || Row.CopyPrice >= Row.OriginalPrice || Row.CopyRuns <= 0 || Row.CopyRuns > 1000000 ||
            !Recipes.Contains(Row.Recipe)) return false;
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
    auto& PurchaseReceipt = State.Journal.Last(); PurchaseReceipt.Item = Offer.Recipe; PurchaseReceipt.Quantity = 1;
    PurchaseReceipt.UnitPrice = double(Cost); PurchaseReceipt.Reference = Blueprint;
    State.BlueprintReceipts.Add(Blueprint, Receipt); return Blueprint;
}
bool UBlueprintCatalogSubsystem::CanPurchase(FName Recipe, bool Original, FText& Reason) const
{
    const auto* Offer = Offers.Find(Recipe);
    if (!Offer) { Reason = INVTEXT("Blueprint offer unavailable."); return false; }
    if (!DockedNPC()) { Reason = INVTEXT("Dock at an NPC base to purchase blueprints."); return false; }
    if(!FStationServices::Offers(ESSWStationService::Blueprints)){Reason=INVTEXT("This dock has no blueprint publisher.");return false;}
    const auto* Players = GetGameInstance()->GetSubsystem<UStarshatterPlayerSubsystem>();
    if (!Players) { Reason = INVTEXT("Player data is unavailable."); return false; }
    const auto& Pilot = Players->GetPlayerInfo();
    const int64 Balance = TransactionAI::BalanceIncludingLegacy(Pilot.Economy, Pilot.InsuranceCredits);
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
bool UBlueprintCatalogSubsystem::MeetsRecipeSkills(const FSSWProductionRecipe& Recipe,
    const TMap<FName, FSkillProgress>* PilotSkills, FText& Reason, const TMap<FName, FSkillDefinition>* SkillCatalog)
{
    if (!Recipe.HasValidSkillRequirements())
    { Reason = INVTEXT("The recipe has invalid skill requirements."); return false; }
    TArray<FName> Required; Recipe.RequiredSkillLevels.GetKeys(Required);
    Required.Sort(FNameLexicalLess());
    for (FName Id : Required)
    {
        const int32 Level = Recipe.RequiredSkillLevels.FindChecked(Id);
        const auto* Definition = SkillCatalog ? SkillCatalog->Find(Id) : nullptr;
        if (SkillCatalog && (!Definition || Level > Definition->MaximumLevel))
        { Reason = INVTEXT("The recipe references an unavailable skill or level."); return false; }
        const auto* Progress = PilotSkills ? PilotSkills->Find(Id) : nullptr;
        // Purchased books grant a complete skill today; saved levels support future training.
        const int32 Actual = Progress ? (Progress->bComplete ? (Definition ? Definition->MaximumLevel : 5)
            : FMath::Clamp(Progress->TrainedLevel, 0, 5)) : 0;
        if (Actual < Level)
        {
            const FText Name = Definition ? FText::FromString(Definition->DisplayName)
                : FText::FromString(Id.ToString().Replace(TEXT("_"), TEXT(" ")));
            Reason = FText::Format(INVTEXT("Requires {0}, level {1}. Your level: {2}."), Name, FText::AsNumber(Level), FText::AsNumber(Actual));
            return false;
        }
    }
    Reason = FText::GetEmpty(); return true;
}
bool UBlueprintCatalogSubsystem::QuotePlayerJob(const FEconomyState& State, const FSSWProductionFacility& Site,
    const FSSWProductionRecipe& Recipe, FGuid Blueprint, int32 Runs, FPlayerIndustryQuote& Quote, FText& Reason,
    const TMap<FName, FSkillProgress>* PilotSkills, const TMap<FName, FSkillDefinition>* SkillCatalog)
{
    Quote = {};
    if(!Recipe.ShipDesign.IsNone() && Site.FacilityType!=ESSWProductionFacilityType::Shipyard){Reason=INVTEXT("Ships can only be constructed at a shipyard.");return false;}
    if (!MeetsRecipeSkills(Recipe, PilotSkills, Reason, SkillCatalog)) return false;
    if (!Site.bPlayerManufacturing) { Reason=INVTEXT("This facility does not offer player manufacturing."); return false; }
    if (!FMath::IsFinite(Site.PlayerMaterialMultiplier) || Site.PlayerMaterialMultiplier<.5 || Site.PlayerMaterialMultiplier>2 ||
        !FMath::IsFinite(Site.PlayerTimeMultiplier) || Site.PlayerTimeMultiplier<.25 || Site.PlayerTimeMultiplier>4 ||
        Site.InstallationFeeCredits<0 || Site.PerRunFeeCredits<0 || Site.MaximumPlayerRuns<1)
    { Reason=INVTEXT("The facility service terms are invalid."); return false; }
    Quote.ServiceProfile=Site.ServiceProfile;
    if (Runs>Site.MaximumPlayerRuns) { Reason=FText::Format(INVTEXT("This facility accepts at most {0} runs per installation."),FText::AsNumber(Site.MaximumPlayerRuns)); return false; }
    const double Fee=double(Site.InstallationFeeCredits)+double(Site.PerRunFeeCredits)*Runs;
    if (!FMath::IsFinite(Fee) || Fee>9.e15) { Reason=INVTEXT("Installation fee exceeds the supported balance limit."); return false; }
    Quote.InstallationFee=FMath::CeilToInt64(Fee);
    if (Quote.InstallationFee>State.Wallets.FindRef(FEconomyLedger::Pilot())) { Reason=INVTEXT("Insufficient credits for this facility's installation fee."); return false; }
    const auto* BP = State.Blueprints.Find(Blueprint);
    if (!BP || BP->Owner != FEconomyLedger::Pilot() || !Site.SupportsRecipe(BP->Recipe) ||
        !State.BlueprintReceipts.Contains(Blueprint) || !State.CommittedTransactions.Contains(State.BlueprintReceipts.FindRef(Blueprint)))
    { Reason = INVTEXT("Purchase the correct blueprint from an NPC publisher first."); return false; }
    if (Runs <= 0 || Runs > 1000000) { Reason = INVTEXT("Choose between 1 and 1,000,000 runs."); return false; }
    if (!BP->bOriginal && BP->RunsRemaining < Runs) { Reason = INVTEXT("The blueprint copy has insufficient remaining runs."); return false; }
    if (!FMath::IsFinite(BP->MaterialEfficiency) || BP->MaterialEfficiency < 0 || BP->MaterialEfficiency > .1 ||
        !FMath::IsFinite(BP->TimeEfficiency) || BP->TimeEfficiency < 0 || BP->TimeEfficiency > .2)
    { Reason = INVTEXT("The blueprint has invalid efficiency data."); return false; }
    if (!ReprocessingAI::Calculate(Recipe).bValid) { Reason = INVTEXT("The facility recipe is invalid."); return false; }
    Quote.Input = FEconomyRuntime::Warehouse(Site.System, Site.Facility, TEXT("Player Input"));
    Quote.Output = FEconomyRuntime::Warehouse(Site.System, Site.Facility, TEXT("Player Output"));
    // A preflight needs this facility's inventories/license, not a copy of the entire galaxy economy.
    FEconomyState Candidate;
    if (const auto* Inventory = State.Inventories.Find(Quote.Input)) Candidate.Inventories.Add(Quote.Input, *Inventory);
    if (const auto* Inventory = State.Inventories.Find(Quote.Output)) Candidate.Inventories.Add(Quote.Output, *Inventory);
    Candidate.Blueprints.Add(Blueprint, *BP);
    Candidate.BlueprintReceipts.Add(Blueprint, State.BlueprintReceipts.FindRef(Blueprint));
    Candidate.CommittedTransactions.Add(State.BlueprintReceipts.FindRef(Blueprint));
    for (const auto& Pair : State.Reservations)
        if (Pair.Value.Inventory == Quote.Input) Candidate.Reservations.Add(Pair.Key, Pair.Value);
    for (const auto& Job : State.Jobs)
        if (Job.Blueprint == Blueprint && Job.State != EIndustryJobState::Completed && Job.State != EIndustryJobState::Cancelled) Candidate.Jobs.Add(Job);
    if (!FEconomyLedger::CreateInventory(Candidate, Quote.Input, FEconomyLedger::Pilot(), FName(*Site.System), Site.CapacityTonnes) ||
        !FEconomyLedger::CreateInventory(Candidate, Quote.Output, FEconomyLedger::Pilot(), FName(*Site.System), Site.CapacityTonnes))
    { Reason = INVTEXT("The facility input or output inventory is unavailable."); return false; }
    TArray<FCommodityAmount> Materials = Recipe.Inputs, Products;
    const double Factor = Runs * (1.0 - BP->MaterialEfficiency);
    for (auto& Material : Materials) Material.Quantity *= Site.PlayerMaterialMultiplier;
    if (Materials.IsEmpty())
    {
        double Needed = Recipe.InputTonnes * Factor * Site.PlayerMaterialMultiplier;
        const auto& Inventory = Candidate.Inventories.FindChecked(Quote.Input);
        for (const auto& Stack : Inventory.Items)
        {
            const bool Accepted = Recipe.Minerals.ContainsByPredicate([&](ESSWMineral Mineral)
            { return DataTableUtils::MakeValidName(StaticEnum<ESSWMineral>()->GetNameStringByValue(static_cast<int64>(Mineral))) == Stack.Type; });
            if (!Accepted || Stack.UnitTonnes != 1) continue;
            const double Amount = FMath::Min(Needed, FEconomyLedger::Available(Candidate, Quote.Input, Stack.Type));
            if (Amount <= 0) continue;
            FCommodityAmount Material; Material.Item = Stack.Type; Material.Quantity = Amount / Factor;
            Materials.Add(Material); Needed -= Amount;
            if (Needed <= UE_SMALL_NUMBER) break;
        }
        if (Needed > UE_SMALL_NUMBER) { Reason = FText::Format(INVTEXT("Deliver {0} more tonnes of accepted, unreserved ore to Player Input."), FText::AsNumber(Needed)); return false; }
    }
    FCommodityAmount Product;
    Product.Item = DataTableUtils::MakeValidName(Recipe.CargoItem.IsEmpty() ? Recipe.OutputDisplayName : Recipe.CargoItem);
    Product.Quantity = Recipe.OutputQuantity > 0 ? Recipe.OutputQuantity : Recipe.OutputTonnes;
    Product.UnitTonnes = Product.Quantity > 0 ? Recipe.OutputTonnes / Product.Quantity : 0;
    Product.bWholeUnits = !Recipe.OutputUnit.IsEmpty() && !Recipe.OutputUnit.Equals(TEXT("t"),ESearchCase::IgnoreCase);
    Products.Add(Product);
    Quote.NominalMaterials=Materials; Quote.NominalProducts=Products; Quote.NominalDuration=Recipe.SecondsPerBatch*Site.PlayerTimeMultiplier;
    const FGuid Id = FEconomyLedger::QueueBlueprintJob(Candidate, FEconomyLedger::Pilot(), FName(*Site.Facility), Quote.Input, Quote.Output,
        Blueprint, Runs, Materials, Products, Recipe.SecondsPerBatch * Site.PlayerTimeMultiplier);
    if (!Id.IsValid()) { Reason = INVTEXT("The blueprint is busy or its recipe and inventories are incompatible."); return false; }
    const auto& Job = Candidate.Jobs.Last();
    Quote.Materials = Job.Materials; Quote.Products = Job.Products; Quote.Duration = Job.Duration;
    if (!FEconomyLedger::CanStartJob(Candidate, Job, true, Reason, false)) return false;
    if (State.Jobs.ContainsByPredicate([&Site](const auto& Existing) { return Existing.Facility == FName(*Site.Facility) && Existing.State == EIndustryJobState::Running; }))
        Reason = INVTEXT("Ready to queue. This facility is busy; your job will start when its current batch finishes.");
    return true;
}
bool UBlueprintCatalogSubsystem::CanBeginPlayerJob(const FSSWProductionFacility& Site, FGuid Blueprint, int32 Runs,
    FPlayerIndustryQuote& Quote, FText& Reason) const
{
    const auto* Local = FStationServices::Facility();
    if (!DockedNPC() || !Local || Local->Facility != Site.Facility || Local->System != Site.System || Local->Recipe != Site.Recipe)
    { Reason = INVTEXT("Dock at this production facility to install a job."); return false; }
    const auto* Players = GetGameInstance()->GetSubsystem<UStarshatterPlayerSubsystem>();
    const auto* BlueprintRow = Players ? Players->GetPlayerInfo().Economy.Blueprints.Find(Blueprint) : nullptr;
    const FName RecipeId = BlueprintRow ? BlueprintRow->Recipe : NAME_None;
    if (!Local->SupportsRecipe(RecipeId))
    { Reason = INVTEXT("This facility does not manufacture the selected recipe."); return false; }
    const auto* Recipe = SSWProductionCatalog::Recipes().Find(RecipeId);
    if (!Players || !Recipe) { Reason = INVTEXT("Player industry data is unavailable."); return false; }
    const auto* Skills = GetGameInstance()->GetSubsystem<USkillsSubsystem>();
    if (!Recipe->RequiredSkillLevels.IsEmpty() && (!Skills || Skills->GetCatalog().IsEmpty()))
    { Reason = INVTEXT("The skill catalog is unavailable; manufacturing cannot be authorized."); return false; }
    return QuotePlayerJob(Players->GetPlayerInfo().Economy, *Local, *Recipe, Blueprint, Runs, Quote, Reason,
        &Players->GetPlayerInfo().Skills, Skills ? &Skills->GetCatalog() : nullptr);
}
FGuid UBlueprintCatalogSubsystem::BeginPlayerJob(const FSSWProductionFacility& Site, FGuid Blueprint, int32 Runs, FText& Reason)
{
    FPlayerIndustryQuote Quote;
    if (!CanBeginPlayerJob(Site, Blueprint, Runs, Quote, Reason)) return {};
    auto* Players = GetGameInstance()->GetSubsystem<UStarshatterPlayerSubsystem>();
    const auto Before = Players->GetPlayerInfo(); auto& State = Players->GetEconomy();
    FEconomyLedger::CreateInventory(State, Quote.Input, FEconomyLedger::Pilot(), FName(*Site.System), Site.CapacityTonnes);
    FEconomyLedger::CreateInventory(State, Quote.Output, FEconomyLedger::Pilot(), FName(*Site.System), Site.CapacityTonnes);
    // Preserve nominal inputs: reversing a rounded discrete preview would apply efficiency twice.
    const FGuid Id = FEconomyLedger::QueueBlueprintJob(State, FEconomyLedger::Pilot(), FName(*Site.Facility), Quote.Input, Quote.Output,
        Blueprint, Runs, Quote.NominalMaterials, Quote.NominalProducts, Quote.NominalDuration);
    if (!Id.IsValid() || (Quote.InstallationFee>0 && !FEconomyLedger::Pay(State,FEconomyLedger::Pilot(),TEXT("NPC facility services"),Quote.InstallationFee,Id,TEXT("Manufacturing installation"))) || !Players->SaveActivePlayerProgress())
    { Players->GetMutablePlayerInfo() = Before; Reason = INVTEXT("Could not save the job. No materials or licensed runs were consumed."); return {}; }
    Reason = INVTEXT("Manufacturing job installed. Output is delivered automatically."); return Id;
}
bool UBlueprintCatalogSubsystem::CancelPlayerJob(FGuid Id, FText& Reason)
{
    auto* Simulation = Sim::GetSim(); const auto* Craft = Simulation ? Simulation->GetPlayerShip() : nullptr;
    auto* Players = GetGameInstance()->GetSubsystem<UStarshatterPlayerSubsystem>();
    if (!DockedNPC() || !Players || !Craft || !Craft->GetCarrier()) { Reason = INVTEXT("Dock at the job's facility to cancel it."); return false; }
    const auto Before = Players->GetPlayerInfo(); auto& State = Players->GetEconomy();
    const auto* Job = State.Jobs.FindByPredicate([Id](const auto& Entry) { return Entry.Id == Id; });
    if (!Job || Job->Owner != FEconomyLedger::Pilot() || Job->Facility != FName(UTF8_TO_TCHAR(Craft->GetCarrier()->GetName())))
    { Players->GetMutablePlayerInfo() = Before; Reason = INVTEXT("Select your own job at this docked facility."); return false; }
    if (!FEconomyLedger::CancelJob(State, FEconomyLedger::Pilot(), Id) || !Players->SaveActivePlayerProgress())
    { Players->GetMutablePlayerInfo() = Before; Reason = INVTEXT("The job could not be cancelled and saved."); return false; }
    Reason = INVTEXT("Job cancelled. Materials and licensed runs already consumed by a running job are not refunded."); return true;
}
