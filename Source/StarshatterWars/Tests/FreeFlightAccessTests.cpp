#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "PlayerFlightAccess.h"
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FFreeFlightRankAccessTest,"Starshatter.Agents.UnassignedFlightRequiresEnsign",
    EAutomationTestFlags_ApplicationContextMask|EAutomationTestFlags::EngineFilter)
bool FFreeFlightRankAccessTest::RunTest(const FString&)
{
    TestFalse(TEXT("Cadets cannot launch without an assignment"),PlayerFlightAccess::CanLaunch(false,0));
    TestTrue(TEXT("Ensign promotion opens unassigned flight"),PlayerFlightAccess::CanLaunch(false,1));
    TestTrue(TEXT("Senior officers retain free flight"),PlayerFlightAccess::CanLaunch(false,6));
    TestTrue(TEXT("Assigned training sorties remain available to cadets"),PlayerFlightAccess::CanLaunch(true,0));
    TestFalse(TEXT("Invalid rank cannot unlock unassigned flight"),PlayerFlightAccess::CanLaunch(false,-1));
    return true;
}
#endif
