#if WITH_DEV_AUTOMATION_TESTS && WITH_EDITOR
#include "Misc/AutomationTest.h"
#include "RecoveryGuidance.h"
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRecoveryMarkerGuidanceTest,"Starshatter.Carrier.Recovery.Markers",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FRecoveryMarkerGuidanceTest::RunTest(const FString& Parameters)
{
    const FVector Start(305,0,-1950),End(305,-12,-500);
    TestTrue(TEXT("Outer station coincides with legacy far hoop/start"),RecoveryGuidance::Marker(Start,End,0).Equals(Start));
    TestTrue(TEXT("Middle station coincides with legacy half-distance hoop"),RecoveryGuidance::Marker(Start,End,1).Equals(End+(Start-End)*0.5));
    TestTrue(TEXT("Inner station coincides with legacy quarter-distance hoop"),RecoveryGuidance::Marker(Start,End,2).Equals(End+(Start-End)*0.25));
    FTransform Carrier(FRotator(15,75,0),FVector(2000,5000,700));
    for(int32 Index=0;Index<3;++Index)
        TestTrue(TEXT("Markers remain carrier relative after translation/rotation"),
            RecoveryGuidance::Marker(Carrier.TransformPosition(Start),Carrier.TransformPosition(End),Index).Equals(
                Carrier.TransformPosition(RecoveryGuidance::Marker(Start,End,Index)),0.001));
    return true;
}
#endif
