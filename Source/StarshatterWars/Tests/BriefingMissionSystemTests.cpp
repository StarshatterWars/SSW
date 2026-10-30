#if WITH_DEV_AUTOMATION_TESTS && WITH_EDITOR
#include "Misc/AutomationTest.h"
#include "MissionNavDlg.h"
#include "MissionBriefingDlg.h"
#include "Mission.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FBriefingMissionSystemTest,
    "Starshatter.Briefing.MissionSystemSelection",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FBriefingMissionSystemTest::RunTest(const FString& Parameters)
{
    Mission FirstMission;
    FirstMission.SetSystem("  Solus  ");
    Mission SecondMission;
    SecondMission.SetSystem("Solus");
    UMissionBriefingDlg* Briefing = NewObject<UMissionBriefingDlg>();
    UMissionNavDlg* Navigation = NewObject<UMissionNavDlg>();
    Navigation->SetParentDlg(Briefing);
    Navigation->HandleGalaxySystemSelected(TEXT("Previous system"));
    Briefing->SetMission(&FirstMission);
    Navigation->RefreshFromMission();
    TestEqual(TEXT("Authored system is selected even before runtime system resolution"),
        Navigation->GetSelectedGalaxySystemName(), FString(TEXT("Solus")));

    Navigation->HandleGalaxySystemSelected(TEXT("Inspected system"));
    Navigation->RefreshFromMission();
    TestEqual(TEXT("Same-mission refresh preserves manual browsing"),
        Navigation->GetSelectedGalaxySystemName(), FString(TEXT("Inspected system")));

    Briefing->SetMission(&SecondMission);
    Navigation->RefreshFromMission();
    TestEqual(TEXT("New mission resets a prior browsing selection"),
        Navigation->GetSelectedGalaxySystemName(), FString(TEXT("Solus")));

    SecondMission.SetSystem("Changed system");
    Navigation->RefreshFromMission();
    TestEqual(TEXT("Changed mission system resets the default"),
        Navigation->GetSelectedGalaxySystemName(), FString(TEXT("Changed system")));
    Briefing->SetMission(nullptr);
    Navigation->RefreshFromMission();
    TestTrue(TEXT("Missing mission clears stale selection"), Navigation->GetSelectedGalaxySystemName().IsEmpty());
    return true;
}
#endif
