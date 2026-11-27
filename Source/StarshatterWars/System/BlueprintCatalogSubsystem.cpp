#include "BlueprintCatalogSubsystem.h"
#include "LedgerCommandAI.h"
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
    FString ServiceJson;FBlueprintServiceTerms NewTerms;
    if(!FFileHelper::LoadFileToString(ServiceJson,*(FPaths::ProjectContentDir()/TEXT("GameData/Production/blueprint_services.json"))) || !FJsonObjectConverter::JsonObjectStringToUStruct(ServiceJson,&NewTerms,0,0) ||
        !FMath::IsFinite(NewTerms.ResearchMaterialSeconds) || NewTerms.ResearchMaterialSeconds<=0 || NewTerms.ResearchMaterialSeconds>31536000 ||
        !FMath::IsFinite(NewTerms.ResearchTimeSeconds) || NewTerms.ResearchTimeSeconds<=0 || NewTerms.ResearchTimeSeconds>31536000 ||
        !FMath::IsFinite(NewTerms.CopySecondsPerRun) || NewTerms.CopySecondsPerRun<=0 || NewTerms.CopySecondsPerRun>31536 ||
        NewTerms.ResearchMaterialCredits<0 || NewTerms.ResearchMaterialCredits>1000000000000LL || NewTerms.ResearchTimeCredits<0 || NewTerms.ResearchTimeCredits>1000000000000LL ||
        NewTerms.CopyCreditsPerRun<0 || NewTerms.CopyCreditsPerRun>1000000000LL || NewTerms.MaximumCopyRuns<1 || NewTerms.MaximumCopyRuns>1000)return false;
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
    Offers = MoveTemp(Imported);ServiceTerms=NewTerms; UE_LOG(LogTemp, Display, TEXT("[Blueprints] Loaded %d NPC blueprint offers."), Offers.Num());
    return !Offers.IsEmpty();
}
FGuid UBlueprintCatalogSubsystem::PurchaseLicense(FEconomyState& State, const FBlueprintOffer& Offer, bool Original)
{
    const int64 Cost = Original ? Offer.OriginalPrice : Offer.CopyPrice;
    if (Offer.Recipe.IsNone() || Cost <= 0 || (!Original && Offer.CopyRuns <= 0)) return {};
    const FGuid Receipt = FGuid::NewGuid();
    if (!FEconomyLedger::Pay(State, FEconomyLedger::Pilot(), TEXT("NPC blueprint publisher"), Cost, Receipt, TEXT("Blueprint purchase"),ECommercePurpose::Purchase)) return {};
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
    if(!Site.IndustryPolicy().Allows(Recipe.Activity)){Reason=INVTEXT("This facility has no lane for this activity.");return false;}
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
        if ((Job.Blueprint == Blueprint || Job.Facility==FName(*Site.Facility)) && Job.State != EIndustryJobState::Completed && Job.State != EIndustryJobState::Cancelled) Candidate.Jobs.Add(Job);
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
            const bool Accepted=ManufacturingAI::MineralKeys(Recipe).Contains(Stack.Type);
            if (!Accepted || Stack.UnitTonnes != 1) continue;
            const double Amount = FMath::Min(Needed, FEconomyLedger::Available(Candidate, Quote.Input, Stack.Type));
            if (Amount <= 0) continue;
            FCommodityAmount Material; Material.Item = Stack.Type; Material.Quantity = Amount / Factor;
            Materials.Add(Material); Needed -= Amount;
            if (Needed <= UE_SMALL_NUMBER) break;
        }
        if (Needed > UE_SMALL_NUMBER) { Reason = FText::Format(INVTEXT("Deliver {0} more tonnes of accepted, unreserved ore to Player Input."), FText::AsNumber(Needed)); return false; }
    }
    if(!Recipe.ResolveOutputs(Products)){Reason=INVTEXT("Invalid recipe outputs.");return false;}
    Candidate.IndustryPolicies.Add(FName(*Site.Facility),Site.IndustryPolicy());Candidate.RecipeActivities.Add(BP->Recipe,Recipe.Activity);
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
    State.IndustryPolicies.Add(FName(*Site.Facility),Site.IndustryPolicy());
    if(const auto* BP=State.Blueprints.Find(Blueprint))if(const auto* Recipe=SSWProductionCatalog::Recipes().Find(BP->Recipe))State.RecipeActivities.Add(BP->Recipe,Recipe->Activity);
    // Preserve nominal inputs: reversing a rounded discrete preview would apply efficiency twice.
    const FGuid Id = FEconomyLedger::QueueBlueprintJob(State, FEconomyLedger::Pilot(), FName(*Site.Facility), Quote.Input, Quote.Output,
        Blueprint, Runs, Quote.NominalMaterials, Quote.NominalProducts, Quote.NominalDuration);
    if (!Id.IsValid() || (Quote.InstallationFee>0 && !FEconomyLedger::Pay(State,FEconomyLedger::Pilot(),TEXT("NPC facility services"),Quote.InstallationFee,Id,TEXT("Manufacturing installation"),ECommercePurpose::Service)) || !Players->SaveActivePlayerProgress())
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

