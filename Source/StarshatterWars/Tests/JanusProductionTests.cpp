#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "GameStructs_products.h"
#include "GameStructs.h"
#include "GameStructs_System.h"
#include "ProductionActor.h"
#include "ShipVisualCatalog.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FJanusProductionTest, "Starshatter.Industry.JanusProduction",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FJanusProductionTest::RunTest(const FString&)
{
    auto* Recipes = LoadObject<UDataTable>(nullptr, TEXT("/Game/Game/DT_ProductionRecipes.DT_ProductionRecipes"));
    auto* Ships = LoadObject<UDataTable>(nullptr, TEXT("/Game/Game/DT_ShipDesign.DT_ShipDesign"));
    auto* Galaxy = LoadObject<UDataTable>(nullptr, TEXT("/Game/Game/DT_GalaxyMap.DT_GalaxyMap"));
    if (!TestNotNull(TEXT("Recipes"), Recipes) || !TestNotNull(TEXT("Ships"), Ships) ||
        !TestNotNull(TEXT("Galaxy"), Galaxy)) return false;
    const auto* Facility = Ships->FindRow<FShipDesign>(TEXT("Processing Facility"), TEXT("Test"));
    if (!TestNotNull(TEXT("Facility design"), Facility)) return false;
    const auto* Recipe = Recipes->FindRow<FSSWProductionRecipe>(Facility->ProductionRecipe, TEXT("Test"));
    if (!TestNotNull(TEXT("Authored recipe"), Recipe)) return false;
    TestTrue(TEXT("Production actor resolves"),
        ShipVisualCatalog::Resolve(TEXT("Processing Facility"), TEXT("Xfer")) == AProductionActor::StaticClass());
    const auto* Janus = Galaxy->FindRow<FS_Galaxy>(TEXT("Janus"), TEXT("Test"));
    if (TestNotNull(TEXT("Janus"), Janus))
        TestTrue(TEXT("Janus has both production sites"), Janus->ProductionSites.Contains(TEXT("Janek Orbital Materials Works")) && Janus->ProductionSites.Contains(TEXT("Dante Orbital Concentrator")));
    FSSWProductionStock Stock;
    TestFalse(TEXT("Waits for ore"), Stock.Advance(*Recipe, Facility->CargoCapacity.Tonnes, 20));
    Stock.Ore = Recipe->InputTonnes;
    TestFalse(TEXT("Waits for storage"), Stock.Advance(*Recipe, 0, 20));
    TestFalse(TEXT("Waits for batch timer"), Stock.Advance(*Recipe, Facility->CargoCapacity.Tonnes, 1));
    TestEqual(TEXT("Inputs reserved once"), Stock.Ore, 0.0);
    TestTrue(TEXT("Batch completes"), Stock.Advance(*Recipe, Facility->CargoCapacity.Tonnes, Recipe->SecondsPerBatch));
    TestEqual(TEXT("Product credited"), Stock.Product, Recipe->OutputTonnes);
    TestFalse(TEXT("Completion is not repeated"), Stock.Advance(*Recipe, Facility->CargoCapacity.Tonnes, 20));
    return true;
}
#endif
