#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "BlueprintCatalogSubsystem.h"
#include "ManufacturingAI.h"
#include "EconomyRuntime.h"
#include "JsonObjectConverter.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FIndustrySkillGateTest, "Starshatter.Economy.PlayerIndustrySkillGate",
    EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::EngineFilter)
bool FIndustrySkillGateTest::RunTest(const FString&)
{
    FEconomyState State;
    const FName Owner = FEconomyLedger::Pilot();
    FSSWProductionFacility Site;
    Site.System = TEXT("Janus"); Site.Facility = TEXT("Qualification Works");
    Site.Recipe = TEXT("QualificationRecipe"); Site.CapacityTonnes = 100;
    Site.InstallationFeeCredits = 25;
    State.Wallets.Add(Owner, 1000);
    const FName Input = FEconomyRuntime::Warehouse(Site.System, Site.Facility, TEXT("Player Input"));
    const FName Output = FEconomyRuntime::Warehouse(Site.System, Site.Facility, TEXT("Player Output"));
    FEconomyLedger::CreateInventory(State, Input, Owner, TEXT("Janus"), 100);
    FEconomyLedger::CreateInventory(State, Output, Owner, TEXT("Janus"), 100);
    FEconomyLedger::Deposit(State, Input, TEXT("Magnetite"), 20, 1);
    FSSWProductionRecipe Recipe;
    Recipe.InputTonnes = 10; Recipe.OutputTonnes = 2; Recipe.SecondsPerBatch = 60;
    Recipe.CargoItem = TEXT("concentrate"); Recipe.OutputQuantity = 4;
    FCommodityAmount Ore; Ore.Item = TEXT("Magnetite"); Ore.Quantity = 10; Ore.UnitTonnes = 1;
    Recipe.Inputs.Add(Ore);
    const FGuid Blueprint = FEconomyLedger::GrantBlueprint(State, Owner, Site.Recipe, false, 2);
    const FGuid Receipt = FGuid::NewGuid();
    State.BlueprintReceipts.Add(Blueprint, Receipt); State.CommittedTransactions.Add(Receipt);
    FPlayerIndustryQuote Quote; FText Reason;
    TestTrue(TEXT("Legacy recipe with no gates remains eligible without skill context"),
        UBlueprintCatalogSubsystem::QuotePlayerJob(State, Site, Recipe, Blueprint, 1, Quote, Reason));

    FSkillDefinition Definition; Definition.Name = TEXT("manufacturing");
    Definition.DisplayName = TEXT("Manufacturing"); Definition.MaximumLevel = 5;
    TMap<FName, FSkillDefinition> Catalog; Catalog.Add(Definition.Name, Definition);
    TMap<FName, FSkillProgress> Skills;
    Recipe.RequiredSkillLevels.Add(Definition.Name, 3);
    TestFalse(TEXT("Gated recipe cannot bypass checks with omitted skill context"),
        UBlueprintCatalogSubsystem::QuotePlayerJob(State, Site, Recipe, Blueprint, 1, Quote, Reason));
    TestFalse(TEXT("Unlearned skill blocks preflight"),
        UBlueprintCatalogSubsystem::QuotePlayerJob(State, Site, Recipe, Blueprint, 1, Quote, Reason, &Skills, &Catalog));
    TestTrue(TEXT("Missing skill is named in UI preflight reason"), Reason.ToString().Contains(TEXT("Manufacturing")) && Reason.ToString().Contains(TEXT("3")));
    TestTrue(TEXT("Blocked quote contains no material commitment"), Quote.Materials.IsEmpty());
    FSkillProgress Partial; Partial.TrainedLevel = 2; Skills.Add(Definition.Name, Partial);
    TestFalse(TEXT("Future partial training below requirement is rejected"),
        UBlueprintCatalogSubsystem::QuotePlayerJob(State, Site, Recipe, Blueprint, 1, Quote, Reason, &Skills, &Catalog));
    Skills.FindChecked(Definition.Name).TrainedLevel = 3;
    TestTrue(TEXT("Future exact trained level is accepted"),
        UBlueprintCatalogSubsystem::QuotePlayerJob(State, Site, Recipe, Blueprint, 1, Quote, Reason, &Skills, &Catalog));
    if (Quote.Materials.IsEmpty() || Quote.Products.IsEmpty()) return false;
    TestEqual(TEXT("Skill permission does not alter material quantity"), Quote.Materials[0].Quantity, 10.0);
    TestEqual(TEXT("Skill permission does not alter products"), Quote.Products[0].Quantity, 4.0);
    TestEqual(TEXT("Skill permission does not alter duration"), Quote.Duration, 60.0);
    TestEqual(TEXT("Skill permission preserves SSW service fee"), Quote.InstallationFee, int64(25));
    Skills.FindChecked(Definition.Name).TrainedLevel = 0;
    Skills.FindChecked(Definition.Name).bComplete = true;
    TestTrue(TEXT("Purchased complete skill satisfies all supported levels"),
        UBlueprintCatalogSubsystem::QuotePlayerJob(State, Site, Recipe, Blueprint, 1, Quote, Reason, &Skills, &Catalog));
    Recipe.RequiredSkillLevels.Add(TEXT("unregistered_skill"), 1);
    FSkillProgress Complete; Complete.bComplete = true; Skills.Add(TEXT("unregistered_skill"), Complete);
    TestFalse(TEXT("Unknown recipe skill fails even if a save contains that key"),
        UBlueprintCatalogSubsystem::QuotePlayerJob(State, Site, Recipe, Blueprint, 1, Quote, Reason, &Skills, &Catalog));
    Recipe.RequiredSkillLevels.Remove(TEXT("unregistered_skill"));
    Recipe.RequiredSkillLevels.FindChecked(Definition.Name) = 6;
    TestFalse(TEXT("Out of range requirement cannot authorize production"),
        UBlueprintCatalogSubsystem::QuotePlayerJob(State, Site, Recipe, Blueprint, 1, Quote, Reason, &Skills, &Catalog));
    Recipe.RequiredSkillLevels.FindChecked(Definition.Name) = 0;
    TestFalse(TEXT("Zero requirement is malformed rather than an implicit bypass"), Recipe.HasValidSkillRequirements());
    Recipe.RequiredSkillLevels.FindChecked(Definition.Name) = 3;
    Recipe.RequiredSkillLevels.Add(NAME_None, 1);
    TestFalse(TEXT("Empty skill identifier is malformed"), Recipe.HasValidSkillRequirements());
    Recipe.RequiredSkillLevels.Remove(NAME_None);
    TArray<FCommodityAmount> Materials, Products;
    TestTrue(TEXT("NPC batch construction remains independent of player qualifications"),
        ManufacturingAI::BuildBatch(State, Input, Recipe, Materials, Products));
    Recipe.RequiredSkillLevels.Reset();
    const FGuid Additional = FEconomyLedger::GrantBlueprint(State, Owner, TEXT("Make_mining_drone"), true, 0);
    State.BlueprintReceipts.Add(Additional, Receipt);
    TestFalse(TEXT("Factory cannot install an unsupported secondary recipe"),
        UBlueprintCatalogSubsystem::QuotePlayerJob(State, Site, Recipe, Additional, 1, Quote, Reason));
    Site.AdditionalRecipes.Add(TEXT("Make_mining_drone"));
    TestTrue(TEXT("Factory can install an explicitly supported additional recipe"),
        UBlueprintCatalogSubsystem::QuotePlayerJob(State, Site, Recipe, Additional, 1, Quote, Reason));
    TestTrue(TEXT("Primary automated recipe remains supported"), Site.SupportsRecipe(Site.Recipe));
    TestFalse(TEXT("Empty recipe never grants blanket factory access"), Site.SupportsRecipe(NAME_None));
    TestEqual(TEXT("Preflight never charges the pilot"), State.Wallets.FindChecked(Owner), int64(1000));
    TestEqual(TEXT("Preflight never consumes ore"), FEconomyLedger::Quantity(State, Input, Ore.Item), 20.0);
    TestEqual(TEXT("Preflight never consumes blueprint runs"), State.Blueprints.FindChecked(Blueprint).RunsRemaining, 2);
    TestTrue(TEXT("Preflight never installs jobs or writes receipts"), State.Jobs.IsEmpty() && State.Journal.IsEmpty());
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FIndustrySkillCatalogTest, "Starshatter.Economy.ProductionRecipeSkillJson",
    EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::EngineFilter)
