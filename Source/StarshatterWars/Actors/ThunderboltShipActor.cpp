// Structural defaults from Thunderbolt.def; simulation remains data-driven.
#include "ThunderboltShipActor.h"
#include "Components/SceneComponent.h"

AThunderboltShipActor::AThunderboltShipActor()
{
    // Native default only: preserve BP and placed-instance scale during construction.
    SetActorScale3D(FVector(0.270000f));
    ApplyThunderboltDefaults();
}
void AThunderboltShipActor::OnConstruction(const FTransform& Transform)
{
    ApplyThunderboltDefaults();
    Super::OnConstruction(Transform);
    ApplyThunderboltFixedPoints();
}
void AThunderboltShipActor::ApplyThunderboltDefaults()
{
    bAutoRebuildGeneratedComponents = true;
    bRebuildOnConstruction = true;
    bEnableMainEngineEmitters = true;
    bEnableThrusterEmitters = true;
    MainEngineEmitterRelativeScale = FVector(1.15f, 0.5f, 0.5f);
    MainEngineEmitterRelativeRotation = FRotator(0.0f,180.0f,0.0f);
    // Runtime systems provide engine/thruster/nav-light definitions.
    NumMainEnginePoints = NumThrusterPoints = 0;
    MainEnginePointDefs.Empty(); ThrusterPointDefs.Empty(); NavLightDefs.Empty();
    BridgePointOffset = FVector(180.000000f, 0.000000f, 20.000000f);
    ChasePointOffset = FVector(-1000.000000f, 0.000000f, 130.000000f);
    FocusPointOffset = FVector(180.000000f, 0.000000f, 20.000000f);
    DriveCenterPointOffset = FVector(-90.000000f, 0.000000f, 0.000000f);
    QuantumPointOffset = FVector(0.000000f, 0.000000f, 0.000000f);
    ShieldPointOffset = FVector(0.000000f, 0.000000f, 0.000000f);
    SensorPointOffset = FVector(30.000000f, 0.000000f, 0.000000f);
    NavPointOffset = FVector(0.000000f, 0.000000f, 0.000000f);
    ReactorPointOffset = FVector(0.000000f, 0.000000f, 0.000000f);
    ComputerPointAOffset = FVector(40.000000f, 0.000000f, 0.000000f);
    ComputerPointBOffset = FVector(90.000000f, 0.000000f, 0.000000f);

    WeaponMountPointDefs.Empty();
    NumWeaponMountPoints = 8;
    {
        FShipPointDef Point;
        Point.PointName = TEXT("Cannon_1");
        Point.LocalOffset = FVector(160.000000f, 0.000000f, 0.000000f);
        Point.LocalRotation = FRotator(0.0f,0.000000f,0.0f);
        WeaponMountPointDefs.Add(Point);
    }
    {
        FShipPointDef Point;
        Point.PointName = TEXT("Turret_2");
        Point.LocalOffset = FVector(-195.000000f, 0.000000f, 0.000000f);
        Point.LocalRotation = FRotator(0.0f,180.000421f,0.0f);
        WeaponMountPointDefs.Add(Point);
    }
    {
        FShipPointDef Point;
        Point.PointName = TEXT("STA-1_3");
        Point.LocalOffset = FVector(-70.000000f, -116.000000f, -25.000000f);
        Point.LocalRotation = FRotator(0.0f,0.000000f,0.0f);
        WeaponMountPointDefs.Add(Point);
    }
    {
        FShipPointDef Point;
        Point.PointName = TEXT("STA-2_4");
        Point.LocalOffset = FVector(-60.000000f, -60.000000f, -16.000000f);
        Point.LocalRotation = FRotator(0.0f,0.000000f,0.0f);
        WeaponMountPointDefs.Add(Point);
    }
    {
        FShipPointDef Point;
        Point.PointName = TEXT("STA-3_5");
        Point.LocalOffset = FVector(-60.000000f, -40.000000f, -16.000000f);
        Point.LocalRotation = FRotator(0.0f,0.000000f,0.0f);
        WeaponMountPointDefs.Add(Point);
    }
    {
        FShipPointDef Point;
        Point.PointName = TEXT("STA-4_6");
        Point.LocalOffset = FVector(-60.000000f, 40.000000f, -16.000000f);
        Point.LocalRotation = FRotator(0.0f,0.000000f,0.0f);
        WeaponMountPointDefs.Add(Point);
    }
    {
        FShipPointDef Point;
        Point.PointName = TEXT("STA-5_7");
        Point.LocalOffset = FVector(-60.000000f, 60.000000f, -16.000000f);
        Point.LocalRotation = FRotator(0.0f,0.000000f,0.0f);
        WeaponMountPointDefs.Add(Point);
    }
    {
        FShipPointDef Point;
        Point.PointName = TEXT("STA-6_8");
        Point.LocalOffset = FVector(-70.000000f, 116.000000f, -25.000000f);
        Point.LocalRotation = FRotator(0.0f,0.000000f,0.0f);
        WeaponMountPointDefs.Add(Point);
    }
    TurretBasePointDefs.Empty();
    NumTurretBasePoints = 1;
    {
        FShipPointDef Point;
        Point.PointName = TEXT("Turret_2_Base");
        Point.LocalOffset = FVector(-195.000000f, 0.000000f, 0.000000f);
        Point.LocalRotation = FRotator(0.0f,180.000421f,0.0f);
        TurretBasePointDefs.Add(Point);
    }
    DockPointDefs.Empty();
    NumDockPoints = 0;
    LandingPointDefs.Empty();
    NumLandingPoints = 0;
}
void AThunderboltShipActor::ApplyThunderboltFixedPoints()
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

