#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "NavAI.h"
#include "Ship.h"
#include "ShipDesign.h"
#include "Instruction.h"
#include "Misc/ScopeExit.h"
namespace {
class FCruiseThrottleProbe : public NavAI {
public:
    explicit FCruiseThrottleProbe(Ship* S):NavAI(S){}
    void Step(Instruction& Leg,double Dt){navpt=&Leg;seconds=Dt;ThrottleControl();}
};
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FAutonavCruiseThrottleTest,"Starshatter.Navigation.CruiseThrottleFullRange",EAutomationTestFlags_ApplicationContextMask|EAutomationTestFlags::EngineFilter)
bool FAutonavCruiseThrottleTest::RunTest(const FString&)
{
    const int OldModel=Ship::GetFlightModel();ON_SCOPE_EXIT{Ship::SetFlightModel(OldModel);};
    for(int Model=0;Model<=2;++Model)for(int FPS:{30,60,120})
    {
        Ship::SetFlightModel(Model);
        ShipDesign D;D.type=static_cast<int>(CLASSIFICATION::LCA);D.mass=100;D.integrity=100;D.vlimit=1000;
        Ship Craft("Cruise fixture","",&D,1,0,nullptr,false);
        FCruiseThrottleProbe Nav(&Craft);Instruction Leg("Cruise",FVector::ZeroVector);Leg.SetSpeed(800);
        for(int Frame=0;Frame<FPS*3;++Frame)Nav.Step(Leg,1.0/FPS);
        TestTrue(TEXT("Under-speed non-fighter autonav can request full thrust in every flight model"),FMath::IsNearlyEqual(Craft.GetThrottleRequest(),100.0,.01));
        TestFalse(TEXT("No augmenter is invented for a craft without a drive"),Craft.GetAugmenter());
    }
    return true;
}
#endif
