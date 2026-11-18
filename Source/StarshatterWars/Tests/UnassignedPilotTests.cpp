#if WITH_DEV_AUTOMATION_TESTS && WITH_EDITOR
#include "Misc/AutomationTest.h"
#include "SSWGameInstance.h"
#include "Campaign.h"
#include "CampaignSave.h"
#include "CampaignPresentationRules.h"
#include "CreditsContent.h"
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FUnassignedPilotTest,"Starshatter.Menu.UnassignedPilot",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FUnassignedPilotTest::RunTest(const FString& Parameters)
{
    auto* Game=NewObject<USSWGameInstance>();
    Game->SelectedCampaignIndex=1;Game->SelectedCampaignRowName=TEXT("OldSelection");Game->SelectedCampaignDisplayName=TEXT("Old assignment");Game->CampaignSave=NewObject<UCampaignSave>(Game);
    Game->ClearCampaignAssignment();
    TestEqual(TEXT("No campaign index is selected"),Game->SelectedCampaignIndex,0);
    TestTrue(TEXT("No campaign row is selected"),Game->SelectedCampaignRowName.IsNone());
    TestNull(TEXT("No campaign save is attached"),Game->CampaignSave);
    TestNull(TEXT("No runtime campaign is active"),Campaign::GetCampaign());
    TestNull(TEXT("Unassigned pilot cannot accidentally create a campaign save"),Game->LoadOrCreateSelectedCampaignSave());
    FS_PlayerGameInfo Pilot;
    TestEqual(TEXT("New pilot data starts unassigned"),Pilot.Campaign,0);
    Pilot.MarkMissionCompleted(Campaign::TRAINING_CAMPAIGN,103);
    TestTrue(TEXT("Flight School unlocks agent orders without an active campaign"),CampaignPresentationRules::CanSelectCampaign(Pilot));
    TestEqual(TEXT("Training completion does not assign a campaign"),Pilot.Campaign,0);
    TestTrue(TEXT("Credits load the authored team list"),CreditsContent::Load().Contains(TEXT("Carlos Bott")));
    return true;
}
#endif
