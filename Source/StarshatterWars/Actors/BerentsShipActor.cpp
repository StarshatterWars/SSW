// Model defaults derived from Berents.def.
#include "BerentsShipActor.h"
#include "Components/SceneComponent.h"

ABerentsShipActor::ABerentsShipActor()
{
    ApplyBerentsDefaults();
}

void ABerentsShipActor::OnConstruction(const FTransform& Transform)
{
    ApplyBerentsDefaults();
    Super::OnConstruction(Transform);
    ApplyBerentsFixedPoints();
}

void ABerentsShipActor::ApplyBerentsDefaults()
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
    BridgePointOffset = FVector(216.0f, 0.0f, 34.0f);
    ChasePointOffset = FVector(-1000.0f, 0.0f, 200.0f);
    FocusPointOffset = FVector(216.0f, 0.0f, 34.0f);
    DriveCenterPointOffset = FVector(-100.0f, 0.0f, 0.0f);
    QuantumPointOffset = FVector(-120.0f, 0.0f, 0.0f);
    ShieldPointOffset = FVector(-80.0f, 0.0f, 20.0f);
    SensorPointOffset = FVector(180.0f, 0.0f, 0.0f);
    NavPointOffset = FVector(60.0f, 0.0f, 16.0f);
    ReactorPointOffset = FVector(-60.0f, 0.0f, 0.0f);
    ComputerPointAOffset = FVector(80.0f, 20.0f, 16.0f);
    ComputerPointBOffset = FVector(80.0f, -20.0f, -16.0f);
    SetActorScale3D(FVector(2.5f));

    WeaponMountPointDefs.Empty();
    NumWeaponMountPoints = 6;
    {
        FShipPointDef Point;
        Point.PointName = TEXT("Plasma Cannon_1");
        Point.LocalOffset = FVector(333.0f, 0.0f, 4.0f);
        Point.LocalRotation = FRotator(0.0f, 0.0f, 0.0f);
        WeaponMountPointDefs.Add(Point);
    }
    {
        FShipPointDef Point;
        Point.PointName = TEXT("Interceptor 1_2");
        Point.LocalOffset = FVector(233.0f, -33.0f, 33.0f);
        Point.LocalRotation = FRotator(0.0f, -0.15f, 0.0f);
        WeaponMountPointDefs.Add(Point);
    }
    {
        FShipPointDef Point;
        Point.PointName = TEXT("Interceptor 2_3");
        Point.LocalOffset = FVector(233.0f, 33.0f, 33.0f);
        Point.LocalRotation = FRotator(0.0f, 0.15f, 0.0f);
        WeaponMountPointDefs.Add(Point);
    }
    {
        FShipPointDef Point;
        Point.PointName = TEXT("Fwd Cannon_4");
        Point.LocalOffset = FVector(-22.0f, 88.0f, 36.0f);
        Point.LocalRotation = FRotator(0.0f, 0.0f, 0.0f);
        WeaponMountPointDefs.Add(Point);
    }
    {
        FShipPointDef Point;
        Point.PointName = TEXT("Fwd Cannon_5");
        Point.LocalOffset = FVector(-22.0f, -88.0f, 36.0f);
        Point.LocalRotation = FRotator(0.0f, 0.0f, 0.0f);
        WeaponMountPointDefs.Add(Point);
    }
    {
        FShipPointDef Point;
        Point.PointName = TEXT("Aft Cannon_6");
        Point.LocalOffset = FVector(-190.0f, 0.0f, -26.0f);
        Point.LocalRotation = FRotator(0.0f, 3.141593f, 0.0f);
        WeaponMountPointDefs.Add(Point);
    }
    TurretBasePointDefs.Empty();
    NumTurretBasePoints = 3;
    {
        FShipPointDef Point;
        Point.PointName = TEXT("Interceptor 1_2_Base");
        Point.LocalOffset = FVector(220.0f, -33.0f, 0.0f);
        Point.LocalRotation = FRotator(0.0f, -0.15f, 0.0f);
        TurretBasePointDefs.Add(Point);
    }
    {
        FShipPointDef Point;
        Point.PointName = TEXT("Interceptor 2_3_Base");
        Point.LocalOffset = FVector(220.0f, 33.0f, 0.0f);
        Point.LocalRotation = FRotator(0.0f, 0.15f, 0.0f);
        TurretBasePointDefs.Add(Point);
    }
    {
        FShipPointDef Point;
        Point.PointName = TEXT("Aft Cannon_6_Base");
        Point.LocalOffset = FVector(-190.0f, 0.0f, -26.0f);
        Point.LocalRotation = FRotator(0.0f, 3.141593f, 0.0f);
        TurretBasePointDefs.Add(Point);
    }
    DockPointDefs.Empty();
    NumDockPoints = 0;
    LandingPointDefs.Empty();
    NumLandingPoints = 0;
}

void ABerentsShipActor::ApplyBerentsFixedPoints()
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
