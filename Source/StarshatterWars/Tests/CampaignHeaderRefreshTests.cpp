#if WITH_DEV_AUTOMATION_TESTS && WITH_EDITOR
#include "Misc/AutomationTest.h"
#include "Misc/ScopeExit.h"
#include "Campaign.h"
#include "CmdDlg.h"
#include "CmpLoadDlg.h"
#include "Components/TextBlock.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCampaignHeaderRefreshTest,"Starshatter.CampaignUI.ReusedCampaignHeaders",
    EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FCampaignHeaderRefreshTest::RunTest(const FString& Parameters)
{
    if(Campaign::GetCampaign()){AddError(TEXT("Run campaign-switch fixture in an isolated Editor process"));return false;}
    ON_SCOPE_EXIT {Campaign::Close();};
    FS_Campaign First;First.Index=0;First.Name=TEXT("First Campaign");First.Start=TEXT("00:00:00");
    FS_Campaign Second;Second.Index=1;Second.Name=TEXT("Second Campaign");Second.Start=TEXT("00:00:00");
    if(!TestNotNull(TEXT("First campaign activated"),Campaign::SelectFromData(First)))return false;
    auto* Command=NewObject<UCmdDlg>();Command->CampaignNameText=NewObject<UTextBlock>(Command);
    Command->CurrentCampaignData=First;Command->bHasCurrentCampaign=true;
    Command->RefreshOperationsHeader();
    TestEqual(TEXT("Initial operations title"),Command->CampaignNameText->GetText().ToString(),First.Name.ToUpper());
    auto* Loading=NewObject<UCmpLoadDlg>();Loading->CenterTitleText=NewObject<UTextBlock>(Loading);
    // Selection requests B while A remains active until the next tick.
    Loading->SetCampaignName(Second.Name);Loading->ApplyCampaignTitleCard();
    TestEqual(TEXT("Deferred activation cannot replace requested B with old A"),Loading->CenterTitleText->GetText().ToString(),Second.Name);
    if(!TestNotNull(TEXT("Second campaign activated"),Campaign::SelectFromData(Second)))return false;
    Command->CurrentCampaignData=First;Command->bHasCurrentCampaign=true;
    Command->RefreshOperationsHeader();
    TestEqual(TEXT("Reused operations widget discards stale A"),Command->CampaignNameText->GetText().ToString(),Second.Name.ToUpper());
    Loading->Hide();Loading->ApplyCampaignTitleCard();
    TestEqual(TEXT("Next loading lifecycle resolves active B"),Loading->CenterTitleText->GetText().ToString(),Second.Name);
    Loading->SetCampaignName(First.Name);Loading->ApplyCampaignTitleCard();
    TestEqual(TEXT("Switch back title replaces B immediately"),Loading->CenterTitleText->GetText().ToString(),First.Name);
    return true;
}
#endif

