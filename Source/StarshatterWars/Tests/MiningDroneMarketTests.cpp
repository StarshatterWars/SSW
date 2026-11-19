#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "CargoCatalogSubsystem.h"
#include "EconomyRuntime.h"
#include "GameStructs.h"
#include "MiningDroneTypes.h"
#include "Engine/GameInstance.h"
#include "JsonObjectConverter.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMiningDroneMarketMigrationTest, "Starshatter.Economy.MiningDroneMarketMigration",
    EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::EngineFilter)
bool FMiningDroneMarketMigrationTest::RunTest(const FString&)
{
    auto* Context = NewObject<UGameInstance>();
    auto* Catalog = NewObject<UCargoCatalogSubsystem>(Context); Catalog->LoadCatalog();
    const auto* Drone = Catalog->FindCargo(TEXT("mining drone"), TEXT("units"));
    if (!TestNotNull(TEXT("New drone is a native cargo catalog item"), Drone)) return false;
    const FName Item = DataTableUtils::MakeValidName(TEXT("mining drone"));
    const FName OldItem = DataTableUtils::MakeValidName(TEXT("avionics"));
    const FName Location = FEconomyRuntime::Warehouse(TEXT("Borova"), TEXT("NPC Market"), TEXT("Stock"));
    const FName OtherLocation = FEconomyRuntime::Warehouse(TEXT("Athenar"), TEXT("NPC Market"), TEXT("Stock"));
    const FName BaseMarker(TEXT("Authored initial market stock v1"));
    const FName Migration(TEXT("Authored initial mining drone stock v1"));
    FEconomyState Existing; Existing.InitializedMarkets.Add(BaseMarker);
    FEconomyRuntime::EnsureWarehouse(Existing, Location, TEXT("Borova"), 1.e12, true);
    FEconomyLedger::Deposit(Existing, Location, OldItem, 4, .2);
    FEconomyRuntime::InitializeMarkets(Existing, *Catalog);
    const double InitialDroneStock = FEconomyLedger::Quantity(Existing, Location, Item);
    TestTrue(TEXT("Existing pilots receive newly authored drone stock"), InitialDroneStock > 0);
    TestTrue(TEXT("Completed migration is recorded separately from old stock"), Existing.InitializedMarkets.Contains(Migration));
    TestEqual(TEXT("Previously consumed old goods are never restocked"), FEconomyLedger::Quantity(Existing, Location, OldItem), 4.0);
    FEconomyLedger::Withdraw(Existing, FEconomyRuntime::IndustryOwner(), Location, Item, 1);
    FEconomyRuntime::InitializeMarkets(Existing, *Catalog);
    TestEqual(TEXT("Reopening markets does not replenish a purchased drone"), FEconomyLedger::Quantity(Existing, Location, Item), InitialDroneStock - 1);

    FEconomyState Fresh; FEconomyRuntime::InitializeMarkets(Fresh, *Catalog);
    TestEqual(TEXT("New pilots seed drones once rather than twice"), FEconomyLedger::Quantity(Fresh, Location, Item), InitialDroneStock);
    TestTrue(TEXT("New pilots retain normal initial goods"), FEconomyLedger::Quantity(Fresh, Location, OldItem) > 0);

    FEconomyState Retry; Retry.InitializedMarkets.Add(BaseMarker);
    FEconomyRuntime::EnsureWarehouse(Retry, Location, TEXT("Borova"), 0, true);
    FEconomyRuntime::InitializeMarkets(Retry, *Catalog);
    TestFalse(TEXT("Failed new-item deposit cannot finish migration"), Retry.InitializedMarkets.Contains(Migration));
    TestEqual(TEXT("Insufficient destination capacity creates no drones"), FEconomyLedger::Quantity(Retry, Location, Item), 0.0);
    const double OtherStock = FEconomyLedger::Quantity(Retry, OtherLocation, Item);
    TestTrue(TEXT("Other producers can complete their own deposit"), OtherStock > 0);
    Retry.Inventories.FindChecked(Location).CapacityTonnes = 100;
    FEconomyRuntime::InitializeMarkets(Retry, *Catalog);
    TestTrue(TEXT("Failed location can be retried after storage recovers"), Retry.InitializedMarkets.Contains(Migration));
    TestEqual(TEXT("Retry deposits the missing location only"), FEconomyLedger::Quantity(Retry, Location, Item), InitialDroneStock);
    TestEqual(TEXT("Successful locations are not duplicated during retry"), FEconomyLedger::Quantity(Retry, OtherLocation, Item), OtherStock);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMiningDroneCatalogCapacityTest, "Starshatter.Mining.Drones.AuthoredBayAndProductionData",
    EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::EngineFilter)
