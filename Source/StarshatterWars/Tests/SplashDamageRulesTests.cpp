#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "SplashDamageRules.h"
#include <limits>
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FSplashDamageBoundsTest,"Starshatter.Simulation.Splash.BoundsAndFiniteInputs",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FSplashDamageBoundsTest::RunTest(const FString&)
{
    const double NaN=std::numeric_limits<double>::quiet_NaN();
    const double Infinity=std::numeric_limits<double>::infinity();
    TestTrue(TEXT("Valid native blast"),SplashDamageRules::Valid(1000,735,FVector::ZeroVector));
    TestFalse(TEXT("Non-finite damage rejected"),SplashDamageRules::Valid(Infinity,735,FVector::ZeroVector));
    TestFalse(TEXT("Non-finite range rejected"),SplashDamageRules::Valid(1000,NaN,FVector::ZeroVector));
    TestFalse(TEXT("Invalid source coordinate rejected"),SplashDamageRules::Valid(1000,735,FVector(NaN,0,0)));
    TestEqual(TEXT("Existing linear falloff preserved"),SplashDamageRules::AtDistance(1000,100,50),500.0);
    TestEqual(TEXT("Outside blast remains unharmed"),SplashDamageRules::AtDistance(1000,735,45000),0.0);
    TestEqual(TEXT("Exact boundary remains unharmed"),SplashDamageRules::AtDistance(1000,735,735),0.0);
    TestEqual(TEXT("Invalid distance cannot create damage"),SplashDamageRules::AtDistance(1000,735,NaN),0.0);
    TestEqual(TEXT("Coincident origin legacy exclusion preserved"),SplashDamageRules::AtDistance(1000,735,0),0.0);
    return true;
}
#endif
