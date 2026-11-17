#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "GameStructs_products.h"
#include "GameStructs.h"
#include "GameStructs_System.h"
#include "ProductionActor.h"
#include "ProductionAI.h"
#include "ContractAI.h"
#include "Asteroid.h"
#include "MiningAI.h"
#include "Ship.h"
#include "ShipDesign.h"
#include "SimRegion.h"
#include "TransportAI.h"
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
    const ManufacturingAI::Job Job{FName(TEXT("Refinery")), FName(TEXT("Ore")), FName(TEXT("Product"))};
    FSSWProductionStock Stock;
    TestFalse(TEXT("Waits for ore"), ManufacturingAI::Advance(Job, *Recipe, Stock, Facility->CargoCapacity.Tonnes, 20));
    Stock.GetInventory(Job.InputLocation) = Recipe->InputTonnes;
    TestFalse(TEXT("Waits for storage"), ManufacturingAI::Advance(Job, *Recipe, Stock, 0, 20));
    TestFalse(TEXT("Waits for batch timer"), ManufacturingAI::Advance(Job, *Recipe, Stock, Facility->CargoCapacity.Tonnes, 1));
    TestEqual(TEXT("Inputs reserved once"), Stock.GetTonnes(Job.InputLocation), 0.0);
    TestEqual(TEXT("Output is unavailable during production"), Stock.GetTonnes(Job.OutputLocation), 0.0);
    TestEqual(TEXT("Batch reserves output space"), Stock.ReservedOutput, Recipe->OutputTonnes);
    auto ChangedJob = Job;
    ChangedJob.OutputLocation = FName(TEXT("Other Storage"));
    TestTrue(TEXT("Batch completes"), ManufacturingAI::Advance(ChangedJob, *Recipe, Stock,
        Facility->CargoCapacity.Tonnes, Recipe->SecondsPerBatch));
    TestEqual(TEXT("Running batch keeps its original output inventory"), Stock.GetTonnes(ChangedJob.OutputLocation), 0.0);
    TestEqual(TEXT("Product credited"), Stock.GetTonnes(Job.OutputLocation), Recipe->OutputTonnes);
    TestFalse(TEXT("Completion is not repeated"), ManufacturingAI::Advance(Job, *Recipe, Stock, Facility->CargoCapacity.Tonnes, 20));
    ManufacturingAI::Job EfficientJob = Job;
    EfficientJob.MaterialMultiplier = 0.8;
    FSSWProductionStock EfficientStock;
    EfficientStock.GetInventory(Job.InputLocation) = Recipe->InputTonnes * 0.8;
    TestFalse(TEXT("Offline facility leaves inputs available"),
        ProductionAI::AdvanceFacility(false, EfficientJob, *Recipe, EfficientStock, Facility->CargoCapacity.Tonnes, Recipe->SecondsPerBatch));
    TestEqual(TEXT("Offline facility preserves ore"), EfficientStock.GetTonnes(Job.InputLocation), Recipe->InputTonnes * 0.8);
    TestTrue(TEXT("Material efficiency allows batch with fewer inputs"),
        ProductionAI::AdvanceFacility(true, EfficientJob, *Recipe, EfficientStock, Facility->CargoCapacity.Tonnes, Recipe->SecondsPerBatch));
    TestEqual(TEXT("Material efficiency preserves output"), EfficientStock.GetTonnes(Job.OutputLocation), Recipe->OutputTonnes);

    ContractAI First;
    ContractAI Second;
    double Available = 4.0;
    TestTrue(TEXT("First contract reserves available stock"), First.Reserve(TEXT("ore"), Available, 4.0));
    TestFalse(TEXT("Second contract cannot book reserved stock"), Second.Reserve(TEXT("ore"), Available, 1.0));
    const FGuid FirstContractId = First.GetId();
    TestTrue(TEXT("A reservation assigns a stable contract ID"), FirstContractId.IsValid());
    First.CommitPickup(1.0);
    TestEqual(TEXT("Partial pickup preserves the contract ID"), First.GetId(), FirstContractId);
    First.Release(Available);
    TestFalse(TEXT("Release clears the contract ID"), First.GetId().IsValid());
    TestEqual(TEXT("Cancellation restores only the portion still at pickup"), Available, 3.0);
    // Run real inventory transfers through the same handlers as T1.
    SimRegion Sector(nullptr, "Janek", 0);
    auto* Rock = new Asteroid(1, FVector(0, 0, 3000), 10000000);
    auto* DistantRock = new Asteroid(2, FVector(0, 0, 26000), 10000000);
    Sector.InsertObject(Rock);
    Sector.InsertObject(DistantRock);
    const TArray<ESSWMineral> Minerals{Rock->GetResource().Mineral};
    const auto Survey = MiningAI::ScanAsteroids(Sector, FVector::ZeroVector, 25000, MiningAI::ScanTier::Survey);
    TestEqual(TEXT("Survey omits the distant contact"), Survey.Readings.Num(), 1);
    if (Survey.Readings.Num() != 1) return false;
    TestEqual(TEXT("Survey gives contact distance"), Survey.Readings[0].Distance, 3000.0);
    TestFalse(TEXT("Survey leaves tonnes unknown"), Survey.Readings[0].RemainingTonnes.IsSet());
    TestNull(TEXT("A survey alone cannot assign a deposit"),
        MiningAI::SelectDeposit(Sector, FVector::ZeroVector, Survey, Minerals, FString(), {}));
    const auto Assay = MiningAI::ScanAsteroids(Sector, FVector::ZeroVector, 25000, MiningAI::ScanTier::Assay);
    if (!TestTrue(TEXT("Assay selects the in-range rock"),
        MiningAI::SelectDeposit(Sector, FVector::ZeroVector, Assay, Minerals, FString(), {}) == Rock))
        return false;

    ShipDesign CourierDesign;
    CourierDesign.CargoCapacity.Tonnes = 0.3;
    Ship Courier("Courier test", "", &CourierDesign, 0, 0, nullptr, false);
    const FName PackageId(TEXT("TestCourier"));
    TestTrue(TEXT("Sealed package fits on an empty fighter"), Courier.LoadCourierPackage(PackageId, 0.1));
    TestFalse(TEXT("The package cannot be loaded twice"), Courier.LoadCourierPackage(PackageId, 0.1));
    TestTrue(TEXT("Other cargo uses only remaining capacity"),
        FMath::IsNearlyEqual(Courier.LoadMinedOre(TEXT("ore"), 1.0), 0.2));
    TestTrue(TEXT("Used capacity includes ordinary cargo and the package"),
        FMath::IsNearlyEqual(Courier.GetCargoUsedTonnes(), 0.3));
    Courier.RemoveCourierPackage(TEXT("WrongContract"));
    TestTrue(TEXT("Only the matching contract can remove the package"), Courier.HasCourierPackage(PackageId));
    Courier.RemoveCourierPackage(PackageId);
    TestFalse(TEXT("Delivery removed the sealed package"), Courier.HasCourierPackage(PackageId));
    TestTrue(TEXT("Delivery restores capacity and preserves ordinary cargo"),
        FMath::IsNearlyEqual(Courier.GetCargoUsedTonnes(), 0.2));
    TestFalse(TEXT("A package exceeding remaining capacity is refused"), Courier.LoadCourierPackage(PackageId, 0.15));

    ShipDesign HaulerDesign;
    HaulerDesign.CargoCapacity.Tonnes = FMath::Max(Recipe->InputTonnes, Recipe->OutputTonnes) + 1.0;
    Ship OreHauler("Ore hauler test", "", &HaulerDesign, 0, 0, nullptr, false);
    Ship GoodsHauler("Goods hauler test", "", &HaulerDesign, 0, 0, nullptr, false);
    double Depot = Rock->ExtractOre(Recipe->InputTonnes);
    if (!TestEqual(TEXT("Mining puts enough ore in the depot"), Depot, Recipe->InputTonnes)) return false;
    ContractAI OreContract;
    TestTrue(TEXT("Freighter reserves depot ore"), OreContract.Reserve(TEXT("ore"), Depot, Recipe->InputTonnes));
    TestEqual(TEXT("Reservation removes depot stock from availability"), Depot, 0.0);
    TransportAI::Load(OreHauler, OreContract, Recipe->InputTonnes);
    FSSWProductionStock DeliveredStock;
    TestEqual(TEXT("Refinery receives nothing before unloading"), DeliveredStock.GetTonnes(Job.InputLocation), 0.0);
    TransportAI::Unload(OreHauler, DeliveredStock.GetInventory(Job.InputLocation),
        HaulerDesign.CargoCapacity.Tonnes, Recipe->InputTonnes);
    TestTrue(TEXT("Delivered ore produces finished goods"),
        ProductionAI::AdvanceFacility(true, Job, *Recipe, DeliveredStock, Facility->CargoCapacity.Tonnes, Recipe->SecondsPerBatch));
    ContractAI GoodsContract;
    TestTrue(TEXT("Second freighter reserves the finished goods"),
        GoodsContract.Reserve(TEXT("finished goods"), DeliveredStock.GetInventory(Job.OutputLocation), Recipe->OutputTonnes));
    TransportAI::Load(GoodsHauler, GoodsContract, Recipe->OutputTonnes);
    double TransferStock = 0.0;
    TestEqual(TEXT("Loading goods does not credit transfer stock"), TransferStock, 0.0);
    TransportAI::Unload(GoodsHauler, TransferStock, HaulerDesign.CargoCapacity.Tonnes, Recipe->OutputTonnes);
    TestEqual(TEXT("Transfer receives manufactured output after unloading"), TransferStock, Recipe->OutputTonnes);
    TestEqual(TEXT("Unload retries cannot duplicate output"),
        TransportAI::Unload(GoodsHauler, TransferStock, HaulerDesign.CargoCapacity.Tonnes, Recipe->OutputTonnes), 0.0);
    Rock->ExtractOre(Rock->GetResource().AmountTonnes);
    const auto Partial = MiningAI::ScanAsteroids(Sector, FVector::ZeroVector, 25000, MiningAI::ScanTier::Assay);
    TestTrue(TEXT("Unscanned rocks keep belt status unknown"), Partial.Status == MiningAI::BeltStatus::Unknown);
    DistantRock->ExtractOre(DistantRock->GetResource().AmountTonnes);
    const auto Depleted = MiningAI::ScanAsteroids(Sector, FVector::ZeroVector, 30000, MiningAI::ScanTier::Assay);
    TestTrue(TEXT("Complete assay confirms depletion"), Depleted.Status == MiningAI::BeltStatus::Depleted);
    return true;
}
#endif


