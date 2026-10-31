#if WITH_DEV_AUTOMATION_TESTS && WITH_EDITOR
#include "Misc/AutomationTest.h"
#include "Engine/DataTable.h"
#include "GameStructs_System.h"
#include "Power.h"
#include "Drive.h"
#include "ShipDesignRegistry.h"
#include "WeaponDesignRegistry.h"
#include "Misc/ScopeExit.h"
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
    if (TestEqual(TEXT("Two Falcon exhaust nozzles"),F->Drive[0].Ports.Num(),2))
    {
        TestTrue(TEXT("Port exhaust matches hull nozzle"),F->Drive[0].Ports[0].Location.Equals(FVector(-47.f,9.f,3.f),0.001));
        TestTrue(TEXT("Starboard exhaust matches hull nozzle"),F->Drive[0].Ports[1].Location.Equals(FVector(-47.f,-9.f,3.f),0.001));
    }
    if (TestTrue(TEXT("Falcon RCS ports exist"),!F->Thruster.IsEmpty() && !F->Thruster[0].Ports.IsEmpty()))
        TestTrue(TEXT("RCS shares the engine port scale convention"),F->Thruster[0].Ports[0].Location.Equals(FVector(0.f,-8.f,3.f),0.001));
    const auto Original=ShipDesignRegistry::GetAll();
    const auto OriginalWeapons=WeaponDesignRegistry::GetAll();
    ON_SCOPE_EXIT {ShipDesignRegistry::Clear();WeaponDesignRegistry::Clear();for(const auto& P:OriginalWeapons)WeaponDesignRegistry::RegisterDesign(P.Key,P.Value);for(const auto& P:Original)ShipDesignRegistry::RegisterDesign(P.Key,P.Value);};
    auto* Weapons=LoadObject<UDataTable>(nullptr,TEXT("/Game/Game/DT_WeaponDesign.DT_WeaponDesign"));
    if(!TestNotNull(TEXT("Weapons table"),Weapons))return false;
    for(const auto& P:Weapons->GetRowMap())WeaponDesignRegistry::RegisterDesign(P.Key,*reinterpret_cast<const FWeaponDesign*>(P.Value));
    // Reproduce a definition with an unnamed nav block. A row count alone
    // missed the runtime conversion dropping this valid component.
    FShipDesign Unnamed=*F;
    for(auto& N:Unnamed.NavSys)N.DesignName.Empty();
    ShipDesignRegistry::RegisterDesign(FName(TEXT("Falcon")),Unnamed);
    const auto* Runtime=ShipDesignRegistry::FindLegacy(FName(TEXT("Falcon")));
    if(!TestNotNull(TEXT("Runtime Falcon design"),Runtime))return false;
    TestTrue(TEXT("Unnamed navigation is created at runtime"),Runtime->navsystems.size()>0);
    TestNotNull(TEXT("Runtime navigation pointer"),Runtime->navsys);
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
