#if WITH_DEV_AUTOMATION_TESTS && WITH_EDITOR
#include "Misc/AutomationTest.h"
#include "CargoCatalogSubsystem.h"
#include "AgentCatalogSubsystem.h"
#include "JsonRecordCatalog.h"
#include "Misc/Paths.h"
#include "Misc/FileHelper.h"
#include "Engine/GameInstance.h"
#include "UObject/UnrealType.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDirectCatalogParityTest, "Starshatter.Catalogs.DirectJsonParity",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FDirectCatalogParityTest::RunTest(const FString&)
{
    auto* Context = NewObject<UGameInstance>();
    auto* Cargo = NewObject<UCargoCatalogSubsystem>(Context);
    Cargo->LoadCatalog();
    auto* Legacy = NewObject<UDataTable>(Context);
    if (!TestTrue(TEXT("Reference cargo adapter loads"), UCargoCatalogSubsystem::ImportDefinitions(*Legacy))) return false;
    int32 Count = 0;
    Cargo->ForEachDefinition([&](FName Key, const FS_CargoCatalogRow& Row)
    {
        ++Count;
        const auto* Expected = Legacy->FindRow<FS_CargoCatalogRow>(Key, TEXT("Parity"), false);
        TestNotNull(TEXT("Original cargo inventory identifier retained"), Expected);
        if (Expected)
        {
            for (TFieldIterator<FProperty> It(FS_CargoCatalogRow::StaticStruct()); It; ++It)
            {
                const auto* Property = *It;
                if (!Property->Identical_InContainer(&Row, Expected))
                {
                    FString ActualText, ExpectedText;
                    Property->ExportText_InContainer(0, ActualText, &Row, nullptr, nullptr, PPF_None);
                    Property->ExportText_InContainer(0, ExpectedText, Expected, nullptr, nullptr, PPF_None);
                    AddError(FString::Printf(TEXT("%s.%s direct=%s table=%s"), *Key.ToString(), *Property->GetName(), *ActualText, *ExpectedText));
                    break;
                }
            }
        }

        TestNotNull(TEXT("Lookup preserves item and unit"), Cargo->FindCargo(Key.ToString(), Row.Unit));
    });
    TestEqual(TEXT("All cargo records retained"), Count, Legacy->GetRowMap().Num());
    TestTrue(TEXT("Expanded cargo catalog is populated"), Count >= 250);
    auto* Agents = NewObject<UAgentCatalogSubsystem>(Context);
    Agents->LoadCatalog();
    FString Text;
    FFileHelper::LoadFileToString(Text, *(FPaths::ProjectContentDir() / TEXT("GameData/Agents/agents.json")));
    Legacy->EmptyTable(); Legacy->RowStruct = FS_AgentCatalogRow::StaticStruct();
    TestEqual(TEXT("Reference agent JSON loads"), Legacy->CreateTableFromJSONString(Text).Num(), 0);
    TestEqual(TEXT("All agents retained"), Agents->GetCatalog().Num(), Legacy->GetRowMap().Num());
    for (const auto& Pair : Agents->GetCatalog())
    {
        const auto* Expected = Legacy->FindRow<FS_AgentCatalogRow>(Pair.Key, TEXT("Parity"), false);
        TestTrue(TEXT("Direct agents preserve every field and ID"), Expected &&
            FS_AgentCatalogRow::StaticStruct()->CompareScriptStruct(&Pair.Value, Expected, 0));
    }
    TMap<FName, FS_CargoCatalogRow> Existing;
    FS_CargoCatalogRow Kept; Existing.Add(TEXT("Preserved"), Kept);
    const FString Path = FPaths::ProjectSavedDir() / TEXT("DirectCatalogInvalid.json");
    FFileHelper::SaveStringToFile(TEXT("[{\"Name\":\"duplicate\"},{\"Name\":\"DUPLICATE\"}]"), *Path);
    FString Error;
    TestFalse(TEXT("Duplicate identifiers rejected"), SSWJsonRecords::Load(Path, Existing, Error));
    TestTrue(TEXT("Failed replacement preserves working records"), Existing.Num() == 1 && Existing.Contains(TEXT("Preserved")));
    return true;
}
#endif
