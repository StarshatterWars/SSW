#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "PreparedSceneGate.h"
#include "Mission.h"
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPreparedSceneGateTest,"Starshatter.Campaign.ScenePreparationDoesNotAdvance",
    EAutomationTestFlags_ApplicationContextMask|EAutomationTestFlags::EngineFilter)
bool FPreparedSceneGateTest::RunTest(const FString&)
{
    Mission Intro,Other;FPreparedSceneGate Gate;
    TestFalse(TEXT("Ordinary mission is not blocked"),Gate.Blocks(&Intro));
    Gate.Prepare(&Intro);
    TestTrue(TEXT("Intro ships and events wait for camera/assets"),Gate.Blocks(&Intro));
    TestFalse(TEXT("Stale cinematic cannot block a replacement mission"),Gate.Blocks(&Other));
    Gate.Begin(&Other);TestTrue(TEXT("Unrelated playback cannot release prepared intro"),Gate.Blocks(&Intro));
    Gate.Begin(&Intro);TestFalse(TEXT("Prepared intro starts only at playback"),Gate.Blocks(&Intro));
    Gate.Prepare(&Intro);Gate.Reset();TestFalse(TEXT("Unload cancels preparation gate"),Gate.Blocks(&Intro));
    Gate.Prepare(nullptr);TestFalse(TEXT("Empty mission cannot block startup"),Gate.Blocks(nullptr));return true;
}
#endif
