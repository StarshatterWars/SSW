// Structural defaults from Eagle.def; simulation remains data-driven.
#include "EagleShipActor.h"
#include "Components/SceneComponent.h"

AEagleShipActor::AEagleShipActor()
{
    // Native default only: preserve BP and placed-instance scale during construction.
    SetActorScale3D(FVector(0.250000f));
    ApplyEagleDefaults();
}
void AEagleShipActor::OnConstruction(const FTransform& Transform)
{
    ApplyEagleDefaults();
    Super::OnConstruction(Transform);
    ApplyEagleFixedPoints();
}
void AEagleShipActor::ApplyEagleDefaults()
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
    BridgePointOffset = FVector(102.000000f, 0.000000f, 23.000000f);
    ChasePointOffset = FVector(-770.000000f, 0.000000f, 80.000000f);
    FocusPointOffset = FVector(102.000000f, 0.000000f, 23.000000f);
    DriveCenterPointOffset = FVector(-150.000000f, 0.000000f, 0.000000f);
    QuantumPointOffset = FVector(0.000000f, 0.000000f, 0.000000f);
    ShieldPointOffset = FVector(0.000000f, 0.000000f, 0.000000f);
    SensorPointOffset = FVector(100.000000f, 0.000000f, 0.000000f);
    NavPointOffset = FVector(0.000000f, 0.000000f, 0.000000f);
    ReactorPointOffset = FVector(-40.000000f, 0.000000f, 0.000000f);
    ComputerPointAOffset = FVector(80.000000f, 20.000000f, 0.000000f);
    ComputerPointBOffset = FVector(80.000000f, -20.000000f, 0.000000f);

    WeaponMountPointDefs.Empty();
    NumWeaponMountPoints = 9;
    {
        FShipPointDef Point;
        Point.PointName = TEXT("Cannon_1");
        Point.LocalOffset = FVector(32.000000f, 0.000000f, 0.000000f);
        Point.LocalRotation = FRotator(0.0f,0.000000f,0.0f);
        WeaponMountPointDefs.Add(Point);
    }
    {
        FShipPointDef Point;
        Point.PointName = TEXT("STA-1_2");
        Point.LocalOffset = FVector(-60.000000f, -69.000000f, -22.000000f);
        Point.LocalRotation = FRotator(0.0f,0.000000f,0.0f);
        WeaponMountPointDefs.Add(Point);
    }
    {
        FShipPointDef Point;
        Point.PointName = TEXT("STA-2_3");
        Point.LocalOffset = FVector(-60.000000f, -48.000000f, -17.000000f);
        Point.LocalRotation = FRotator(0.0f,0.000000f,0.0f);
        WeaponMountPointDefs.Add(Point);
    }
    {
        FShipPointDef Point;
        Point.PointName = TEXT("STA-3_4");
        Point.LocalOffset = FVector(-15.000000f, -20.000000f, -13.000000f);
        Point.LocalRotation = FRotator(0.0f,0.000000f,0.0f);
        WeaponMountPointDefs.Add(Point);
    }
    {
        FShipPointDef Point;
        Point.PointName = TEXT("STA-4_5");
        Point.LocalOffset = FVector(60.000000f, -14.000000f, -14.000000f);
        Point.LocalRotation = FRotator(0.0f,0.000000f,0.0f);
        WeaponMountPointDefs.Add(Point);
    }
    {
        FShipPointDef Point;
        Point.PointName = TEXT("STA-5_6");
        Point.LocalOffset = FVector(60.000000f, 15.000000f, -14.000000f);
        Point.LocalRotation = FRotator(0.0f,0.000000f,0.0f);
        WeaponMountPointDefs.Add(Point);
    }
    {
        FShipPointDef Point;
        Point.PointName = TEXT("STA-6_7");
        Point.LocalOffset = FVector(-15.000000f, 20.000000f, -13.000000f);
        Point.LocalRotation = FRotator(0.0f,0.000000f,0.0f);
        WeaponMountPointDefs.Add(Point);
    }
    {
        FShipPointDef Point;
        Point.PointName = TEXT("STA-7_8");
        Point.LocalOffset = FVector(-60.000000f, 48.000000f, -17.000000f);
        Point.LocalRotation = FRotator(0.0f,0.000000f,0.0f);
        WeaponMountPointDefs.Add(Point);
    }
    {
        FShipPointDef Point;
        Point.PointName = TEXT("STA-8_9");
        Point.LocalOffset = FVector(-60.000000f, 70.000000f, -22.000000f);
        Point.LocalRotation = FRotator(0.0f,0.000000f,0.0f);
        WeaponMountPointDefs.Add(Point);
    }
    TurretBasePointDefs.Empty();
    NumTurretBasePoints = 0;
    DockPointDefs.Empty();
    NumDockPoints = 0;
    LandingPointDefs.Empty();
    NumLandingPoints = 0;
}
void AEagleShipActor::ApplyEagleFixedPoints()
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

