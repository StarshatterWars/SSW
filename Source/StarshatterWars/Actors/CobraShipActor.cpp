// Structural defaults from Cobra.def; simulation remains data-driven.
#include "CobraShipActor.h"
#include "Components/SceneComponent.h"

ACobraShipActor::ACobraShipActor()
{
    SetActorScale3D(FVector(0.260000f));
    ApplyCobraDefaults();
}
void ACobraShipActor::OnConstruction(const FTransform& Transform)
{
    ApplyCobraDefaults();
    Super::OnConstruction(Transform);
    ApplyCobraFixedPoints();
}
void ACobraShipActor::ApplyCobraDefaults()
{
    bAutoRebuildGeneratedComponents = true;
    bRebuildOnConstruction = true;
    bEnableMainEngineEmitters = true;
    bEnableThrusterEmitters = true;
    MainEngineEmitterRelativeScale = FVector(1.15f, 0.5f, 0.5f);
    MainEngineEmitterRelativeRotation = FRotator(0.0f, 180.0f, 0.0f);
    NumMainEnginePoints = NumThrusterPoints = 0;
    MainEnginePointDefs.Empty(); ThrusterPointDefs.Empty(); NavLightDefs.Empty();
    BridgePointOffset = FVector(8.000000f, 0.000000f, 145.000000f);
    ChasePointOffset = FVector(250.000000f, 0.000000f, -1200.000000f);
    FocusPointOffset = FVector(8.000000f, 0.000000f, 145.000000f);
    DriveCenterPointOffset = FVector(-190.000000f, 0.000000f, 0.000000f);
    QuantumPointOffset = FVector(0.0f, 0.0f, 0.0f);
    ShieldPointOffset = FVector(0.0f, 0.0f, 0.0f);
    SensorPointOffset = FVector(100.000000f, 0.000000f, 0.000000f);
    NavPointOffset = FVector(60.000000f, 0.000000f, 0.000000f);
    ReactorPointOffset = FVector(0.000000f, 0.000000f, 0.000000f);
    ComputerPointAOffset = FVector(80.000000f, 20.000000f, 0.000000f);
    ComputerPointBOffset = FVector(80.000000f, -20.000000f, 0.000000f);

    WeaponMountPointDefs.Empty();
    NumWeaponMountPoints = 9;
    {
        FShipPointDef Point;
        Point.PointName = TEXT("Cannon");
        Point.LocalOffset = FVector(175.000000f, -7.000000f, 8.000000f);
        Point.LocalRotation = FRotator(0.0f, 0.0f, 0.0f);
        WeaponMountPointDefs.Add(Point);
    }
    {
        FShipPointDef Point;
        Point.PointName = TEXT("STA-1");
        Point.LocalOffset = FVector(-70.000000f, 129.000000f, -21.000000f);
        Point.LocalRotation = FRotator(0.0f, 0.0f, 0.0f);
        WeaponMountPointDefs.Add(Point);
    }
    {
        FShipPointDef Point;
        Point.PointName = TEXT("STA-2");
        Point.LocalOffset = FVector(-70.000000f, 105.000000f, -18.000000f);
        Point.LocalRotation = FRotator(0.0f, 0.0f, 0.0f);
        WeaponMountPointDefs.Add(Point);
    }
    {
        FShipPointDef Point;
        Point.PointName = TEXT("STA-3");
        Point.LocalOffset = FVector(-70.000000f, 65.000000f, -16.000000f);
        Point.LocalRotation = FRotator(0.0f, 0.0f, 0.0f);
        WeaponMountPointDefs.Add(Point);
    }
    {
        FShipPointDef Point;
        Point.PointName = TEXT("STA-4");
        Point.LocalOffset = FVector(-70.000000f, 40.000000f, -14.000000f);
        Point.LocalRotation = FRotator(0.0f, 0.0f, 0.0f);
        WeaponMountPointDefs.Add(Point);
    }
    {
        FShipPointDef Point;
        Point.PointName = TEXT("STA-5");
        Point.LocalOffset = FVector(-70.000000f, -40.000000f, -14.000000f);
        Point.LocalRotation = FRotator(0.0f, 0.0f, 0.0f);
        WeaponMountPointDefs.Add(Point);
    }
    {
        FShipPointDef Point;
        Point.PointName = TEXT("STA-6");
        Point.LocalOffset = FVector(-70.000000f, -65.000000f, -16.000000f);
        Point.LocalRotation = FRotator(0.0f, 0.0f, 0.0f);
        WeaponMountPointDefs.Add(Point);
    }
    {
        FShipPointDef Point;
        Point.PointName = TEXT("STA-7");
        Point.LocalOffset = FVector(-70.000000f, -105.000000f, -18.000000f);
        Point.LocalRotation = FRotator(0.0f, 0.0f, 0.0f);
        WeaponMountPointDefs.Add(Point);
    }
    {
        FShipPointDef Point;
        Point.PointName = TEXT("STA-8");
        Point.LocalOffset = FVector(-70.000000f, -128.000000f, -21.000000f);
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
void ACobraShipActor::ApplyCobraFixedPoints()
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
