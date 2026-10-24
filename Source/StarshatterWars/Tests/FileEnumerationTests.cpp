#if WITH_DEV_AUTOMATION_TESTS && WITH_EDITOR
#include "Misc/AutomationTest.h"
#include "DataLoader.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Misc/ScopeExit.h"
#include "HAL/FileManager.h"
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FFileEnumerationTest,"Starshatter.Audit.F22.FileEnumeration",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FFileEnumerationTest::RunTest(const FString& Parameters)
{
    const FString Root=FPaths::ConvertRelativePathToFull(FPaths::ProjectSavedDir()/TEXT("Automation/P2Music"));
    const FString A=Root/TEXT("Flight-loop.ogg"),B=Root/TEXT("Sub/Combat-loop.ogg"),C=Root/TEXT("ignore.txt");
    IFileManager::Get().MakeDirectory(*(Root/TEXT("Sub")),true);
    FFileHelper::SaveStringToFile(TEXT("fixture"),*A);FFileHelper::SaveStringToFile(TEXT("fixture"),*B);FFileHelper::SaveStringToFile(TEXT("fixture"),*C);
    ON_SCOPE_EXIT {IFileManager::Get().Delete(*A);IFileManager::Get().Delete(*B);IFileManager::Get().Delete(*C);};
    DataLoader Loader;Loader.SetDataPath(TCHAR_TO_ANSI(*(Root+TEXT("/"))));List<Text> Files;
    TestEqual(TEXT("Recursive filter discovers root and nested tracks"),Loader.ListFiles("*.ogg",Files,true),2);
    TestTrue(TEXT("Paths remain relative to requested data folder"),Files.size()==2&&FString(ANSI_TO_TCHAR(Files[0]->data()))==TEXT("Flight-loop.ogg")&&FString(ANSI_TO_TCHAR(Files[1]->data()))==TEXT("Sub/Combat-loop.ogg"));
    Files.destroy();TestEqual(TEXT("Nonrecursive filter excludes child folder"),Loader.ListFiles("*.ogg",Files,false),1);Files.destroy();return true;
}
#endif
