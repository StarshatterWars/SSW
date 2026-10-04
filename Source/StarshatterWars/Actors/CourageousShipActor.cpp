// Model defaults derived from Courageous.def.
#include "CourageousShipActor.h"
#include "Components/SceneComponent.h"

ACourageousShipActor::ACourageousShipActor()
{
    ApplyCourageousDefaults();
}

void ACourageousShipActor::OnConstruction(const FTransform& Transform)
{
    ApplyCourageousDefaults();
    Super::OnConstruction(Transform);
    ApplyCourageousFixedPoints();
}

void ACourageousShipActor::ApplyCourageousDefaults()
{
    bAutoRebuildGeneratedComponents = true;
    bRebuildOnConstruction = true;
    bEnableMainEngineEmitters = true;
    bEnableThrusterEmitters = true;
    MainEngineEmitterRelativeScale = FVector(1.15f, 0.5f, 0.5f);
    MainEngineEmitterRelativeRotation = FRotator(0.0f, 180.0f, 0.0f);

    // Runtime Ship systems own these emitters; do not duplicate fallback ports.
    NumMainEnginePoints = 0;
    NumThrusterPoints = 0;
    MainEnginePointDefs.Empty();
    ThrusterPointDefs.Empty();
    NavLightDefs.Empty();

    // Camera definitions: (right, forward, up) -> UE (forward, right, up).
    // Subsystem/model points: (right, up, forward) -> UE (forward, right, up).
    BridgePointOffset = FVector(520.0f, 0.0f, 120.0f);
    ChasePointOffset = FVector(-1300.0f, 0.0f, 300.0f);
    FocusPointOffset = FVector(520.0f, 0.0f, 120.0f);
    DriveCenterPointOffset = FVector(-320.0f, 0.0f, 0.0f);
    QuantumPointOffset = FVector(0.0f, 0.0f, 0.0f);
    ShieldPointOffset = FVector(40.0f, 0.0f, 0.0f);
    SensorPointOffset = FVector(380.0f, 0.0f, 32.0f);
    NavPointOffset = FVector(60.0f, 0.0f, 32.0f);
    ReactorPointOffset = FVector(-92.0f, 0.0f, 0.0f);
    ComputerPointAOffset = FVector(80.0f, 20.0f, -32.0f);
    ComputerPointBOffset = FVector(80.0f, -20.0f, -32.0f);
    SetActorScale3D(FVector(2.2f));

    WeaponMountPointDefs.Empty();
    NumWeaponMountPoints = 13;
    {
        FShipPointDef Point;
        Point.PointName = TEXT("XRay Laser 1_1");
        Point.LocalOffset = FVector(641.0f, -51.0f, -12.0f);
        Point.LocalRotation = FRotator(0.0f, 0.0f, 0.0f);
        WeaponMountPointDefs.Add(Point);
    }
    {
        FShipPointDef Point;
        Point.PointName = TEXT("XRay Laser 2_2");
        Point.LocalOffset = FVector(641.0f, 51.0f, -12.0f);
        Point.LocalRotation = FRotator(0.0f, 0.0f, 0.0f);
        WeaponMountPointDefs.Add(Point);
    }
    {
        FShipPointDef Point;
        Point.PointName = TEXT("XRay Laser 3_3");
        Point.LocalOffset = FVector(623.0f, -37.0f, -45.0f);
        Point.LocalRotation = FRotator(0.0f, 0.0f, 0.0f);
        WeaponMountPointDefs.Add(Point);
    }
    {
        FShipPointDef Point;
        Point.PointName = TEXT("XRay Laser 4_4");
        Point.LocalOffset = FVector(623.0f, 37.0f, -45.0f);
        Point.LocalRotation = FRotator(0.0f, 0.0f, 0.0f);
        WeaponMountPointDefs.Add(Point);
    }
    {
        FShipPointDef Point;
        Point.PointName = TEXT("Starboard Missile_5");
        Point.LocalOffset = FVector(355.0f, 70.0f, 55.0f);
        Point.LocalRotation = FRotator(0.0f, 15.0f, 0.0f);
        WeaponMountPointDefs.Add(Point);
    }
    {
        FShipPointDef Point;
        Point.PointName = TEXT("Port Missile_6");
        Point.LocalOffset = FVector(355.0f, -70.0f, 55.0f);
        Point.LocalRotation = FRotator(0.0f, -15.0f, 0.0f);
        WeaponMountPointDefs.Add(Point);
    }
    {
        FShipPointDef Point;
        Point.PointName = TEXT("Interceptor_7");
        Point.LocalOffset = FVector(355.0f, 35.0f, 75.0f);
        Point.LocalRotation = FRotator(0.0f, 0.0f, 0.0f);
        WeaponMountPointDefs.Add(Point);
    }
    {
        FShipPointDef Point;
        Point.PointName = TEXT("Gun-1_8");
        Point.LocalOffset = FVector(400.0f, 0.0f, 87.0f);
        Point.LocalRotation = FRotator(0.0f, 0.0f, 0.0f);
        WeaponMountPointDefs.Add(Point);
    }
    {
        FShipPointDef Point;
        Point.PointName = TEXT("Gun-2_9");
        Point.LocalOffset = FVector(-15.0f, 0.0f, 130.0f);
        Point.LocalRotation = FRotator(0.0f, 180.0f, 0.0f);
        WeaponMountPointDefs.Add(Point);
    }
    {
        FShipPointDef Point;
        Point.PointName = TEXT("Port Phalanx 1_10");
        Point.LocalOffset = FVector(-250.0f, -176.0f, 78.0f);
        Point.LocalRotation = FRotator(0.0f, -90.0f, 0.0f);
        WeaponMountPointDefs.Add(Point);
    }
    {
        FShipPointDef Point;
        Point.PointName = TEXT("Stbd Phalanx 1_11");
        Point.LocalOffset = FVector(-250.0f, 176.0f, 78.0f);
        Point.LocalRotation = FRotator(0.0f, 90.0f, 0.0f);
        WeaponMountPointDefs.Add(Point);
    }
    {
        FShipPointDef Point;
        Point.PointName = TEXT("Port Phalanx 2_12");
        Point.LocalOffset = FVector(-220.0f, -176.0f, 18.0f);
        Point.LocalRotation = FRotator(0.0f, -90.0f, 0.0f);
        WeaponMountPointDefs.Add(Point);
    }
    {
        FShipPointDef Point;
        Point.PointName = TEXT("Stbd Phalanx 2_13");
        Point.LocalOffset = FVector(-220.0f, 176.0f, 18.0f);
        Point.LocalRotation = FRotator(0.0f, 90.0f, 0.0f);
        WeaponMountPointDefs.Add(Point);
    }
    TurretBasePointDefs.Empty();
    NumTurretBasePoints = 7;
    {
        FShipPointDef Point;
        Point.PointName = TEXT("Starboard Missile_5_Base");
        Point.LocalOffset = FVector(250.0f, 100.0f, 22.0f);
        Point.LocalRotation = FRotator(0.0f, 15.0f, 0.0f);
        TurretBasePointDefs.Add(Point);
    }
    {
        FShipPointDef Point;
        Point.PointName = TEXT("Port Missile_6_Base");
        Point.LocalOffset = FVector(250.0f, -100.0f, 22.0f);
        Point.LocalRotation = FRotator(0.0f, -15.0f, 0.0f);
        TurretBasePointDefs.Add(Point);
    }
    {
        FShipPointDef Point;
        Point.PointName = TEXT("Gun-2_9_Base");
        Point.LocalOffset = FVector(-15.0f, 0.0f, 130.0f);
        Point.LocalRotation = FRotator(0.0f, 180.0f, 0.0f);
        TurretBasePointDefs.Add(Point);
    }
    {
        FShipPointDef Point;
        Point.PointName = TEXT("Port Phalanx 1_10_Base");
        Point.LocalOffset = FVector(-250.0f, -176.0f, 78.0f);
        Point.LocalRotation = FRotator(0.0f, -90.0f, 0.0f);
        TurretBasePointDefs.Add(Point);
    }
    {
        FShipPointDef Point;
        Point.PointName = TEXT("Stbd Phalanx 1_11_Base");
        Point.LocalOffset = FVector(-250.0f, 176.0f, 78.0f);
        Point.LocalRotation = FRotator(0.0f, 90.0f, 0.0f);
        TurretBasePointDefs.Add(Point);
    }
    {
        FShipPointDef Point;
        Point.PointName = TEXT("Port Phalanx 2_12_Base");
        Point.LocalOffset = FVector(-220.0f, -176.0f, 18.0f);
        Point.LocalRotation = FRotator(0.0f, -90.0f, 0.0f);
        TurretBasePointDefs.Add(Point);
    }
    {
        FShipPointDef Point;
        Point.PointName = TEXT("Stbd Phalanx 2_13_Base");
        Point.LocalOffset = FVector(-220.0f, 176.0f, 18.0f);
        Point.LocalRotation = FRotator(0.0f, 90.0f, 0.0f);
        TurretBasePointDefs.Add(Point);
    }
    DockPointDefs.Empty();
    NumDockPoints = 0;
    LandingPointDefs.Empty();
    NumLandingPoints = 0;
}

void ACourageousShipActor::ApplyCourageousFixedPoints()
{
    if (BridgePoint) BridgePoint->SetRelativeLocation(BridgePointOffset);
    if (ChasePoint) ChasePoint->SetRelativeLocation(ChasePointOffset);
    if (FocusPoint) FocusPoint->SetRelativeLocation(FocusPointOffset);
    if (DriveCenterPoint) DriveCenterPoint->SetRelativeLocation(DriveCenterPointOffset);
    if (QuantumPoint) QuantumPoint->SetRelativeLocation(QuantumPointOffset);
    if (ShieldPoint) ShieldPoint->SetRelativeLocation(ShieldPointOffset);
    if (SensorPoint) SensorPoint->SetRelativeLocation(SensorPointOffset);
    if (NavPoint) NavPoint->SetRelativeLocation(NavPointOffset);
    if (ReactorPoint) ReactorPoint->SetRelativeLocation(ReactorPointOffset);
    if (ComputerPointA) ComputerPointA->SetRelativeLocation(ComputerPointAOffset);
    if (ComputerPointB) ComputerPointB->SetRelativeLocation(ComputerPointBOffset);
}
