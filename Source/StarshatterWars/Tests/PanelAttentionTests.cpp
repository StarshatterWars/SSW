#if WITH_DEV_AUTOMATION_TESTS && WITH_EDITOR
#include "Misc/AutomationTest.h"
#include "PanelAttentionState.h"
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPanelAttentionTest,"Starshatter.Mission.PanelAttention",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FPanelAttentionTest::RunTest(const FString&)
{
 FPanelAttentionState State;State.BeginSession(TEXT("Pilot A"));
 State.Observe(TEXT("Cargo"),1,false);TestFalse(TEXT("Initial data is not an unread update"),State.IsUnread(TEXT("Cargo")));
 State.Observe(TEXT("Cargo"),2,false);TestTrue(TEXT("Closed panel change highlights its button"),State.IsUnread(TEXT("Cargo")));
 State.Observe(TEXT("Cargo"),2,false);TestTrue(TEXT("Highlight persists until viewed"),State.IsUnread(TEXT("Cargo")));
 State.Acknowledge(TEXT("Cargo"));TestFalse(TEXT("Opening acknowledges update"),State.IsUnread(TEXT("Cargo")));
 State.Observe(TEXT("Cargo"),3,true);TestFalse(TEXT("Open panels do not collect unread alerts"),State.IsUnread(TEXT("Cargo")));
 State.Observe(TEXT("Cargo"),4,false);TestTrue(TEXT("Later closed update highlights again"),State.IsUnread(TEXT("Cargo")));
 State.Observe(TEXT("Intel"),1,false);TestFalse(TEXT("Independent panel starts read"),State.IsUnread(TEXT("Intel")));
 State.BeginSession(TEXT("Pilot B"));TestFalse(TEXT("Pilot switch clears prior alerts"),State.IsUnread(TEXT("Cargo")));
 return true;
}
#endif
