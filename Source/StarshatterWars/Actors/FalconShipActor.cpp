// Structural defaults from Falcon.def; simulation remains data-driven.
#include "FalconShipActor.h"
#include "Components/SceneComponent.h"

AFalconShipActor::AFalconShipActor()
{
    ApplyFalconDefaults();
}
void AFalconShipActor::OnConstruction(const FTransform& Transform)
{
    ApplyFalconDefaults();
    Super::OnConstruction(Transform);
    ApplyFalconFixedPoints();
}
void AFalconShipActor::ApplyFalconDefaults()
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
    BridgePointOffset = FVector(131.000000f, 0.000000f, 23.000000f);
    ChasePointOffset = FVector(-750.000000f, 0.000000f, 80.000000f);
    FocusPointOffset = FVector(131.000000f, 0.000000f, 23.000000f);
    DriveCenterPointOffset = FVector(-130.000000f, 0.000000f, 0.000000f);
    QuantumPointOffset = FVector(0.000000f, 0.000000f, 0.000000f);
    ShieldPointOffset = FVector(0.000000f, 0.000000f, 0.000000f);
    SensorPointOffset = FVector(100.000000f, 0.000000f, 0.000000f);
    NavPointOffset = FVector(0.000000f, 0.000000f, 0.000000f);
    ReactorPointOffset = FVector(-40.000000f, 0.000000f, 0.000000f);
    ComputerPointAOffset = FVector(80.000000f, 20.000000f, 0.000000f);
    ComputerPointBOffset = FVector(80.000000f, -20.000000f, 0.000000f);
    SetActorScale3D(FVector(0.270000f));
    WeaponMountPointDefs.Empty();
    NumWeaponMountPoints = 5;
    {
        FShipPointDef Point;
        Point.PointName = TEXT("Cannon_1");
        Point.LocalOffset = FVector(-32.000000f, 0.000000f, 0.000000f);
        Point.LocalRotation = FRotator(0.0f,0.000000f,0.0f);
        WeaponMountPointDefs.Add(Point);
    }
    {
        FShipPointDef Point;
        Point.PointName = TEXT("STA-1_2");
        Point.LocalOffset = FVector(-145.000000f, -110.000000f, -7.000000f);
        Point.LocalRotation = FRotator(0.0f,0.000000f,0.0f);
        WeaponMountPointDefs.Add(Point);
    }
    {
        FShipPointDef Point;
        Point.PointName = TEXT("STA-2_3");
        Point.LocalOffset = FVector(-145.000000f, -93.000000f, -5.000000f);
        Point.LocalRotation = FRotator(0.0f,0.000000f,0.0f);
        WeaponMountPointDefs.Add(Point);
    }
    {
        FShipPointDef Point;
        Point.PointName = TEXT("STA-3_4");
        Point.LocalOffset = FVector(-145.000000f, 93.000000f, -5.000000f);
        Point.LocalRotation = FRotator(0.0f,0.000000f,0.0f);
        WeaponMountPointDefs.Add(Point);
    }
    {
        FShipPointDef Point;
        Point.PointName = TEXT("STA-4_5");
        Point.LocalOffset = FVector(-145.000000f, 110.000000f, -7.000000f);
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
void AFalconShipActor::ApplyFalconFixedPoints()
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

