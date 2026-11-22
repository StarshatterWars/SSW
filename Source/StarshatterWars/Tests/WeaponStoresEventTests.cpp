#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "WeaponGroup.h"
#include "WeaponDesign.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FWeaponStoresEventTest,"Starshatter.Weapons.ChangeDrivenStoreTotals",
    EAutomationTestFlags_ApplicationContextMask|EAutomationTestFlags::EngineFilter)
bool FWeaponStoresEventTest::RunTest(const FString&)
{
    class FIdleWeapon : public Weapon
    {
    public:
        explicit FIdleWeapon(WeaponDesign* Design):Weapon(Design,1,nullptr){}
        virtual void ExecFrame(double) override{++Frames;}
        int32 Frames=0;
    };
    WeaponDesign Definition;Definition.ammo=5;Definition.nbarrels=0;Definition.nstores=0;
    Definition.carry_mass=2;Definition.carry_resist=.5f;
    WeaponGroup Group("Store event fixture");
    auto* First=new FIdleWeapon(&Definition);auto* Second=new FIdleWeapon(&Definition);
    Group.AddWeapon(First);Group.AddWeapon(Second);
    TestEqual(TEXT("Loadout initializes totals without a frame tick"),Group.Ammo(),10);
    const auto Before=Group.GetStoresRebuildCount();
    for(int32 Frame=0;Frame<120;++Frame){Group.ExecFrame(1.0/60);Group.Ammo();Group.Mass();Group.Resistance();}
    TestEqual(TEXT("Idle frame reads do not rescan the group"),Group.GetStoresRebuildCount(),Before);
    TestEqual(TEXT("Active weapon updates still run"),First->Frames,120);
    First->SetAmmo(2);
    TestEqual(TEXT("Ammo change is visible immediately"),Group.Ammo(),7);
    TestEqual(TEXT("Remaining stores drive carried mass"),Group.Mass(),14.f);
    TestEqual(TEXT("Remaining stores drive resistance"),Group.Resistance(),3.5f);
    TestEqual(TEXT("One event coalesces three queries into one rebuild"),Group.GetStoresRebuildCount(),Before+1);
    First->SetAmmo(2);Group.Ammo();
    TestEqual(TEXT("Unchanged ammo does not invalidate"),Group.GetStoresRebuildCount(),Before+1);
    Second->SetAmmo(0);
    TestEqual(TEXT("Exhausted launcher updates the group"),Group.Ammo(),2);
    return true;
}
#endif
