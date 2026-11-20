#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "StationServiceCatalog.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FStationServiceRolesTest,"Starshatter.StationServices.AuthoredRoleBoundaries",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FStationServiceRolesTest::RunTest(const FString&)
{
    FStationServiceCatalog Catalog;FString Json,Error;
    if(!TestTrue(TEXT("Authored role file readable"),FFileHelper::LoadFileToString(Json,*(FPaths::ProjectContentDir()/TEXT("GameData/Production/station_services.json")))))return false;
    if(!TestTrue(TEXT("Authored roles valid"),Catalog.LoadJson(Json,Error)))return false;
    TestFalse(TEXT("Carrier does not provide shipyard refit"),Catalog.HasService(TEXT("Orion"),TEXT("Archon"),ESSWStationService::Refit));
    TestFalse(TEXT("Carrier does not provide a commercial market"),Catalog.HasService(TEXT("Orion"),TEXT("Archon"),ESSWStationService::Market));
    TestTrue(TEXT("Carrier retains basic repair"),Catalog.HasService(TEXT("Orion"),TEXT("Archon"),ESSWStationService::Repair));
    TestTrue(TEXT("Orion offers reserve fighter purchases"),Catalog.HasService(TEXT("Orion"),TEXT("Archon"),ESSWStationService::HullSales));
    TestTrue(TEXT("Fortress drydock offers refit"),Catalog.HasService(TEXT("Station4"),NAME_None,ESSWStationService::Refit));
    TestFalse(TEXT("Mining station cannot refit"),Catalog.HasService(TEXT("Mining Station"),NAME_None,ESSWStationService::Refit));
    TestFalse(TEXT("Transfer point cannot refit"),Catalog.HasService(TEXT("Station1"),NAME_None,ESSWStationService::Refit));
    TestFalse(TEXT("Refinery cannot refit"),Catalog.HasService(TEXT("Processing Facility"),NAME_None,ESSWStationService::Refit));
    TestTrue(TEXT("Refinery offers industry"),Catalog.HasService(TEXT("Processing Facility"),NAME_None,ESSWStationService::Manufacturing));
    TestFalse(TEXT("Unknown cannot inherit sensitive service"),Catalog.HasService(TEXT("Unknown"),TEXT("Archon"),ESSWStationService::Refit));
    return true;
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FStationServiceOverrideTest,"Starshatter.StationServices.OverrideAndTransactionalValidation",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FStationServiceOverrideTest::RunTest(const FString&)
{
    FStationServiceCatalog Catalog;FString Error;
    const FString Json=TEXT(R"JSON({"schemaVersion":1,"profiles":[{"name":"yard","services":["Refit","Repair"]},{"name":"limited","services":["Cargo"]}],"stations":[{"design":"TestStation","profile":"yard"},{"design":"TestStation","facility":"Damaged Yard","profile":"limited"}]})JSON");
    if(!TestTrue(TEXT("Role fixture parses"),Catalog.LoadJson(Json,Error)))return false;
    TestTrue(TEXT("Design default provides refit"),Catalog.HasService(TEXT("TestStation"),TEXT("Other Yard"),ESSWStationService::Refit));
    TestFalse(TEXT("Named facility replaces rather than unions services"),Catalog.HasService(TEXT("TestStation"),TEXT("Damaged Yard"),ESSWStationService::Refit));
    TestTrue(TEXT("Named facility keeps explicit service"),Catalog.HasService(TEXT("TestStation"),TEXT("Damaged Yard"),ESSWStationService::Cargo));
    TestFalse(TEXT("Invalid service rejected"),Catalog.LoadJson(Json.Replace(TEXT("Repair"),TEXT("UnlimitedFreeShips")),Error));
    TestTrue(TEXT("Invalid reload cannot partially change roles"),Catalog.HasService(TEXT("TestStation"),TEXT("Other Yard"),ESSWStationService::Refit));
    TestFalse(TEXT("Unknown profile rejected"),Catalog.LoadJson(Json.Replace(TEXT("\"profile\":\"yard\""),TEXT("\"profile\":\"missing\"")),Error));
    TestFalse(TEXT("Wrong schema rejected"),Catalog.LoadJson(Json.Replace(TEXT("\"schemaVersion\":1"),TEXT("\"schemaVersion\":2")),Error));
    return true;
}
#endif
