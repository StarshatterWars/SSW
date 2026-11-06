#if WITH_DEV_AUTOMATION_TESTS && WITH_EDITOR
#include "Misc/AutomationTest.h"
#include "Misc/ScopeExit.h"
#include "Engine/GameInstance.h"
#include "StarshatterWeaponDesignSubsystem.h"
#include "WeaponDesignRegistry.h"
#include "WeaponDesign.h"
#include "SimShot.h"
#include "Ship.h"
#include "ShipDesign.h"
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMasslessBoltParityTest,"Starshatter.Weapons.BoltFlightParity",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FMasslessBoltParityTest::RunTest(const FString&)
{
 const auto Before=WeaponDesignRegistry::GetAll();ON_SCOPE_EXIT{WeaponDesignRegistry::Clear();for(const auto& P:Before)WeaponDesignRegistry::RegisterDesign(P.Key,P.Value);};
 auto* Loader=NewObject<UStarshatterWeaponDesignSubsystem>(NewObject<UGameInstance>());const FString File=FPaths::ProjectContentDir()/TEXT("GameData/Weapons/Wep.def");const auto Ansi=StringCast<ANSICHAR>(*File);Loader->LoadWeaponDesign(Ansi.Get());
 TSet<FName> Seen;int Bolts=0,Turrets=0;
 for(const auto& P:WeaponDesignRegistry::GetAll()){
  const auto& R=P.Value;const FName Name(R.Name);if(Seen.Contains(Name))continue;Seen.Add(Name);auto* D=WeaponDesignRegistry::FindLegacy(Name);if(!TestNotNull(*R.Name,D))continue;
  if(!R.Turret.IsEmpty()){
   ++Turrets;TestEqual(*(R.Name+TEXT(" model scale preserved")),D->scale,R.Scale);TestEqual(*(R.Name+TEXT(" barrel count preserved")),D->nbarrels,FMath::Min(R.NBarrels,int32(WeaponDesign::MAX_STORES)));TestEqual(TEXT("Muzzle copy does not overwrite aim limits"),D->aim_az_max,R.AimAzMax);
   for(int I=0;I<R.NBarrels;++I)TestTrue(TEXT("Unscaled definition muzzle retained"),D->muzzle_pts[I].Equals(R.MuzzlePoints[I]));
  }
  if(R.GraphicType!=3||R.bBeam)continue;++Bolts;
  Camera Aim;Aim.Yaw(.4);Aim.Pitch(-.2);const FVector Start(100,200,300);SimShot S(Start,Aim,D,nullptr);S.SetCharge(D->charge);
  S.ExecFrame(0);const double Life=S.GetLife();S.ExecFrame(.02);
  TestTrue(*(R.Name+TEXT(" travels at legacy speed")),S.GetLocation().Equals(Start+Aim.vpn()*double(D->speed)*.02,.01));
  TestTrue(*(R.Name+TEXT(" lifetime advances")),FMath::IsNearlyEqual(S.GetLife(),Life-.02,.00001));
  TestTrue(*(R.Name+TEXT(" damage remains positive")),S.Damage()>0);TestEqual(TEXT("Definition width retained"),D->width,R.Width);
 }
 TestEqual(TEXT("All bolt entries tested"),Bolts,26);TestEqual(TEXT("All turret entries tested"),Turrets,26);return true;
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FSweptBoltDamageTest, "Starshatter.Weapons.SweptBoltDamage",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FSweptBoltDamageTest::RunTest(const FString&)
{
    class FTargetShip : public Ship
    {
    public:
        explicit FTargetShip(ShipDesign* Design) : Ship("Bolt target", "", Design, 2, 0, nullptr, false)
        {
            radius = 12;
            SetFlightPhase(EOPSMode::ACTIVE);
        }
    };
    ShipDesign TargetDesign;
    TargetDesign.integrity = 1000;
    TargetDesign.mass = 5;
    TargetDesign.type = static_cast<int>(CLASSIFICATION::DRONE);
    FTargetShip Victim(&TargetDesign);

    WeaponDesign Bolt;
    Bolt.primary = true;
    Bolt.graphic_type = Graphic::BOLT;
    Bolt.speed = 8000;
    Bolt.life = 1.6f;
    Bolt.damage = 20;
    Bolt.charge = 5;

    Camera Aim;
    const FVector Direction = Aim.vpn();
    const FVector Center = FVector(1000, 500, 200);
    Victim.MoveTo(Center);
    const double Before = Victim.GetIntegrity();
    SimShot Crossing(Center - Direction * 100, Aim, &Bolt, nullptr);
    Crossing.SetCharge(Bolt.charge);
    Crossing.ExecFrame(0);
    Crossing.ExecFrame(0.025);
    FVector Impact;
    TestTrue(TEXT("Bolt crossing a small hull with both endpoints outside hits"), Victim.HitBy(&Crossing, Impact) != 0);
    TestTrue(TEXT("A hit reduces actual hull integrity"), Victim.GetIntegrity() < Before);

    const double AfterHit = Victim.GetIntegrity();
    SimShot Short(Center - Direction * 28, Aim, &Bolt, nullptr);
    Short.SetCharge(Bolt.charge);
    Short.ExecFrame(0);
    Short.ExecFrame(0.001);
    TestEqual(TEXT("A bolt short of the hull does not hit early"), Victim.HitBy(&Short, Impact), 0);
    TestEqual(TEXT("A near miss leaves hull integrity unchanged"), Victim.GetIntegrity(), AfterHit);
    return true;
}
#endif