bool UBlueprintCatalogSubsystem::RequestPurchase(FName Recipe,bool Original,TFunction<void(bool,const FText&,FGuid)>&& Completed,FText& Reason)
{
    if(!CanPurchase(Recipe,Original,Reason))return false;const auto Offer=Offers.FindChecked(Recipe);
    FNativeLedgerCommand C;C.Action=ELedgerCommand::BlueprintPurchase;C.Receipt=FGuid::NewGuid();C.Recipe=Recipe;C.bOriginal=Original;C.Credits=Original?Offer.OriginalPrice:Offer.CopyPrice;C.Runs=Offer.CopyRuns;
    const TWeakObjectPtr<UBlueprintCatalogSubsystem> Weak(this);
    return FStationServices::RequestLedgerAction(GetGameInstance(),C,INVTEXT("BLUEPRINT PURCHASE"),[Weak,Recipe,Original,C](FText& Why)
    {
        const auto* Catalog=Weak.Get();const auto* Current=Catalog?Catalog->Offers.Find(Recipe):nullptr;
        return Current && (Original?Current->OriginalPrice:Current->CopyPrice)==C.Credits && Current->CopyRuns==C.Runs && Catalog->CanPurchase(Recipe,Original,Why);
    },MoveTemp(Completed),Reason);
}
bool UBlueprintCatalogSubsystem::RequestBeginPlayerJob(const FSSWProductionFacility& Site,FGuid Blueprint,int32 Runs,TFunction<void(bool,const FText&,FGuid)>&& Completed,FText& Reason)
{
    FPlayerIndustryQuote Quote;if(!CanBeginPlayerJob(Site,Blueprint,Runs,Quote,Reason))return false;
    FNativeLedgerCommand C;C.Action=ELedgerCommand::IndustryInstall;C.Receipt=FGuid::NewGuid();C.Object=Blueprint;C.Runs=Runs;C.Source=Quote.Input;C.Destination=Quote.Output;
    C.Facility=FName(*Site.Facility);C.System=FName(*Site.System);C.Capacity=Site.CapacityTonnes;C.Materials=Quote.NominalMaterials;C.Products=Quote.NominalProducts;C.Duration=Quote.NominalDuration;C.Credits=Quote.InstallationFee;C.Policy=Site.IndustryPolicy();
    const auto* P=GetGameInstance()->GetSubsystem<UStarshatterPlayerSubsystem>();const auto* BP=P?P->GetPlayerInfo().Economy.Blueprints.Find(Blueprint):nullptr;
    if(!BP)return false;C.Activity=SSWProductionCatalog::Recipes().FindChecked(BP->Recipe).Activity;
    const TWeakObjectPtr<UBlueprintCatalogSubsystem> Weak(this);
    return FStationServices::RequestLedgerAction(GetGameInstance(),C,INVTEXT("INSTALL MANUFACTURING JOB"),[Weak,Site,Blueprint,Runs,C](FText& Why)
    {
        FPlayerIndustryQuote Fresh;if(!Weak.IsValid() || !Weak->CanBeginPlayerJob(Site,Blueprint,Runs,Fresh,Why))return false;
        const auto* LiveSite=FStationServices::Facility();if(!LiveSite || LiveSite->ProductionLanes!=C.Policy.Lanes || LiveSite->AllowedActivities!=C.Policy.Activities)return false;
        if(Fresh.Input!=C.Source || Fresh.Output!=C.Destination || Fresh.InstallationFee!=C.Credits || Fresh.NominalDuration!=C.Duration || Fresh.NominalMaterials.Num()!=C.Materials.Num() || Fresh.NominalProducts.Num()!=C.Products.Num())return false;
        for(int32 I=0;I<C.Materials.Num();++I)if(!FCommodityAmount::StaticStruct()->CompareScriptStruct(&Fresh.NominalMaterials[I],&C.Materials[I],0))return false;
        for(int32 I=0;I<C.Products.Num();++I)if(!FCommodityAmount::StaticStruct()->CompareScriptStruct(&Fresh.NominalProducts[I],&C.Products[I],0))return false;
        return true;
    },MoveTemp(Completed),Reason);
}
bool UBlueprintCatalogSubsystem::RequestCancelPlayerJob(FGuid Id,TFunction<void(bool,const FText&,FGuid)>&& Completed,FText& Reason)
{
    FNativeLedgerCommand C;C.Action=ELedgerCommand::IndustryCancel;C.Receipt=FGuid::NewGuid();C.Object=Id;const TWeakObjectPtr<UBlueprintCatalogSubsystem> Weak(this);
    return FStationServices::RequestLedgerAction(GetGameInstance(),C,INVTEXT("CANCEL MANUFACTURING JOB"),[Weak,Id](FText& Why)
    {
        const auto* Players=Weak.IsValid()?Weak->GetGameInstance()->GetSubsystem<UStarshatterPlayerSubsystem>():nullptr;const auto* Site=FStationServices::Facility();
        const auto* Job=Players?Players->GetPlayerInfo().Economy.Jobs.FindByPredicate([Id](const auto& J){return J.Id==Id;}):nullptr;
        if(!DockedNPC() || !Site || !Job || Job->Owner!=FEconomyLedger::Pilot() || Job->Facility!=FName(*Site->Facility) || Job->State==EIndustryJobState::Completed || Job->State==EIndustryJobState::Cancelled)
        {Why=INVTEXT("Select your own cancellable job at this docked facility.");return false;}return true;
    },MoveTemp(Completed),Reason);
}

