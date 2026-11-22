#if WITH_DEV_AUTOMATION_TESTS && WITH_EDITOR
#include "Misc/AutomationTest.h"
#include "UObject/UnrealType.h"
#include "StarshatterGameDataSubsystem.h"
#include "SSWGameInstance.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "HAL/FileManager.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMissionJsonParityTest,"Starshatter.Missions.JsonDefinitionParity",
    EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FMissionJsonParityTest::RunTest(const FString&)
{
    auto* Context=NewObject<USSWGameInstance>();
    auto* Data=NewObject<UStarshatterGameDataSubsystem>(Context);
    TArray<FString> Files;
    IFileManager::Get().FindFilesRecursive(Files,*(FPaths::ProjectContentDir()/TEXT("GameData")),TEXT("*.json"),true,false);
    int32 Checked=0;
    for (const FString& Path:Files)
    {
        const FString Def=FPaths::ChangeExtension(Path,TEXT("def"));
        if (!FPaths::FileExists(Def)) continue;
        FString Header; FFileHelper::LoadFileToString(Header,*Def); Header.TrimStartInline();
        if (!Header.StartsWith(TEXT("MISSION")) || Header.StartsWith(TEXT("MISSION_TEMPLATE")) || Header.StartsWith(TEXT("MISSIONLIST"))) continue;
        FS_CampaignMission Legacy,Json;
        if (!TestTrue(*FString::Printf(TEXT("DEF parses: %s"),*Def),Data->ReadLooseMission(Def,Legacy,false))) continue;
        if (!TestTrue(*FString::Printf(TEXT("JSON parses: %s"),*Path),Data->ReadLooseMission(Path,Json))) continue;
        const bool Same = FS_CampaignMission::StaticStruct()->CompareScriptStruct(&Legacy, &Json, 0);
        if (!Same)
            for (TFieldIterator<FProperty> Property(FS_CampaignMission::StaticStruct()); Property; ++Property)
                if (!Property->Identical_InContainer(&Legacy, &Json))
                {
                    FString Left, Right;
                    Property->ExportText_InContainer(0, Left, &Legacy, nullptr, nullptr, PPF_None);
                    Property->ExportText_InContainer(0, Right, &Json, nullptr, nullptr, PPF_None);
                    AddError(FString::Printf(TEXT("%s property %s differs: DEF=%s JSON=%s"), *Path, *Property->GetName(), *Left, *Right));
                }
        TestTrue(*FString::Printf(TEXT("Every mission property survives JSON: %s"),*Path), Same);
        ++Checked;
    }
    TestTrue(TEXT("Authored playable mission JSON was tested"),Checked>=79);
    return true;
}
#endif
