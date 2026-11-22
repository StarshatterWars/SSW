#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "Weapon.h"
#include "WeaponDesign.h"
#include "Physical.h"
#include "Camera.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FWeaponMountBasisTest,"Starshatter.Weapons.NativeMountBasis",
    EAutomationTestFlags_ApplicationContextMask|EAutomationTestFlags::EngineFilter)
bool FWeaponMountBasisTest::RunTest(const FString&)
{
    class FMountProbe : public Weapon
    {
    public:
        FMountProbe(WeaponDesign* Definition,FVector* Offset):Weapon(Definition,1,Offset){}
        FVector Muzzle()const{return muzzle_pts[0];}
    };
    WeaponDesign Definition; Definition.nbarrels=0; Definition.nstores=0;
    FVector Offset(13,21,34);
    FMountProbe Mount(&Definition,&Offset);
    Physical Hull; Hull.MoveTo(FVector(100,200,300));
    Mount.Orient(&Hull);
    TestTrue(TEXT("Neutral hull preserves positive up-offset"),Mount.Muzzle().Equals(Hull.GetLocation()+Offset,1.e-5));
    Camera Rotated; Rotated.Yaw(.71); Rotated.Pitch(-.23); Rotated.Roll(.37);
    Hull.CloneCam(Rotated); Hull.MoveTo(FVector(100,200,300));
    Mount.Orient(&Hull);
    const auto& Frame=Hull.GetCam();
    const FVector Expected=Hull.GetLocation()+Frame.vrt()*Offset.X+Frame.vup()*Offset.Y+Frame.vpn()*Offset.Z;
    TestTrue(TEXT("Rotated muzzle follows the hull camera basis without reflection"),Mount.Muzzle().Equals(Expected,1.e-5));
    return true;
}
#endif
