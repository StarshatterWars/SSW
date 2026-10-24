#if WITH_DEV_AUTOMATION_TESTS && WITH_EDITOR
#include "Misc/AutomationTest.h"
#include "EngineeringDlg.h"
#include "Power.h"
#include "SimSystem.h"
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FEngineeringControlsTest,"Starshatter.Audit.F08.EngineeringControls",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FEngineeringControlsTest::RunTest(const FString& Parameters)
{
    PowerSource Source(EPowerSource::BATTERY,100);
    UEngineeringDlg* Dialog=NewObject<UEngineeringDlg>();
    Dialog->selected_source=&Source;
    TestFalse(TEXT("Reactor starts without override"),Source.IsOverrideActive());
    Dialog->OnOverride();TestTrue(TEXT("First click enables reactor requested override"),Source.IsOverrideActive());
    TestTrue(TEXT("Actual output need not exceed 100 for override to be active"),Source.GetPowerLevel()<=100);
    Dialog->OnOverride();TestFalse(TEXT("Second click disables override even at limited output"),Source.IsOverrideActive());
    SimSystem Client(SYSTEM_CATEGORY::MISC_SYSTEM,0,"F08 client",1);
    Dialog->selected_source=nullptr;Dialog->selected_clients.append(&Client);
    Dialog->OnOverride();TestTrue(TEXT("Client override enables"),Client.IsOverrideActive());
    Dialog->OnOverride();TestFalse(TEXT("Client override disables"),Client.IsOverrideActive());
    Dialog->selected_clients.clear();Dialog->OnOverride();
    return true;
}
#endif
