// Structural defaults from Avenger.def; simulation remains data-driven.
#include "AvengerShipActor.h"
#include "Components/SceneComponent.h"

AAvengerShipActor::AAvengerShipActor()
{
    ConfigureWeaponHardpointsFromShipDef(TEXT("Avenger"));
    SetActorScale3D(FVector(0.270000f));
    ApplyAvengerDefaults();
}
void AAvengerShipActor::OnConstruction(const FTransform& Transform)
{
    ApplyAvengerDefaults();
    Super::OnConstruction(Transform);
    ApplyAvengerFixedPoints();
}
void AAvengerShipActor::ApplyAvengerDefaults()
{
    bAutoRebuildGeneratedComponents = true;
    bRebuildOnConstruction = true;
    bEnableMainEngineEmitters = true;
    bEnableThrusterEmitters = true;
    MainEngineEmitterRelativeScale = FVector(1.15f, 0.5f, 0.5f);
    MainEngineEmitterRelativeRotation = FRotator(0.0f, 180.0f, 0.0f);
    NumMainEnginePoints = NumThrusterPoints = 0;
    MainEnginePointDefs.Empty(); ThrusterPointDefs.Empty(); NavLightDefs.Empty();
    BridgePointOffset = FVector(0.000000f, 0.000000f, 256.000000f);
    ChasePointOffset = FVector(250.000000f, 0.000000f, -1200.000000f);
    FocusPointOffset = FVector(0.000000f, 0.000000f, 256.000000f);
    DriveCenterPointOffset = FVector(-120.000000f, 0.000000f, 0.000000f);
    QuantumPointOffset = FVector(0.0f, 0.0f, 0.0f);
    ShieldPointOffset = FVector(0.0f, 0.0f, 0.0f);
    SensorPointOffset = FVector(30.000000f, 0.000000f, 0.000000f);
    NavPointOffset = FVector(-10.000000f, 0.000000f, 0.000000f);
    ReactorPointOffset = FVector(0.000000f, 0.000000f, 0.000000f);
    ComputerPointAOffset = FVector(40.000000f, 0.000000f, 0.000000f);
    ComputerPointBOffset = FVector(90.000000f, 0.000000f, 0.000000f);

    WeaponMountPointDefs.Empty();
    NumWeaponMountPoints = 9;
    {
        FShipPointDef Point;
        Point.PointName = TEXT("Cannon");
        Point.LocalOffset = FVector(205.000000f, -33.000000f, -21.000000f);
        Point.LocalRotation = FRotator(0.0f, 0.0f, 0.0f);
        WeaponMountPointDefs.Add(Point);
    }
    {
        FShipPointDef Point;
        Point.PointName = TEXT("STA-1");
        Point.LocalOffset = FVector(-135.000000f, -111.000000f, -35.000000f);
        Point.LocalRotation = FRotator(0.0f, 0.0f, 0.0f);
        WeaponMountPointDefs.Add(Point);
    }
    {
        FShipPointDef Point;
        Point.PointName = TEXT("STA-2");
        Point.LocalOffset = FVector(-130.000000f, -93.000000f, -34.000000f);
        Point.LocalRotation = FRotator(0.0f, 0.0f, 0.0f);
        WeaponMountPointDefs.Add(Point);
    }
    {
        FShipPointDef Point;
        Point.PointName = TEXT("STA-3");
        Point.LocalOffset = FVector(-125.000000f, -76.000000f, -33.000000f);
        Point.LocalRotation = FRotator(0.0f, 0.0f, 0.0f);
        WeaponMountPointDefs.Add(Point);
    }
    {
        FShipPointDef Point;
        Point.PointName = TEXT("STA-4");
        Point.LocalOffset = FVector(-120.000000f, -58.000000f, -32.000000f);
        Point.LocalRotation = FRotator(0.0f, 0.0f, 0.0f);
        WeaponMountPointDefs.Add(Point);
    }
    {
        FShipPointDef Point;
        Point.PointName = TEXT("STA-5");
        Point.LocalOffset = FVector(-120.000000f, 58.000000f, -32.000000f);
        Point.LocalRotation = FRotator(0.0f, 0.0f, 0.0f);
        WeaponMountPointDefs.Add(Point);
    }
    {
        FShipPointDef Point;
        Point.PointName = TEXT("STA-6");
        Point.LocalOffset = FVector(-125.000000f, 76.000000f, -33.000000f);
        Point.LocalRotation = FRotator(0.0f, 0.0f, 0.0f);
        WeaponMountPointDefs.Add(Point);
    }
    {
        FShipPointDef Point;
        Point.PointName = TEXT("STA-7");
        Point.LocalOffset = FVector(-130.000000f, 93.000000f, -34.000000f);
        Point.LocalRotation = FRotator(0.0f, 0.0f, 0.0f);
        WeaponMountPointDefs.Add(Point);
    }
    {
        FShipPointDef Point;
        Point.PointName = TEXT("STA-8");
        Point.LocalOffset = FVector(-135.000000f, 111.000000f, -35.000000f);
        Point.LocalRotation = FRotator(0.0f, 0.0f, 0.0f);
        WeaponMountPointDefs.Add(Point);
    }
    TurretBasePointDefs.Empty();
    NumTurretBasePoints = 0;
    DockPointDefs.Empty();
    NumDockPoints = 0;
    LandingPointDefs.Empty();
    NumLandingPoints = 0;
}
void AAvengerShipActor::ApplyAvengerFixedPoints()
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
