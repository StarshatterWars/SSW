#if WITH_DEV_AUTOMATION_TESTS && WITH_EDITOR
#include "Misc/AutomationTest.h"
#include "Misc/ScopeExit.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "HAL/FileManager.h"
#include "Campaign.h"
#include "MissionInfo.h"
namespace {
class FDeleteTestCampaign : public Campaign {
public:
    FDeleteTestCampaign(int Id,const FString& Root):Campaign(Id,"Deletion audit",true){FCStringAnsi::Strcpy(path,TCHAR_TO_ANSI(*Root));}
    virtual void Load() override {++Reloads;}
    int Reloads=0;
    void Add(int Id,const char* Script){auto* M=new MissionInfo;M->id=Id;M->script=Script;GetMissionList().append(M);}
};}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCustomMissionDeletionTest,"Starshatter.Audit.F28.CustomMissionDeletion",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FCustomMissionDeletionTest::RunTest(const FString& Parameters)
{
    const FString Root=FPaths::ConvertRelativePathToFull(FPaths::ProjectSavedDir()/TEXT("Automation/P2Delete"));
    const FString Owned=Root/TEXT("owned.def"),Outside=Root/TEXT("../P2Outside.def");
    IFileManager::Get().MakeDirectory(*Root,true);FFileHelper::SaveStringToFile(TEXT("fixture"),*Owned);FFileHelper::SaveStringToFile(TEXT("fixture"),*Outside);
    ON_SCOPE_EXIT {IFileManager::Get().Delete(*Owned);IFileManager::Get().Delete(*Outside);};
    FDeleteTestCampaign Custom(Campaign::CUSTOM_MISSIONS,Root);Custom.Add(1,"owned.def");Custom.Add(2,"../P2Outside.def");
    Custom.DeleteMission(2);TestTrue(TEXT("A selected script cannot escape its mission folder"),IFileManager::Get().FileExists(*Outside));
    Custom.DeleteMission(1);TestFalse(TEXT("A custom mission is deleted from its source folder"),IFileManager::Get().FileExists(*Owned));TestEqual(TEXT("Successful deletion refreshes the campaign list"),Custom.Reloads,1);
    FFileHelper::SaveStringToFile(TEXT("fixture"),*Owned);FDeleteTestCampaign BuiltIn(Campaign::SINGLE_MISSIONS,Root);BuiltIn.Add(1,"owned.def");BuiltIn.DeleteMission(1);
    TestTrue(TEXT("Built-in campaign missions cannot be deleted"),IFileManager::Get().FileExists(*Owned));return true;
}
#endif
