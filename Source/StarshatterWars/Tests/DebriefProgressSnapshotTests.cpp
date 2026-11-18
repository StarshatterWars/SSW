#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "DebriefProgressSnapshot.h"
#include "UObject/UObjectGlobals.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDebriefProgressRollbackTest,"Starshatter.Missions.Debrief.ProgressRollback",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FDebriefProgressRollbackTest::RunTest(const FString&)
{
    const FName Pilot(TEXT("DebriefPilot")), Issuer(TEXT("Instructor"));
    AgentAI Agent; FS_PlayerGameInfo Player;
    TArray<int32> Eligible={101};
    Agent.Request(Pilot,Eligible,Issuer); Agent.Accept(Pilot,101); Agent.Start(Pilot,101);
    Player.PlayerPoints=25;
    UCampaignSave* Save=NewObject<UCampaignSave>();
    Save->PlayedSceneNames.Add(TEXT("intro"));
    Save->CampaignElapsedAtAnchor=42;
    FDebriefProgressSnapshot Before(Agent,Player,nullptr,Save);
    Agent.Resolve(Pilot,101,true);
    Player.PlayerPoints=100; Player.bTrainingGraduate=true;
    Save->CompletedMissionIds.Add(101); Save->MissionDebriefs.AddDefaulted();
    Save->PlayedSceneNames.Add(TEXT("outro")); Save->CampaignElapsedAtAnchor=99;
    Before.Restore(Agent,Player,Save);
    TestFalse(TEXT("Failed save cannot retain agent completion"),Agent.IsCompleted(Pilot,101));
    TestEqual(TEXT("Running assignment retained for retry"),Agent.GetQueue(Pilot).Num(),1);
    if(!Agent.GetQueue(Pilot).IsEmpty())
        TestTrue(TEXT("Exact running state restored"),Agent.GetQueue(Pilot)[0].State==AgentAI::EState::InProgress);
    TestEqual(TEXT("Points restored"),Player.PlayerPoints,25);
    TestFalse(TEXT("Graduation restored"),Player.bTrainingGraduate);
    TestTrue(TEXT("Campaign completion restored"),Save->CompletedMissionIds.IsEmpty());
    TestTrue(TEXT("Debrief receipt restored"),Save->MissionDebriefs.IsEmpty());
    TestEqual(TEXT("Scene history restored"),Save->PlayedSceneNames.Num(),1);
    TestEqual(TEXT("Clock anchor restored"),Save->CampaignElapsedAtAnchor,uint64(42));
    TestTrue(TEXT("Retry resolves once"),Agent.Resolve(Pilot,101,true));
    TestFalse(TEXT("Repeated completion rejected"),Agent.Resolve(Pilot,101,true));
    return true;
}
#endif