bool UBlueprintCatalogSubsystem::CanBlueprintService(const FSSWProductionFacility& Site,FGuid Blueprint,FName Activity,int32 Runs,double& Seconds,int64& Price,FText& Reason)const
{
    Seconds=0;Price=0;const auto* Local=FStationServices::Facility();
    if(!DockedNPC() || !Local || !Local->bPlayerManufacturing || Local->Facility!=Site.Facility || !Local->IndustryPolicy().Allows(Activity))
    {Reason=INVTEXT("Dock at a facility with a permitted blueprint-service lane.");return false;}
    const auto* Players=GetGameInstance()->GetSubsystem<UStarshatterPlayerSubsystem>();if(!Players)return false;const auto& State=Players->GetPlayerInfo().Economy;
    const auto* BP=State.Blueprints.Find(Blueprint);
    if(!BP || !BP->bOriginal || BP->Owner!=FEconomyLedger::Pilot() || !Local->SupportsRecipe(BP->Recipe) || !State.BlueprintReceipts.Contains(Blueprint) || !State.CommittedTransactions.Contains(State.BlueprintReceipts.FindRef(Blueprint)))
    {Reason=INVTEXT("Select your purchased original blueprint for this facility.");return false;}
    if(State.Jobs.ContainsByPredicate([Blueprint](const auto& J){return J.Blueprint==Blueprint && J.State!=EIndustryJobState::Completed && J.State!=EIndustryJobState::Cancelled;}))
    {Reason=INVTEXT("This original is already locked by a queued or running job.");return false;}
    if(Activity==TEXT("ResearchMaterial")){if(BP->MaterialEfficiency>=.1-1.e-9){Reason=INVTEXT("Material efficiency is at its 10% limit.");return false;}Seconds=ServiceTerms.ResearchMaterialSeconds;Price=ServiceTerms.ResearchMaterialCredits;}
    else if(Activity==TEXT("ResearchTime")){if(BP->TimeEfficiency>=.2-1.e-9){Reason=INVTEXT("Time efficiency is at its 20% limit.");return false;}Seconds=ServiceTerms.ResearchTimeSeconds;Price=ServiceTerms.ResearchTimeCredits;}
    else if(Activity==TEXT("Copying")){if(Runs<1 || Runs>ServiceTerms.MaximumCopyRuns){Reason=INVTEXT("Invalid licensed-copy run count.");return false;}Seconds=ServiceTerms.CopySecondsPerRun*Runs;Price=ServiceTerms.CopyCreditsPerRun*Runs;}
    else{Reason=INVTEXT("Unknown blueprint service.");return false;}
    const auto* Skills=GetGameInstance()->GetSubsystem<USkillsSubsystem>();
    const FName Required=Activity==TEXT("ResearchMaterial")?FName(TEXT("material_research")):Activity==TEXT("ResearchTime")?FName(TEXT("process_research")):FName(TEXT("blueprint_duplication"));
    if(!Skills || !Skills->HasSkill(Required,1)){Reason=FText::Format(INVTEXT("Train {0}, level I, before installing this service."),FText::FromName(Required));return false;}
    Seconds*=Local->PlayerTimeMultiplier;
    if(TransactionAI::BalanceIncludingLegacy(State,Players->GetPlayerInfo().InsuranceCredits)<Price){Reason=INVTEXT("Insufficient credits for this service.");return false;}
    Reason=FText::Format(INVTEXT("{0}: {1} CR (fees included), {2} seconds. Queues for a permitted lane. Results apply only on completion."),FText::FromName(Activity),FText::AsNumber(Price),FText::AsNumber(Seconds));return true;
}
bool UBlueprintCatalogSubsystem::RequestBlueprintService(const FSSWProductionFacility& Site,FGuid Blueprint,FName Activity,int32 Runs,TFunction<void(bool,const FText&,FGuid)>&& Completed,FText& Reason)
{
    double Seconds=0;int64 Price=0;if(!CanBlueprintService(Site,Blueprint,Activity,Runs,Seconds,Price,Reason))return false;
    FNativeLedgerCommand C;C.Action=ELedgerCommand::IndustryService;C.Object=Blueprint;C.Receipt=FGuid::NewGuid();C.Activity=Activity;C.Duration=Seconds;C.Credits=Price;C.Runs=Activity==TEXT("Copying")?Runs:1;C.Facility=FName(*Site.Facility);C.Policy=Site.IndustryPolicy();
    const TWeakObjectPtr<UBlueprintCatalogSubsystem> Weak(this);
    return FStationServices::RequestLedgerAction(GetGameInstance(),C,INVTEXT("BLUEPRINT RESEARCH / COPYING"),[Weak,Site,C](FText& Why)
    {
        double Duration=0;int64 Cost=0;const auto* Current=FStationServices::Facility();
        return Weak.IsValid() && Current && Current->ProductionLanes==C.Policy.Lanes && Current->AllowedActivities==C.Policy.Activities &&
            Weak->CanBlueprintService(Site,C.Object,C.Activity,C.Runs,Duration,Cost,Why) && Duration==C.Duration && Cost==C.Credits;
    },MoveTemp(Completed),Reason);
}
