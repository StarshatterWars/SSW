#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "CinematicSimulationStep.h"
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCinematicStepTest,"Starshatter.Campaign.Cinematics.PausedMenuLifecycleStep",
 EAutomationTestFlags_ApplicationContextMask|EAutomationTestFlags::EngineFilter)
bool FCinematicStepTest::RunTest(const FString&)
{
 const bool OriginalPause=Game::Paused();const double OriginalSeconds=Game::FrameTime();const uint32 OriginalRate=Game::GetSimulationRate();
 Game::SetPausedState(true);Game::SetSimulationStep(0,1);
 {
  FScopedCinematicSimulationStep Scene(.04);
  TestFalse(TEXT("Native docking/damage pause gate opens for owned cinematic"),Game::Paused());
  TestEqual(TEXT("Legacy systems receive the cinematic step"),Game::FrameTime(),.04);
  {FScopedCinematicSimulationStep Nested(.02);TestEqual(TEXT("Nested step"),Game::FrameTime(),.02);}
  TestEqual(TEXT("Nested scope restores its owner"),Game::FrameTime(),.04);
 }
 TestTrue(TEXT("Menu remains paused after the cinematic frame"),Game::Paused());
 TestEqual(TEXT("Paused menu has no flight step"),Game::FrameTime(),0.0);
 Game::SetPausedState(false);Game::SetSimulationStep(.1,5);
 {FScopedCinematicSimulationStep Scene(.03);TestEqual(TEXT("Cinematics never inherit travel acceleration"),Game::GetSimulationRate(),1u);}
 TestFalse(TEXT("Originally running state preserved"),Game::Paused());TestEqual(TEXT("Travel state restored"),Game::GetSimulationRate(),5u);
 TestEqual(TEXT("Previous real frame restored"),Game::FrameTime(),.1);
 Game::SetSimulationStep(OriginalSeconds,OriginalRate);Game::SetPausedState(OriginalPause);return true;
}
#endif
