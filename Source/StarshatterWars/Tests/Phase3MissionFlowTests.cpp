#if WITH_DEV_AUTOMATION_TESTS && WITH_EDITOR
#include "Misc/AutomationTest.h"
#include "TimerSubsystem.h"
#include "MissionBriefingDlg.h"
#include "Mission.h"
#include "Campaign.h"
#include "Engine/GameInstance.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPhase3MissionClockTest,
    "Starshatter.Phase3.MissionClockLifecycle",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FPhase3MissionClockTest::RunTest(const FString& Parameters)
{
    UGameInstance* Instance = NewObject<UGameInstance>();
    UTimerSubsystem* Clock = NewObject<UTimerSubsystem>(Instance);
    Clock->StartMissionRun(true);
    Clock->ManualMissionTick(2.f);
    TestEqual(TEXT("Live mission advances time"), Clock->GetMissionTimeSeconds(), 2.);
    Clock->PauseMissionClock();
    Clock->ManualMissionTick(5.f);
    TestEqual(TEXT("Paused mission does not advance time"), Clock->GetMissionTimeSeconds(), 2.);
    Clock->ResumeMissionClock();
    Clock->ManualMissionTick(1.f);
    TestEqual(TEXT("Resumed mission continues time"), Clock->GetMissionTimeSeconds(), 3.);
    Clock->StopMissionRun();
    Clock->ResumeMissionClock();
    Clock->ManualMissionTick(9.f);
    TestEqual(TEXT("Stopped mission stays stopped during debrief/menu"), Clock->GetMissionTimeSeconds(), 3.);
    TestTrue(TEXT("Stopped clock state retained"), Clock->GetMissionClockState() == EMissionClockState::Stopped);
    Clock->StartMissionRun(true);
    TestEqual(TEXT("Relaunch resets the mission clock"), Clock->GetMissionTimeSeconds(), 0.);
    Clock->ManualMissionTick(1.f);
    TestEqual(TEXT("Relaunched mission advances time"), Clock->GetMissionTimeSeconds(), 1.);
    Clock->StopMissionRun();
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPhase3StandaloneStateTest,
    "Starshatter.Phase3.StandaloneBriefingIsolation",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FPhase3StandaloneStateTest::RunTest(const FString& Parameters)
{
    Campaign* StoryCampaign = Campaign::GetCampaign();
    Campaign Repository(Campaign::SINGLE_MISSIONS, "Phase 3 fixture", true);
    Mission FirstMission;
    Mission SecondMission;
    UMissionBriefingDlg* Briefing = NewObject<UMissionBriefingDlg>();
    Briefing->SetStandaloneMission(&FirstMission, &Repository);
    TestTrue(TEXT("Standalone entry selects supplied mission"), Briefing->GetMissionPtr() == &FirstMission);
    TestTrue(TEXT("Standalone entry is marked standalone"), Briefing->IsStandaloneMission());
    TestTrue(TEXT("Standalone entry preserves active story campaign"), Campaign::GetCampaign() == StoryCampaign);
    Briefing->ClearStandaloneMission();
    TestFalse(TEXT("Standalone exit clears its mode"), Briefing->IsStandaloneMission());
    TestTrue(TEXT("Standalone exit releases its mission pointer"), Briefing->GetMissionPtr() == nullptr);
    TestTrue(TEXT("Standalone exit preserves active story campaign"), Campaign::GetCampaign() == StoryCampaign);
    Briefing->SetStandaloneMission(&SecondMission, &Repository);
    TestTrue(TEXT("Reopening uses the new mission rather than the previous one"), Briefing->GetMissionPtr() == &SecondMission);
    Briefing->ClearStandaloneMission();
    return true;
}
#endif
