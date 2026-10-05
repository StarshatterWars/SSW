// Model defaults derived from Devastator.def.
#include "DevastatorShipActor.h"
#include "Components/SceneComponent.h"

ADevastatorShipActor::ADevastatorShipActor()
{
    ApplyDevastatorDefaults();
}

void ADevastatorShipActor::OnConstruction(const FTransform& Transform)
{
    ApplyDevastatorDefaults();
    Super::OnConstruction(Transform);
    ApplyDevastatorFixedPoints();
}

void ADevastatorShipActor::ApplyDevastatorDefaults()
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
    BridgePointOffset = FVector(520.0f, 0.0f, 50.0f);
    ChasePointOffset = FVector(-1300.0f, 0.0f, 300.0f);
    FocusPointOffset = FVector(520.0f, 0.0f, 50.0f);
    DriveCenterPointOffset = FVector(-320.0f, 0.0f, 0.0f);
    QuantumPointOffset = FVector(-200.0f, 0.0f, 0.0f);
    ShieldPointOffset = FVector(-60.0f, 0.0f, 0.0f);
    SensorPointOffset = FVector(380.0f, 0.0f, 32.0f);
    NavPointOffset = FVector(60.0f, 0.0f, 32.0f);
    ReactorPointOffset = FVector(-92.0f, 0.0f, 0.0f);
    ComputerPointAOffset = FVector(80.0f, 20.0f, -32.0f);
    ComputerPointBOffset = FVector(80.0f, -20.0f, -32.0f);
    SetActorScale3D(FVector(2.5f));

    WeaponMountPointDefs.Empty();
    NumWeaponMountPoints = 10;
    {
        FShipPointDef Point;
        Point.PointName = TEXT("Gamma Laser 1_1");
        Point.LocalOffset = FVector(760.0f, -65.0f, -22.0f);
        Point.LocalRotation = FRotator(0.0f, 0.0f, 0.0f);
        WeaponMountPointDefs.Add(Point);
    }
    {
        FShipPointDef Point;
        Point.PointName = TEXT("Gamma Laser 2_2");
        Point.LocalOffset = FVector(760.0f, 65.0f, -22.0f);
        Point.LocalRotation = FRotator(0.0f, 0.0f, 0.0f);
        WeaponMountPointDefs.Add(Point);
    }
    {
        FShipPointDef Point;
        Point.PointName = TEXT("Gamma Laser 33_3");
        Point.LocalOffset = FVector(740.0f, -52.0f, -54.0f);
        Point.LocalRotation = FRotator(0.0f, 0.0f, 0.0f);
        WeaponMountPointDefs.Add(Point);
    }
    {
        FShipPointDef Point;
        Point.PointName = TEXT("Gamma Laser 4_4");
        Point.LocalOffset = FVector(740.0f, 52.0f, -54.0f);
        Point.LocalRotation = FRotator(0.0f, 0.0f, 0.0f);
        WeaponMountPointDefs.Add(Point);
    }
    {
        FShipPointDef Point;
        Point.PointName = TEXT("Starboard Missile_5");
        Point.LocalOffset = FVector(600.0f, 75.0f, 13.0f);
        Point.LocalRotation = FRotator(0.0f, 15.0f, 0.0f);
        WeaponMountPointDefs.Add(Point);
    }
    {
        FShipPointDef Point;
        Point.PointName = TEXT("Port Missile_6");
        Point.LocalOffset = FVector(600.0f, -75.0f, 13.0f);
        Point.LocalRotation = FRotator(0.0f, -15.0f, 0.0f);
        WeaponMountPointDefs.Add(Point);
    }
    {
        FShipPointDef Point;
        Point.PointName = TEXT("Gun-1_7");
        Point.LocalOffset = FVector(575.0f, 0.0f, 80.0f);
        Point.LocalRotation = FRotator(0.0f, 0.0f, 0.0f);
        WeaponMountPointDefs.Add(Point);
    }
    {
        FShipPointDef Point;
        Point.PointName = TEXT("Gun-2_8");
        Point.LocalOffset = FVector(-550.0f, 0.0f, 110.0f);
        Point.LocalRotation = FRotator(0.0f, 180.0f, 0.0f);
        WeaponMountPointDefs.Add(Point);
    }
    {
        FShipPointDef Point;
        Point.PointName = TEXT("Gun-3_9");
        Point.LocalOffset = FVector(-275.0f, 190.0f, 72.0f);
        Point.LocalRotation = FRotator(0.0f, 90.0f, 0.0f);
        WeaponMountPointDefs.Add(Point);
    }
    {
        FShipPointDef Point;
        Point.PointName = TEXT("Gun-4_10");
        Point.LocalOffset = FVector(-275.0f, -190.0f, 72.0f);
        Point.LocalRotation = FRotator(0.0f, 270.0f, 0.0f);
        WeaponMountPointDefs.Add(Point);
    }
    TurretBasePointDefs.Empty();
    NumTurretBasePoints = 6;
    {
        FShipPointDef Point;
        Point.PointName = TEXT("Starboard Missile_5_Base");
        Point.LocalOffset = FVector(400.0f, 60.0f, 0.0f);
        Point.LocalRotation = FRotator(0.0f, 15.0f, 0.0f);
        TurretBasePointDefs.Add(Point);
    }
    {
        FShipPointDef Point;
        Point.PointName = TEXT("Port Missile_6_Base");
        Point.LocalOffset = FVector(400.0f, -60.0f, 0.0f);
        Point.LocalRotation = FRotator(0.0f, -15.0f, 0.0f);
        TurretBasePointDefs.Add(Point);
    }
    {
        FShipPointDef Point;
        Point.PointName = TEXT("Gun-1_7_Base");
        Point.LocalOffset = FVector(575.0f, 0.0f, 80.0f);
        Point.LocalRotation = FRotator(0.0f, 0.0f, 0.0f);
        TurretBasePointDefs.Add(Point);
    }
    {
        FShipPointDef Point;
        Point.PointName = TEXT("Gun-2_8_Base");
        Point.LocalOffset = FVector(-550.0f, 0.0f, 110.0f);
        Point.LocalRotation = FRotator(0.0f, 180.0f, 0.0f);
        TurretBasePointDefs.Add(Point);
    }
    {
        FShipPointDef Point;
        Point.PointName = TEXT("Gun-3_9_Base");
        Point.LocalOffset = FVector(-275.0f, 190.0f, 72.0f);
        Point.LocalRotation = FRotator(0.0f, 90.0f, 0.0f);
        TurretBasePointDefs.Add(Point);
    }
    {
        FShipPointDef Point;
        Point.PointName = TEXT("Gun-4_10_Base");
        Point.LocalOffset = FVector(-275.0f, -190.0f, 72.0f);
        Point.LocalRotation = FRotator(0.0f, 270.0f, 0.0f);
        TurretBasePointDefs.Add(Point);
    }
    DockPointDefs.Empty();
    NumDockPoints = 0;
    LandingPointDefs.Empty();
    NumLandingPoints = 0;
}

void ADevastatorShipActor::ApplyDevastatorFixedPoints()
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
