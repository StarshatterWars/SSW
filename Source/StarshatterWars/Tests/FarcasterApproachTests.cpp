#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "Farcaster.h"

namespace
{
    class FApproachFixture : public Farcaster
    {
    public:
        FApproachFixture() : Farcaster(100, 10) {}
        void Geometry(const FVector& Start, const FVector& Approach)
        { start_point = Start; approach_point[0] = Approach; }
    };
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FFarcasterApproachEntryTest, "Starshatter.Travel.GateApproach.EntryChecks",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FFarcasterApproachEntryTest::RunTest(const FString&)
{
    FApproachFixture Gate;
    TestFalse(TEXT("Unprovisioned gate is unavailable"), Gate.HasValidApproach());
    TestFalse(TEXT("Zero-axis gate cannot admit stationary craft"), Gate.CanEnterAt(FVector::ZeroVector, FVector::ForwardVector, FVector::ZeroVector));
    Gate.Geometry(FVector::ZeroVector, FVector(-2000,0,0));
    TestTrue(TEXT("Authored approach is valid"), Gate.HasValidApproach());
    const FVector Position(-500,0,0), Heading(1,0,0), Velocity(250,0,0);
    TestTrue(TEXT("Aligned inbound craft enters"), Gate.CanEnterAt(Position, Heading, Velocity));
    TestFalse(TEXT("Outside entry sphere"), Gate.CanEnterAt(FVector(-1000,0,0), Heading, Velocity));
    TestFalse(TEXT("Stationary does not jump"), Gate.CanEnterAt(Position, Heading, FVector::ZeroVector));
    TestFalse(TEXT("Reverse velocity rejected"), Gate.CanEnterAt(Position, Heading, -Velocity));
    TestFalse(TEXT("Sideways heading rejected"), Gate.CanEnterAt(Position, FVector(0,1,0), Velocity));
    TestFalse(TEXT("Sideways velocity rejected"), Gate.CanEnterAt(Position, Heading, FVector(0,250,0)));
    TestFalse(TEXT("Outside corridor rejected"), Gate.CanEnterAt(FVector(-500,351,0), Heading, Velocity));
    TestFalse(TEXT("Beyond exit-side tolerance rejected"), Gate.CanEnterAt(FVector(351,0,0), Heading, Velocity));
    const FVector Tilted = FRotator(0,16,0).RotateVector(Heading);
    TestFalse(TEXT("Over fifteen degree heading rejected"), Gate.CanEnterAt(Position, Tilted, Velocity));
    TestFalse(TEXT("Over fifteen degree drift rejected"), Gate.CanEnterAt(Position, Heading, Tilted*250));
    const FVector Within = FRotator(0,14,0).RotateVector(Heading);
    TestTrue(TEXT("Within heading/drift cone accepted"), Gate.CanEnterAt(Position, Within, Within*250));
    return true;
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FFarcasterApproachRoutingTest, "Starshatter.Travel.GateApproach.RouteGeometry",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FFarcasterApproachRoutingTest::RunTest(const FString&)
{
    FApproachFixture Gate;
    const FVector Start(12345,-45678,9000);
    const FQuat Rotation = FRotator(30,75,10).Quaternion();
    const FVector Axis = Rotation.RotateVector(FVector::ForwardVector);
    const FVector Side = Rotation.RotateVector(FVector::RightVector);
    Gate.Geometry(Start, Start-Axis*2000);
    TestTrue(TEXT("Off-lane craft targets approach before gate"), Gate.GetApproachTarget(Start+Side*800).Equals(Start-Axis*2000,0.001));
    TestTrue(TEXT("Staged craft flies through, not stops at gate center"), Gate.GetApproachTarget(Start-Axis*1500).Equals(Start+Axis*2000,0.001));
    TestTrue(TEXT("Rotated translated valid entry"), Gate.CanEnterAt(Start-Axis*500,Axis,Axis*250));
    TestFalse(TEXT("Exit-side craft cannot enter backwards"), Gate.CanEnterAt(Start+Axis*500,-Axis,-Axis*250));
    TestTrue(TEXT("Exit-side craft repositions to approach"), Gate.GetApproachTarget(Start+Axis*1500).Equals(Start-Axis*2000,0.001));
    Gate.Geometry(Start,Start);
    TestFalse(TEXT("Coincident approach fails closed"), Gate.CanEnterAt(Start,Axis,Axis*250));
    return true;
}
#endif
