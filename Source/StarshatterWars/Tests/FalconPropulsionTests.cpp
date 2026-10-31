#if WITH_DEV_AUTOMATION_TESTS && WITH_EDITOR
#include "Misc/AutomationTest.h"
#include "Engine/DataTable.h"
#include "GameStructs_System.h"
#include "Power.h"
#include "Drive.h"
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FFalconPropulsionTest,"Starshatter.Fighters.FalconPropulsion",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FFalconPropulsionTest::RunTest(const FString& Parameters)
{
    auto* Table=LoadObject<UDataTable>(nullptr,TEXT("/Game/Game/DT_ShipDesign.DT_ShipDesign"));
    const auto* F=Table?Table->FindRow<FShipDesign>(TEXT("Falcon"),TEXT("Propulsion")):nullptr;
    const auto* S=Table?Table->FindRow<FShipDesign>(TEXT("Stormhawk"),TEXT("Propulsion")):nullptr;
    if(!TestNotNull(TEXT("Falcon"),F)||!TestNotNull(TEXT("Stormhawk"),S))return false;
    TestEqual(TEXT("Navigation component matches Stormhawk coverage"),F->NavSys.Num(),S->NavSys.Num());
    TestTrue(TEXT("Falcon has navigation"),F->NavSys.Num()>0);
    TestEqual(TEXT("Computer coverage"),F->Computer.Num(),S->Computer.Num());
    if(!TestTrue(TEXT("Reactor and drive exist"),!F->Power.IsEmpty()&&!F->Drive.IsEmpty()))return false;
    TestEqual(TEXT("Authored Falcon reactor output"),F->Power[0].Output,65.f);
    PowerSource Reactor(F->Power[0].Type,F->Power[0].Output,F->Power[0].Fuel);
    Drive Engine(F->Drive[0].Type,F->Drive[0].Thrust,F->Drive[0].Augmenter,false);
    Reactor.AddClient(&Engine);
    float Output=0;
    for(int32 I=0;I<300;++I){Engine.SetThrottle(100,false,1./60.);Reactor.ExecFrame(1./60.);Output=Engine.GetThrust(1./60.);}
    TestTrue(TEXT("Real reactor supplies positive drive thrust at full throttle"),Output>0.f);
    AddInfo(FString::Printf(TEXT("Falcon full-throttle output %.3f, visual power %.3f"),Output,Engine.GetVisualPower()));
    Engine.SetThrottle(0,false,2.);Reactor.ExecFrame(1./60.);
    TestEqual(TEXT("Zero throttle gives zero thrust"),Engine.GetThrust(1./60.),0.f);
    return true;
}
#endif