bool FMiningDroneCatalogCapacityTest::RunTest(const FString&)
{
    const FString Directory = FPaths::ProjectContentDir() / TEXT("GameData");
    FString Json; TArray<TSharedPtr<FJsonValue>> Rows;
    if (!TestTrue(TEXT("Drone bay JSON reads"), FFileHelper::LoadFileToString(Json, *(Directory / TEXT("Cargo/mining_drones.json")))) ||
        !TestTrue(TEXT("Drone bay JSON parses"), FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(Json), Rows))) return false;
    TSet<FName> Hulls;
    for (const auto& Row : Rows)
    {
        FMiningDroneBayDefinition Bay;
        if (!TestTrue(TEXT("Native bay definition parses"), FJsonObjectConverter::JsonObjectToUStruct(Row->AsObject().ToSharedRef(), &Bay))) return false;
        Hulls.Add(Bay.ShipDesign);
        const int32 Count = Bay.ShipDesign == TEXT("Prospector") ? 3 : 10;
        TestEqual(TEXT("Mining hull has the requested drone count"), Bay.BayCount, Count);
        TestTrue(TEXT("Fitted control bandwidth supports the requested deployment"), Bay.DroneBandwidth > 0 && Bay.Bandwidth / Bay.DroneBandwidth >= Count);
        TestTrue(TEXT("Throughput calibration stays within native bounds"), Bay.MiningRateScale > 0 && Bay.MiningRateScale <= 1);
    }
    TestTrue(TEXT("Only explicitly fitted mining hulls receive bays"), Hulls.Num() == 2 && Hulls.Contains(TEXT("Prospector")) && Hulls.Contains(TEXT("Rockhopper")));

    TArray<TSharedPtr<FJsonValue>> Sites, Markets;
    if (!FFileHelper::LoadFileToString(Json, *(Directory / TEXT("Production/facilities.json"))) ||
        !FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(Json), Sites)) return false;
    if (!FFileHelper::LoadFileToString(Json, *(Directory / TEXT("Cargo/economy.json"))) ||
        !FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(Json), Markets)) return false;
    TSet<FName> FactorySystems, MarketSystems;
    for (const auto& Value : Sites)
    {
        const auto Site = Value->AsObject(); const TArray<TSharedPtr<FJsonValue>>* Recipes = nullptr;
        if (!Site->TryGetArrayField(TEXT("AdditionalRecipes"), Recipes)) continue;
        for (const auto& Recipe : *Recipes) if (Recipe->AsString() == TEXT("Make_mining_drone"))
            FactorySystems.Add(FName(*Site->GetStringField(TEXT("System"))));
    }
    for (const auto& Value : Markets)
    {
        const auto Market = Value->AsObject();
        for (const auto& Product : Market->GetArrayField(TEXT("Production")))
            if (Product->AsString() == TEXT("mining drone")) MarketSystems.Add(FName(*Market->GetStringField(TEXT("Name"))));
    }
    TestEqual(TEXT("Market and factory producer counts agree"), MarketSystems.Num(), FactorySystems.Num());
    for (FName System : FactorySystems) TestTrue(TEXT("Each drone-capable factory system advertises its production"), MarketSystems.Contains(System));
    return true;
}
#endif
