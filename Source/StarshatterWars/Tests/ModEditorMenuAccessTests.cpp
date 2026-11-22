#if WITH_DEV_AUTOMATION_TESTS && WITH_EDITOR
#include "Misc/AutomationTest.h"
#include "Misc/Paths.h"
#include "Misc/FileHelper.h"
#include "Misc/ScopeExit.h"
#include "HAL/FileManager.h"
#include "MenuScreen.h"
#include "ModEditorDlg.h"
#include "SSWMissionMods.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDockedMenuAccessTest,"Starshatter.UI.Menu.DockedAccessPolicy",
    EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FDockedMenuAccessTest::RunTest(const FString&)
{
    TestTrue(TEXT("Initial startup has access without a flying pilot"),UMenuScreen::MenuAccessForState(false,false,false));
    TestTrue(TEXT("Session browser is usable before joining a mission"),UMenuScreen::MenuAccessForState(false,false,true));
    TestFalse(TEXT("Flying pilot cannot leave mission for main menu"),UMenuScreen::MenuAccessForState(true,false,false));
    TestTrue(TEXT("Docked single-player pilot may open main menu"),UMenuScreen::MenuAccessForState(true,true,false));
    TestFalse(TEXT("Network pilot cannot evade resign by docking"),UMenuScreen::MenuAccessForState(true,true,true));
    TestFalse(TEXT("Network flight requires resign and cleanup"),UMenuScreen::MenuAccessForState(true,false,true));
    TestEqual(TEXT("Mod Editor and main menu use the same live docking rule"),UModEditorDlg::CanUseDockedEditor(),UMenuScreen::CanOpenMainMenu());
    return true;
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FModEditorPathPolicyTest,"Starshatter.UI.ModEditor.UserSourceIsolation",
    EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FModEditorPathPolicyTest::RunTest(const FString&)
{
    const FString Root=SSWMissionMods::Directory();
    TestFalse(TEXT("Legacy DEF requires explicit conversion before editing"),UModEditorDlg::IsEditableMissionPath(Root/TEXT("Example/Missions/Exercise.def")));
    TestFalse(TEXT("Authored game mission is protected"),UModEditorDlg::IsEditableMissionPath(FPaths::ProjectContentDir()/TEXT("GameData/Campaigns/01/Scenes/T1.def")));
    TestFalse(TEXT("Sibling directory cannot impersonate Mods"),UModEditorDlg::IsEditableMissionPath(Root+TEXT("_other/Exercise.def")));
    TestFalse(TEXT("Parent traversal cannot escape user Mods"),UModEditorDlg::IsEditableMissionPath(Root/TEXT("../Protected.def")));
    TestFalse(TEXT("Archive binary is not a text mission"),UModEditorDlg::IsEditableMissionPath(Root/TEXT("Example.dat")));
    const FString Folder=Root/(TEXT("Automation_")+FGuid::NewGuid().ToString(EGuidFormats::Digits))/TEXT("Missions");
    const FString Json=Folder/TEXT("Exercise.json");
    if(!TestTrue(TEXT("Temporary fixture folder created"),IFileManager::Get().MakeDirectory(*Folder,true)))return false;
    ON_SCOPE_EXIT
    {
        IFileManager::Get().Delete(*Json);
        IFileManager::Get().DeleteDirectory(*Folder,false,false);
        IFileManager::Get().DeleteDirectory(*FPaths::GetPath(Folder),false,false);
    };
    if(!TestTrue(TEXT("JSON priority fixture written"),FFileHelper::SaveStringToFile(TEXT("{}"),*Json)))return false;
    TestTrue(TEXT("User JSON mission source can be edited"),UModEditorDlg::IsEditableMissionPath(Json));
    TestFalse(TEXT("Cannot silently edit shadowed DEF when JSON is authoritative"),UModEditorDlg::IsEditableMissionPath(FPaths::ChangeExtension(Json,TEXT("def"))));
    return true;
}
#endif
