#if WITH_DEV_AUTOMATION_TESTS && WITH_EDITOR
#include "Misc/AutomationTest.h"
#include "ShipMFDPreview.h"
#include "GameStructs_System.h"
#include "Engine/StaticMesh.h"
#include "Engine/DataTable.h"
#include "Sim.h"
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FShipMFDisolationTest,"Starshatter.MFD.PreviewIsolationAndTargetReplacement",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FShipMFDisolationTest::RunTest(const FString& Parameters)
{
    Sim* Before=Sim::GetSim();
    FShipMFDPreview Player,Target;
    auto* Table=LoadObject<UDataTable>(nullptr,TEXT("/Game/Game/DT_ShipDesign.DT_ShipDesign"));
    if(!TestNotNull(TEXT("Ship design table"),Table))return false;
    for(const TCHAR* Name:{TEXT("Falcon"),TEXT("Thunderbolt"),TEXT("Eagle"),TEXT("Orion")})
    {
        const auto* Design=Table->FindRow<FShipDesign>(FName(Name),TEXT("MFD tests"));
        if(!TestNotNull(Name,Design))continue;
        TestTrue(TEXT("Actual player model creates an isolated 3D preview"),Player.SetDesign(Design));
        TestTrue(TEXT("Target replacement resolves the actual model"),Target.SetDesign(Design));
        TestTrue(TEXT("Target capture is ready"),Target.IsReady());
        TestTrue(TEXT("Repeated samples reuse the existing preview"),Target.SetDesign(Design));
        TestTrue(TEXT("Preview does not create or replace gameplay Sim"),Sim::GetSim()==Before);
    }
    TestFalse(TEXT("Lost target clears its preview"),Target.SetDesign(nullptr));
    TestFalse(TEXT("Lost target retains no stale model"),Target.IsReady());
    TestTrue(TEXT("Clearing target preserves player preview"),Player.IsReady());
    return true;
}
#endif
