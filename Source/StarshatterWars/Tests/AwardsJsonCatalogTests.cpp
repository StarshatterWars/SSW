#if WITH_DEV_AUTOMATION_TESTS && WITH_EDITOR
#include "Misc/AutomationTest.h"
#include "AwardsJsonCatalog.h"
#include "StarshatterGameDataSubsystem.h"
#include "Engine/GameInstance.h"
#include "Engine/DataTable.h"
#include "GameStructs.h"
#include "HAL/FileManager.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FAwardsJsonParityTest, "Starshatter.Awards.JsonLegacyParity",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FAwardsJsonParityTest::RunTest(const FString&)
{
    auto* Context = NewObject<UGameInstance>();
    auto* Data = NewObject<UStarshatterGameDataSubsystem>(Context);
    Data->RanksDataTable = NewObject<UDataTable>(); Data->RanksDataTable->RowStruct = FRankInfo::StaticStruct();
    Data->MedalsDataTable = NewObject<UDataTable>(); Data->MedalsDataTable->RowStruct = FMedalInfo::StaticStruct();
    Data->LoadAwardTables(false);
    if (!TestTrue(TEXT("Legacy ranks loaded"), Data->RanksDataTable->GetRowMap().Num() > 0) ||
        !TestTrue(TEXT("Legacy medals loaded"), Data->MedalsDataTable->GetRowMap().Num() > 0)) return false;
    auto* Ranks = NewObject<UDataTable>(); Ranks->RowStruct = FRankInfo::StaticStruct();
    auto* Medals = NewObject<UDataTable>(); Medals->RowStruct = FMedalInfo::StaticStruct();
    FString Error;
    const FString Path = FPaths::ProjectContentDir() / TEXT("GameData/Awards/awards.json");
    if (!TestTrue(TEXT("Native awards JSON imports"), AwardsJsonCatalog::Import(Path, *Ranks, *Medals, Error)))
    { AddError(Error); return false; }
    TestEqual(TEXT("Every rank retained"), Ranks->GetRowMap().Num(), Data->RanksDataTable->GetRowMap().Num());
    TestEqual(TEXT("Every medal retained"), Medals->GetRowMap().Num(), Data->MedalsDataTable->GetRowMap().Num());
    for (const auto& Pair : Data->RanksDataTable->GetRowMap())
    {
        const auto* Row = Ranks->FindRow<FRankInfo>(Pair.Key, TEXT("Awards parity"), false);
        TestTrue(*FString::Printf(TEXT("Rank %s: thresholds, grants, text, sound and image references unchanged"), *Pair.Key.ToString()),
            Row && FRankInfo::StaticStruct()->CompareScriptStruct(Pair.Value, Row, 0));
    }
    for (const auto& Pair : Data->MedalsDataTable->GetRowMap())
    {
        const auto* Row = Medals->FindRow<FMedalInfo>(Pair.Key, TEXT("Awards parity"), false);
        TestTrue(*FString::Printf(TEXT("Medal %s: flag, eligibility, ceremony and presentation unchanged"), *Pair.Key.ToString()),
            Row && FMedalInfo::StaticStruct()->CompareScriptStruct(Pair.Value, Row, 0));
    }
    // Default runtime path must consume the same JSON into isolated instances.
    auto* Runtime = NewObject<UStarshatterGameDataSubsystem>(Context);
    Runtime->RanksDataTable = Ranks; Runtime->MedalsDataTable = Medals;
    Runtime->LoadAwardTables();
    TestTrue(TEXT("Runtime creates isolated tables"), Runtime->RanksDataTable != Ranks && Runtime->MedalsDataTable != Medals);
    auto* FirstRanks = Runtime->RanksDataTable;
    Runtime->LoadAwardTables();
    TestTrue(TEXT("Load-once guard leaves live rank table stable"), Runtime->RanksDataTable == FirstRanks);
    AddInfo(FString::Printf(TEXT("Awards JSON preserves %d ranks and %d medals, full native row parity"),
        Ranks->GetRowMap().Num(), Medals->GetRowMap().Num()));
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FAwardsJsonAtomicTest, "Starshatter.Awards.JsonAtomicReloadAndValidation",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FAwardsJsonAtomicTest::RunTest(const FString&)
{
    const FString Folder = FPaths::ProjectSavedDir() / TEXT("Automation/AwardsJson") / FGuid::NewGuid().ToString(EGuidFormats::Digits);
    IFileManager::Get().MakeDirectory(*Folder, true);
    const FString Path = Folder / TEXT("awards.json");
    auto* SourceRanks = NewObject<UDataTable>(); SourceRanks->RowStruct = FRankInfo::StaticStruct();
    auto* SourceMedals = NewObject<UDataTable>(); SourceMedals->RowStruct = FMedalInfo::StaticStruct();
    FRankInfo Rank; Rank.Name = TEXT("Ensign"); Rank.RankId = 1; Rank.TotalPoints = 500; Rank.GrantedShipClasses = 7;
    FMedalInfo Medal; Medal.Name = TEXT("Test Medal"); Medal.MedalId = 0x80; Medal.Lost = -1; Medal.Lottery = 10;
    Medal.RequiredAwards = 0x20; Medal.bEligibilityConfigured = true;
    SourceRanks->AddRow(FName(*Rank.Name), Rank); SourceMedals->AddRow(FName(*Medal.Name), Medal);
    FString Error;
    if (!TestTrue(TEXT("Full row export"), AwardsJsonCatalog::Export(Path, *SourceRanks, *SourceMedals, Error))) return false;
    auto* Ranks = NewObject<UDataTable>(); Ranks->RowStruct = FRankInfo::StaticStruct();
    auto* Medals = NewObject<UDataTable>(); Medals->RowStruct = FMedalInfo::StaticStruct();
    Ranks->AddRow(TEXT("Stale Rank"), Rank); Medals->AddRow(TEXT("Stale Medal"), Medal);
    TestTrue(TEXT("Valid catalog replaces stale entries"), AwardsJsonCatalog::Import(Path, *Ranks, *Medals, Error));
    TestEqual(TEXT("No stale ranks"), Ranks->GetRowMap().Num(), 1);
    TestEqual(TEXT("No stale medals"), Medals->GetRowMap().Num(), 1);
    TestTrue(TEXT("Repeated import is idempotent"), AwardsJsonCatalog::Import(Path, *Ranks, *Medals, Error));
    FString Json; FFileHelper::LoadFileToString(Json, *Path);
    TSharedPtr<FJsonObject> Object;
    if (!FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(Json), Object) || !Object) return false;
    auto Duplicate = Object->GetArrayField(TEXT("medals")); const auto FirstMedal = Duplicate[0]; Duplicate.Add(FirstMedal);
    Object->SetArrayField(TEXT("medals"), Duplicate);
    FString Invalid; FJsonSerializer::Serialize(Object.ToSharedRef(), TJsonWriterFactory<>::Create(&Invalid));
    FFileHelper::SaveStringToFile(Invalid, *Path);
    TestFalse(TEXT("Duplicate medal flag rejects entire transaction"), AwardsJsonCatalog::Import(Path, *Ranks, *Medals, Error));
    const auto* KeptRank = Ranks->FindRow<FRankInfo>(TEXT("Ensign"), TEXT("Failed import"), false);
    const auto* KeptMedal = Medals->FindRow<FMedalInfo>(TEXT("Test Medal"), TEXT("Failed import"), false);
    TestTrue(TEXT("Invalid second table leaves first table unchanged"), KeptRank && FRankInfo::StaticStruct()->CompareScriptStruct(&Rank, KeptRank, 0));
    TestTrue(TEXT("Signed loss and lottery requirements remain unchanged"), KeptMedal && FMedalInfo::StaticStruct()->CompareScriptStruct(&Medal, KeptMedal, 0));
    Object->SetArrayField(TEXT("medals"), {Duplicate[0]});
    Object->GetArrayField(TEXT("ranks"))[0]->AsObject()->RemoveField(TEXT("rankId"));
    Invalid.Reset(); FJsonSerializer::Serialize(Object.ToSharedRef(), TJsonWriterFactory<>::Create(&Invalid));
    FFileHelper::SaveStringToFile(Invalid, *Path);
    AddExpectedError(TEXT("JsonObjectToUStruct - Missing JSON value named RankId"), EAutomationExpectedErrorFlags::Contains, 1);
    TestFalse(TEXT("Missing authored ID cannot silently become Cadet"), AwardsJsonCatalog::Import(Path, *Ranks, *Medals, Error));
    TestEqual(TEXT("Incomplete catalog preserves both tables"), Ranks->GetRowMap().Num() + Medals->GetRowMap().Num(), 2);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FAwardsDirectJsonTest, "Starshatter.Awards.DirectJsonWithoutTables",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FAwardsDirectJsonTest::RunTest(const FString&)
{
    auto* Context = NewObject<UGameInstance>();
    auto* Data = NewObject<UStarshatterGameDataSubsystem>(Context);
    Data->LoadAwardsJson();
    TestEqual(TEXT("All authored ranks load without table assets"), Data->RankDefinitions.Num(), 11);
    TestEqual(TEXT("All authored medals load without table assets"), Data->MedalDefinitions.Num(), 16);
    for (const auto& Rank : Data->RankDefinitions)
    {
        FRankInfo Found;
        TestTrue(TEXT("Rank lookup populated directly"), Data->GetRankInfo(Rank.RankId, Found));
        TestTrue(TEXT("Every native rank field retained"), FRankInfo::StaticStruct()->CompareScriptStruct(&Rank, &Found, 0));
    }
    for (const auto& Medal : Data->MedalDefinitions)
    {
        FMedalInfo Found;
        TestTrue(TEXT("Medal flag lookup populated directly"), Data->GetMedalInfoByFlag(Medal.MedalId, Found));
        TestTrue(TEXT("Every native medal field retained"), FMedalInfo::StaticStruct()->CompareScriptStruct(&Medal, &Found, 0));
    }
    Data->LoadAwardsJson();
    TestEqual(TEXT("Repeated boot does not duplicate ranks"), Data->RankDefinitions.Num(), 11);
    // A missing replacement must preserve both validated arrays.
    FString Error;
    TestFalse(TEXT("Missing JSON rejected atomically"), AwardsJsonCatalog::Load(
        FPaths::ProjectSavedDir() / TEXT("MissingAwardsCatalog.json"), Data->RankDefinitions, Data->MedalDefinitions, Error));
    TestEqual(TEXT("Failed reload retains ranks"), Data->RankDefinitions.Num(), 11);
    TestEqual(TEXT("Failed reload retains medals"), Data->MedalDefinitions.Num(), 16);
    return true;
}
#endif
