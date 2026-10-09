// Model defaults derived from Asher.def.
#include "AsherShipActor.h"
#include "Components/SceneComponent.h"

AAsherShipActor::AAsherShipActor()
{
    // Native default only: preserve BP and placed-instance scale during construction.
    SetActorScale3D(FVector(2.5f));
    ApplyAsherDefaults();
}

void AAsherShipActor::OnConstruction(const FTransform& Transform)
{
    ApplyAsherDefaults();
    Super::OnConstruction(Transform);
    ApplyAsherFixedPoints();
}

void AAsherShipActor::ApplyAsherDefaults()
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
    BridgePointOffset = FVector(320.0f, 0.0f, 60.0f);
    ChasePointOffset = FVector(-1800.0f, 0.0f, 170.0f);
    FocusPointOffset = FVector(320.0f, 0.0f, 60.0f);
    DriveCenterPointOffset = FVector(-480.0f, 0.0f, -20.0f);
    QuantumPointOffset = FVector(0.0f, 0.0f, 0.0f);
    ShieldPointOffset = FVector(0.0f, 0.0f, -20.0f);
    SensorPointOffset = FVector(380.0f, 0.0f, -16.0f);
    NavPointOffset = FVector(60.0f, 0.0f, 16.0f);
    ReactorPointOffset = FVector(-220.0f, 0.0f, -20.0f);
    ComputerPointAOffset = FVector(80.0f, 20.0f, 16.0f);
    ComputerPointBOffset = FVector(80.0f, -20.0f, -16.0f);


    WeaponMountPointDefs.Empty();
    NumWeaponMountPoints = 8;
    {
        FShipPointDef Point;
        Point.PointName = TEXT("XRay Laser 1_1");
        Point.LocalOffset = FVector(425.0f, -22.0f, -9.0f);
        Point.LocalRotation = FRotator(0.0f, 0.0f, 0.0f);
        WeaponMountPointDefs.Add(Point);
    }
    {
        FShipPointDef Point;
        Point.PointName = TEXT("XRay Laser 2_2");
        Point.LocalOffset = FVector(425.0f, 0.0f, -22.0f);
        Point.LocalRotation = FRotator(0.0f, 0.0f, 0.0f);
        WeaponMountPointDefs.Add(Point);
    }
    {
        FShipPointDef Point;
        Point.PointName = TEXT("XRay Laser 3_3");
        Point.LocalOffset = FVector(425.0f, 22.0f, -9.0f);
        Point.LocalRotation = FRotator(0.0f, 0.0f, 0.0f);
        WeaponMountPointDefs.Add(Point);
    }
    {
        FShipPointDef Point;
        Point.PointName = TEXT("Starboard Torpedo_4");
        Point.LocalOffset = FVector(60.0f, 106.0f, -12.0f);
        Point.LocalRotation = FRotator(0.0f, 0.0f, 0.0f);
        WeaponMountPointDefs.Add(Point);
    }
    {
        FShipPointDef Point;
        Point.PointName = TEXT("Port Torpedo_5");
        Point.LocalOffset = FVector(60.0f, -106.0f, -12.0f);
        Point.LocalRotation = FRotator(0.0f, 0.0f, 0.0f);
        WeaponMountPointDefs.Add(Point);
    }
    {
        FShipPointDef Point;
        Point.PointName = TEXT("Port Phalanx_6");
        Point.LocalOffset = FVector(-128.0f, -110.0f, 15.0f);
        Point.LocalRotation = FRotator(0.0f, -90.0f, 0.0f);
        WeaponMountPointDefs.Add(Point);
    }
    {
        FShipPointDef Point;
        Point.PointName = TEXT("Starboard Phalanx_7");
        Point.LocalOffset = FVector(-128.0f, 110.0f, 15.0f);
        Point.LocalRotation = FRotator(0.0f, 90.0f, 0.0f);
        WeaponMountPointDefs.Add(Point);
    }
    {
        FShipPointDef Point;
        Point.PointName = TEXT("Aft Phalanx_8");
        Point.LocalOffset = FVector(-190.0f, 0.0f, 56.0f);
        Point.LocalRotation = FRotator(0.0f, 180.0f, 0.0f);
        WeaponMountPointDefs.Add(Point);
    }
    TurretBasePointDefs.Empty();
    NumTurretBasePoints = 3;
    {
        FShipPointDef Point;
        Point.PointName = TEXT("Port Phalanx_6_Base");
        Point.LocalOffset = FVector(-128.0f, -110.0f, 15.0f);
        Point.LocalRotation = FRotator(0.0f, -90.0f, 0.0f);
        TurretBasePointDefs.Add(Point);
    }
    {
        FShipPointDef Point;
        Point.PointName = TEXT("Starboard Phalanx_7_Base");
        Point.LocalOffset = FVector(-128.0f, 110.0f, 15.0f);
        Point.LocalRotation = FRotator(0.0f, 90.0f, 0.0f);
        TurretBasePointDefs.Add(Point);
    }
    {
        FShipPointDef Point;
        Point.PointName = TEXT("Aft Phalanx_8_Base");
        Point.LocalOffset = FVector(-190.0f, 0.0f, 56.0f);
        Point.LocalRotation = FRotator(0.0f, 180.0f, 0.0f);
        TurretBasePointDefs.Add(Point);
    }
    DockPointDefs.Empty();
    NumDockPoints = 0;
    LandingPointDefs.Empty();
    NumLandingPoints = 0;
}

void AAsherShipActor::ApplyAsherFixedPoints()
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
