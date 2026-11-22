#if WITH_DEV_AUTOMATION_TESTS && WITH_EDITOR
#include "Misc/AutomationTest.h"
#include "GalaxyJsonCatalog.h"
#include "StarshatterEnvironmentSubsystem.h"
#include "SSWGameInstance.h"
#include "Engine/DataTable.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "HAL/FileManager.h"
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGalaxyJsonParityTest,"Starshatter.Data.Galaxy.JsonTableRoundTrip",
    EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FGalaxyJsonParityTest::RunTest(const FString&)
{
    auto* Context=NewObject<USSWGameInstance>();auto* Source=NewObject<UStarshatterEnvironmentSubsystem>(Context);
    Source->GetSSWInstance();Source->LoadGalaxyMap();
    TestEqual(TEXT("All 21 systems loaded from JSON"),Source->GalaxyDataArray.Num(),21);
    TArray<FRegion> Regions;
    if(!TestNotNull(TEXT("Parsed region index"),Source->GetRegionsTable()))return false;
    for(const FName Key:Source->GetRegionsTable()->GetRowNames())
        Regions.Add(*Source->GetRegionsTable()->FindRow<FRegion>(Key,TEXT("Galaxy parity")));
    TestEqual(TEXT("All 93 JSON regions populate the table"), Regions.Num(), 93);
    const FString Directory=FPaths::ProjectSavedDir()/TEXT("Automation/GalaxyJson");IFileManager::Get().MakeDirectory(*Directory,true);
    const FString Path=Directory/TEXT("galaxy.json");FString Error;
    if(!TestTrue(TEXT("JSON catalog round trips through compatibility tables"),GalaxyJsonCatalog::Save(Path,Source->GalaxyDataArray,Regions,Error)))
    {AddError(Error);return false;}
    TArray<FS_Galaxy> Systems;TArray<FRegion> LoadedRegions;
    if(!TestTrue(TEXT("Native JSON catalog loads"),GalaxyJsonCatalog::Load(Path,Systems,LoadedRegions,Error)))
    {AddError(Error);return false;}
    TestEqual(TEXT("Every system retained"),Systems.Num(),Source->GalaxyDataArray.Num());
    TestEqual(TEXT("Every region retained"),LoadedRegions.Num(),Regions.Num());
    for(int32 Index=0;Index<Systems.Num();++Index)
        TestTrue(TEXT("All nested stars, planets, moons, locations, orbits and production match"),
            FS_Galaxy::StaticStruct()->CompareScriptStruct(&Systems[Index],&Source->GalaxyDataArray[Index],0));
    for(int32 Index=0;Index<LoadedRegions.Num();++Index)
        TestTrue(TEXT("Every region property retained"),FRegion::StaticStruct()->CompareScriptStruct(&LoadedRegions[Index],&Regions[Index],0));
    const int32 Count=Systems.Num();
    TestFalse(TEXT("Unsupported schema rejected"),GalaxyJsonCatalog::Parse(TEXT("{\"schemaVersion\":999}"),Systems,LoadedRegions,Error));
    TestEqual(TEXT("Failed import preserves published catalog"),Systems.Num(),Count);
    return true;
}
#endif
