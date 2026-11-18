#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "SSWModFiles.h"
#include "SSWMissionMods.h"
#include "Misc/Paths.h"
#include "Misc/FileHelper.h"
#include "Misc/ConfigCacheIni.h"
#include "Misc/ScopeExit.h"
#include "HAL/FileManager.h"
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMissionModFormatPriorityTest,"Starshatter.Missions.JsonModPriority",
    EAutomationTestFlags_ApplicationContextMask|EAutomationTestFlags::EngineFilter)
bool FMissionModFormatPriorityTest::RunTest(const FString&)
{
    if (!GConfig) return false;
    const FString Token=FGuid::NewGuid().ToString(EGuidFormats::Digits);
    const FString Lower=TEXT("ZZ_A_")+Token, Higher=TEXT("ZZ_Z_")+Token;
    const FString LowFolder=SSWMissionMods::Directory()/Lower, HighFolder=SSWMissionMods::Directory()/Higher;
    const FString Leaf=TEXT("Missions/")+Token;
    ON_SCOPE_EXIT
    {
        GConfig->RemoveKey(TEXT("Starshatter.MissionMods"),*Lower,GGameUserSettingsIni);
        GConfig->RemoveKey(TEXT("Starshatter.MissionMods"),*Higher,GGameUserSettingsIni);
        IFileManager::Get().DeleteDirectory(*LowFolder,false,true);
        IFileManager::Get().DeleteDirectory(*HighFolder,false,true);
    };
    IFileManager::Get().MakeDirectory(*(LowFolder/TEXT("Missions")),true);
    IFileManager::Get().MakeDirectory(*(HighFolder/TEXT("Missions")),true);
    FFileHelper::SaveStringToFile(TEXT("LOW JSON"),*(LowFolder/(Leaf+TEXT(".json"))),FFileHelper::EEncodingOptions::ForceUTF8WithoutBOM);
    FFileHelper::SaveStringToFile(TEXT("HIGH DEF"),*(HighFolder/(Leaf+TEXT(".def"))),FFileHelper::EEncodingOptions::ForceUTF8WithoutBOM);
    GConfig->SetBool(TEXT("Starshatter.MissionMods"),*Lower,true,GGameUserSettingsIni);
    GConfig->SetBool(TEXT("Starshatter.MissionMods"),*Higher,true,GGameUserSettingsIni);
    TArray<uint8> Bytes; bool bJson=false;
    const FString Authored=FPaths::ProjectContentDir()/TEXT("GameData")/(Leaf+TEXT(".def"));
    TestTrue(TEXT("Enabled override resolves"),SSWModFiles::ReadMission(Authored,Bytes,bJson));
    TestFalse(TEXT("Higher-priority DEF beats lower-priority JSON"),bJson);
    Bytes.Add(0);TestEqual(TEXT("Highest priority pack wins"),FString(UTF8_TO_TCHAR(reinterpret_cast<const char*>(Bytes.GetData()))),FString(TEXT("HIGH DEF")));
    FFileHelper::SaveStringToFile(TEXT("HIGH JSON"),*(HighFolder/(Leaf+TEXT(".json"))),FFileHelper::EEncodingOptions::ForceUTF8WithoutBOM);
    Bytes.Reset();TestTrue(TEXT("JSON override resolves"),SSWModFiles::ReadMission(Authored,Bytes,bJson));
    TestTrue(TEXT("Within same pack JSON replaces DEF"),bJson);
    Bytes.Add(0);TestEqual(TEXT("JSON body selected"),FString(UTF8_TO_TCHAR(reinterpret_cast<const char*>(Bytes.GetData()))),FString(TEXT("HIGH JSON")));
    return true;
}
#endif
