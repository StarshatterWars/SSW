#if WITH_DEV_AUTOMATION_TESTS && WITH_EDITOR
#include "Misc/AutomationTest.h"
#include "RaptorShipActor.h"
#include "ShipWeaponMountComponent.h"
#include "TurretActor.h"
#include "Weapon.h"
#include "WeaponDesign.h"
#include "WeaponPresentationFrame.h"
#include "Engine/World.h"
class FAuditAimWeapon : public Weapon
{
public:
    FAuditAimWeapon(WeaponDesign* Design,FVector* Muzzles):Weapon(Design,2,Muzzles,0,0) {}
    Camera& AimFrame() { return aim_cam; }
    void SetOldAzimuth(float V) { old_azimuth=V; }
};
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTurretAimBridgeTest,"Starshatter.Audit.F02.TurretAim", EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FTurretAimBridgeTest::RunTest(const FString& Parameters)
{
    const UWorld::InitializationValues Options=UWorld::InitializationValues().AllowAudioPlayback(false).CreatePhysicsScene(false).CreateNavigation(false).CreateAISystem(false).ShouldSimulatePhysics(false);
    UWorld* World=UWorld::CreateWorld(EWorldType::EditorPreview,false,NAME_None,nullptr,true,ERHIFeatureLevel::Num,&Options);
    auto* Actor=World->SpawnActor<ARaptorShipActor>();
    Actor->SetActorRotation(FRotator(15,130,-8));
    auto* Mount=CastChecked<UShipWeaponMountComponent>(Actor->HullWeaponMounts[0].Get());
    const FQuat Rest=Mount->GetRelativeRotation().Quaternion();
    WeaponDesign Design; Design.turret="audit"; Design.self_aiming=true; Design.turret_axis=0;
    FVector Muzzles[2]={FVector(7,0,60),FVector(-7,0,60)};
    FAuditAimWeapon Weapon(&Design,Muzzles);
    Camera SimShip; SimShip.Yaw(0.3);SimShip.Pitch(-0.2);
    Weapon.AimFrame().Clone(SimShip);Weapon.AimFrame().Yaw(0.7);Weapon.AimFrame().Pitch(0.25);
    Weapon.SetAzimuth(0.7);Weapon.SetElevation(0.25);Weapon.SetOldAzimuth(0.15f);
    TestTrue(TEXT("Base offset is stored old angle, not aim counted twice"), FMath::IsNearlyEqual(double(Weapon.GetTurretAzimuthOffset()),0.15,0.000001));
    Mount->BindRuntimeWeapon(&Weapon,TEXT("Audit"),TEXT("Audit"));
    Mount->UpdateRuntimeAim(SimShip,Actor->GetActorQuat());
    const FQuat Expected=WeaponPresentationFrame::Display(Weapon.AimFrame(),SimShip,Actor->GetActorQuat());
    TestTrue(TEXT("Native weapon tracks simulated aim under a differently oriented display ship"),Mount->GetComponentQuat().Equals(Expected,0.00001));
    TestTrue(TEXT("Native barrels inherit aim"),Mount->BarrelComponents[0]->GetComponentQuat().Equals(Expected,0.00001));
    Mount->SetChildActorClass(ATurretActor::StaticClass());
    auto* Visual=CastChecked<ATurretActor>(Mount->GetChildActor());
    Visual->SyncFromWeaponFrame(Weapon,SimShip,Actor->GetActorQuat());
    TestTrue(TEXT("Turret child does not apply aim twice"),Visual->GunPivot->GetComponentQuat().Equals(Expected,0.00001));
    Camera Base;Base.Clone(SimShip);Base.Yaw(0.85);
    TestTrue(TEXT("Base follows only its yaw axis"),Visual->BasePivot->GetComponentQuat().Equals(WeaponPresentationFrame::Display(Base,SimShip,Actor->GetActorQuat()),0.00001));
    const int32 BarrelCount=Mount->BarrelComponents.Num();
    Mount->UpdateRuntimeAim(SimShip,Actor->GetActorQuat());
    TestEqual(TEXT("Aim updates create no extra barrels"),Mount->BarrelComponents.Num(),BarrelCount);
    Mount->ClearRuntimeWeapon();
    TestTrue(TEXT("Unbind restores authored rotation"),Mount->GetRelativeRotation().Quaternion().Equals(Rest,0.00001));
    World->DestroyWorld(false);
    return true;
}
#endif