bool FIndustrySkillCatalogTest::RunTest(const FString&)
{
    FString Json; TArray<TSharedPtr<FJsonValue>> SkillRows;
    const FString Directory = FPaths::ProjectContentDir() / TEXT("GameData");
    if (!TestTrue(TEXT("Native skill catalog reads"), FFileHelper::LoadFileToString(Json, *(Directory / TEXT("Skills/skills.json")))) ||
        !TestTrue(TEXT("Native skill JSON parses"), FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(Json), SkillRows))) return false;
    TMap<FName, FSkillDefinition> Catalog;
    for (const auto& Value : SkillRows)
    {
        FSkillDefinition Definition;
        if (!TestTrue(TEXT("Native skill row parses"), FJsonObjectConverter::JsonObjectToUStruct(Value->AsObject().ToSharedRef(), &Definition))) return false;
        Catalog.Add(Definition.Name, Definition);
    }
    if (!TestTrue(TEXT("Recipe JSON reads"), FFileHelper::LoadFileToString(Json, *(Directory / TEXT("Cargo/recipes.json"))))) return false;
    auto* Table = NewObject<UDataTable>(); Table->RowStruct = FSSWProductionRecipe::StaticStruct();
    TestTrue(TEXT("Recipe requirements use the existing data table ingestion format"), Table->CreateTableFromJSONString(Json).IsEmpty());
    TestTrue(TEXT("All existing production recipes remain present"), Table->GetRowMap().Num() >= 214);
    TestNotNull(TEXT("Mining drone recipe ingests through the standard table"), Table->FindRow<FSSWProductionRecipe>(TEXT("Make_mining_drone"), TEXT("Mining drone recipe test"), false));
    for (const auto& Pair : Table->GetRowMap())
    {
        const auto& Recipe = *reinterpret_cast<const FSSWProductionRecipe*>(Pair.Value);
        TestTrue(TEXT("Every authored recipe declares a player qualification"), !Recipe.RequiredSkillLevels.IsEmpty());
        TestTrue(TEXT("Requirement keys and levels are valid"), Recipe.HasValidSkillRequirements());
        TMap<FName, FSkillProgress> Qualified;
        for (const auto& Required : Recipe.RequiredSkillLevels)
        {
            const auto* Skill = Catalog.Find(Required.Key);
            TestTrue(TEXT("Required skill exists in native SSW catalog"), Skill && Required.Value <= Skill->MaximumLevel);
            FSkillProgress Progress; Progress.bComplete = true; Qualified.Add(Required.Key, Progress);
        }
        FText Reason;
        TestTrue(TEXT("Native complete books can qualify for every current recipe"),
            UBlueprintCatalogSubsystem::MeetsRecipeSkills(Recipe, &Qualified, Reason, &Catalog));
    }
    return true;
}
#endif
