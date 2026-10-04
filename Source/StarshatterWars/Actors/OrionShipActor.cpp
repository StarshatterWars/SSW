// Model defaults derived from Orion.def.
#include "OrionShipActor.h"
#include "Components/SceneComponent.h"

AOrionShipActor::AOrionShipActor()
{
    ApplyOrionDefaults();
}

void AOrionShipActor::OnConstruction(const FTransform& Transform)
{
    ApplyOrionDefaults();
    Super::OnConstruction(Transform);
    ApplyOrionFixedPoints();
}

void AOrionShipActor::ApplyOrionDefaults()
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
    BridgePointOffset = FVector(600.0f, 0.0f, 100.0f);
    ChasePointOffset = FVector(-1800.0f, 0.0f, 170.0f);
    FocusPointOffset = FVector(600.0f, 0.0f, 100.0f);
    DriveCenterPointOffset = FVector(-1240.0f, 0.0f, 48.0f);
    QuantumPointOffset = FVector(0.0f, 0.0f, 0.0f);
    ShieldPointOffset = FVector(0.0f, 0.0f, 0.0f);
    SensorPointOffset = FVector(380.0f, 0.0f, -16.0f);
    NavPointOffset = FVector(60.0f, 0.0f, 16.0f);
    ReactorPointOffset = FVector(-180.0f, 0.0f, -48.0f);
    ComputerPointAOffset = FVector(80.0f, 20.0f, 16.0f);
    ComputerPointBOffset = FVector(80.0f, -20.0f, -16.0f);
    SetActorScale3D(FVector(4.0f));

    WeaponMountPointDefs.Empty();
    NumWeaponMountPoints = 10;
    {
        FShipPointDef Point;
        Point.PointName = TEXT("Vanguard 1_1");
        Point.LocalOffset = FVector(704.0f, 128.0f, 95.0f);
        Point.LocalRotation = FRotator(0.0f, 60.0f, 0.0f);
        WeaponMountPointDefs.Add(Point);
    }
    {
        FShipPointDef Point;
        Point.PointName = TEXT("Vanguard 2_2");
        Point.LocalOffset = FVector(704.0f, -128.0f, 95.0f);
        Point.LocalRotation = FRotator(0.0f, -60.0f, 0.0f);
        WeaponMountPointDefs.Add(Point);
    }
    {
        FShipPointDef Point;
        Point.PointName = TEXT("Vanguard 3_3");
        Point.LocalOffset = FVector(228.0f, 128.0f, 95.0f);
        Point.LocalRotation = FRotator(0.0f, 90.0f, 0.0f);
        WeaponMountPointDefs.Add(Point);
    }
    {
        FShipPointDef Point;
        Point.PointName = TEXT("Vanguard 4_4");
        Point.LocalOffset = FVector(228.0f, -128.0f, 95.0f);
        Point.LocalRotation = FRotator(0.0f, -90.0f, 0.0f);
        WeaponMountPointDefs.Add(Point);
    }
    {
        FShipPointDef Point;
        Point.PointName = TEXT("Vanguard 5_5");
        Point.LocalOffset = FVector(-830.0f, 64.0f, 103.0f);
        Point.LocalRotation = FRotator(0.0f, 180.0f, 0.0f);
        WeaponMountPointDefs.Add(Point);
    }
    {
        FShipPointDef Point;
        Point.PointName = TEXT("Vanguard 6_6");
        Point.LocalOffset = FVector(-830.0f, -64.0f, 103.0f);
        Point.LocalRotation = FRotator(0.0f, 180.0f, 0.0f);
        WeaponMountPointDefs.Add(Point);
    }
    {
        FShipPointDef Point;
        Point.PointName = TEXT("Port Phalanx_7");
        Point.LocalOffset = FVector(960.0f, 0.0f, 70.0f);
        Point.LocalRotation = FRotator(0.0f, 0.0f, 0.0f);
        WeaponMountPointDefs.Add(Point);
    }
    {
        FShipPointDef Point;
        Point.PointName = TEXT("Port Phalanx_8");
        Point.LocalOffset = FVector(960.0f, 0.0f, -75.0f);
        Point.LocalRotation = FRotator(0.0f, 0.0f, 0.0f);
        WeaponMountPointDefs.Add(Point);
    }
    {
        FShipPointDef Point;
        Point.PointName = TEXT("Port Phalanx_9");
        Point.LocalOffset = FVector(-830.0f, 64.0f, -132.0f);
        Point.LocalRotation = FRotator(0.0f, 180.0f, 0.0f);
        WeaponMountPointDefs.Add(Point);
    }
    {
        FShipPointDef Point;
        Point.PointName = TEXT("Port Phalanx_10");
        Point.LocalOffset = FVector(-830.0f, -64.0f, -132.0f);
        Point.LocalRotation = FRotator(0.0f, 180.0f, 0.0f);
        WeaponMountPointDefs.Add(Point);
    }
    TurretBasePointDefs.Empty();
    NumTurretBasePoints = 8;
    {
        FShipPointDef Point;
        Point.PointName = TEXT("Vanguard 1_1_Base");
        Point.LocalOffset = FVector(704.0f, 128.0f, 95.0f);
        Point.LocalRotation = FRotator(0.0f, 60.0f, 0.0f);
        TurretBasePointDefs.Add(Point);
    }
    {
        FShipPointDef Point;
        Point.PointName = TEXT("Vanguard 2_2_Base");
        Point.LocalOffset = FVector(704.0f, -128.0f, 95.0f);
        Point.LocalRotation = FRotator(0.0f, -60.0f, 0.0f);
        TurretBasePointDefs.Add(Point);
    }
    {
        FShipPointDef Point;
        Point.PointName = TEXT("Vanguard 3_3_Base");
        Point.LocalOffset = FVector(228.0f, 128.0f, 95.0f);
        Point.LocalRotation = FRotator(0.0f, 90.0f, 0.0f);
        TurretBasePointDefs.Add(Point);
    }
    {
        FShipPointDef Point;
        Point.PointName = TEXT("Vanguard 4_4_Base");
        Point.LocalOffset = FVector(228.0f, -128.0f, 95.0f);
        Point.LocalRotation = FRotator(0.0f, -90.0f, 0.0f);
        TurretBasePointDefs.Add(Point);
    }
    {
        FShipPointDef Point;
        Point.PointName = TEXT("Vanguard 5_5_Base");
        Point.LocalOffset = FVector(-830.0f, 64.0f, 103.0f);
        Point.LocalRotation = FRotator(0.0f, 180.0f, 0.0f);
        TurretBasePointDefs.Add(Point);
    }
    {
        FShipPointDef Point;
        Point.PointName = TEXT("Vanguard 6_6_Base");
        Point.LocalOffset = FVector(-830.0f, -64.0f, 103.0f);
        Point.LocalRotation = FRotator(0.0f, 180.0f, 0.0f);
        TurretBasePointDefs.Add(Point);
    }
    {
        FShipPointDef Point;
        Point.PointName = TEXT("Port Phalanx_9_Base");
        Point.LocalOffset = FVector(-830.0f, 64.0f, -132.0f);
        Point.LocalRotation = FRotator(0.0f, 180.0f, 0.0f);
        TurretBasePointDefs.Add(Point);
    }
    {
        FShipPointDef Point;
        Point.PointName = TEXT("Port Phalanx_10_Base");
        Point.LocalOffset = FVector(-830.0f, -64.0f, -132.0f);
        Point.LocalRotation = FRotator(0.0f, 180.0f, 0.0f);
        TurretBasePointDefs.Add(Point);
    }
    DockPointDefs.Empty();
    NumDockPoints = 8;
    {
        FShipPointDef Point;
        Point.PointName = TEXT("Deck 1_Spot_1");
        Point.LocalOffset = FVector(825.0f, 45.0f, -20.0f);
        Point.LocalRotation = FRotator(0.0f, 0.0f, 0.0f);
        DockPointDefs.Add(Point);
    }
    {
        FShipPointDef Point;
        Point.PointName = TEXT("Deck 1_Spot_2");
        Point.LocalOffset = FVector(825.0f, 75.0f, -20.0f);
        Point.LocalRotation = FRotator(0.0f, 0.0f, 0.0f);
        DockPointDefs.Add(Point);
    }
    {
        FShipPointDef Point;
        Point.PointName = TEXT("Deck 1_Spot_3");
        Point.LocalOffset = FVector(825.0f, 60.0f, -20.0f);
        Point.LocalRotation = FRotator(0.0f, 0.0f, 0.0f);
        DockPointDefs.Add(Point);
    }
    {
        FShipPointDef Point;
        Point.PointName = TEXT("Deck 2_Spot_1");
        Point.LocalOffset = FVector(825.0f, -45.0f, -20.0f);
        Point.LocalRotation = FRotator(0.0f, 0.0f, 0.0f);
        DockPointDefs.Add(Point);
    }
    {
        FShipPointDef Point;
        Point.PointName = TEXT("Deck 2_Spot_2");
        Point.LocalOffset = FVector(825.0f, -75.0f, -20.0f);
        Point.LocalRotation = FRotator(0.0f, 0.0f, 0.0f);
        DockPointDefs.Add(Point);
    }
    {
        FShipPointDef Point;
        Point.PointName = TEXT("Deck 2_Spot_3");
        Point.LocalOffset = FVector(825.0f, -60.0f, -20.0f);
        Point.LocalRotation = FRotator(0.0f, 0.0f, 0.0f);
        DockPointDefs.Add(Point);
    }
    {
        FShipPointDef Point;
        Point.PointName = TEXT("Deck 3_Spot_1");
        Point.LocalOffset = FVector(-500.0f, 305.0f, -12.0f);
        Point.LocalRotation = FRotator(0.0f, 0.0f, 0.0f);
        DockPointDefs.Add(Point);
    }
    {
        FShipPointDef Point;
        Point.PointName = TEXT("Deck 4_Spot_1");
        Point.LocalOffset = FVector(-500.0f, -305.0f, -12.0f);
        Point.LocalRotation = FRotator(0.0f, 0.0f, 0.0f);
        DockPointDefs.Add(Point);
    }
    LandingPointDefs.Empty();
    NumLandingPoints = 2;
    {
        FShipPointDef Point;
        Point.PointName = TEXT("Deck 3_Recovery");
        Point.LocalOffset = FVector(-1950.0f, 305.0f, 0.0f);
        Point.LocalRotation = FRotator(0.0f, 0.0f, 0.0f);
        LandingPointDefs.Add(Point);
    }
    {
        FShipPointDef Point;
        Point.PointName = TEXT("Deck 4_Recovery");
        Point.LocalOffset = FVector(-1950.0f, -305.0f, 0.0f);
        Point.LocalRotation = FRotator(0.0f, 0.0f, 0.0f);
        LandingPointDefs.Add(Point);
    }
}

void AOrionShipActor::ApplyOrionFixedPoints()
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
