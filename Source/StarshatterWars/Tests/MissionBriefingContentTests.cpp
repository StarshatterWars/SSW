#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "MissionBriefingContent.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMissionBriefingContentTest, "Starshatter.Mission.SharedBriefingContent",
    EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::EngineFilter)
bool FMissionBriefingContentTest::RunTest(const FString&)
{
    Mission Model;
    Model.SetSystem("Janus");
    Model.SetRegion("Janek");
    Model.SetDescription("Carrier launch and recovery.");
    Model.SetObjective("Return to Archon.");
    Model.SetSituation("Third Fleet training exercise.");
    const FString Body = MissionBriefingContent::Build(Model);
    TestTrue(TEXT("Briefing contains its origin"), Body.Contains(TEXT("Janus / Janek")));
    TestTrue(TEXT("Description remains available in live objectives"), Body.Contains(TEXT("Carrier launch and recovery.")));
    TestTrue(TEXT("Objectives retained"), Body.Contains(TEXT("Return to Archon.")));
    TestTrue(TEXT("Situation retained"), Body.Contains(TEXT("Third Fleet training exercise.")));
    Model.SetDescription("");
    TestFalse(TEXT("Missing description does not create a blank section"), MissionBriefingContent::Build(Model).Contains(TEXT("BRIEFING\n")));
    return true;
}
#endif
