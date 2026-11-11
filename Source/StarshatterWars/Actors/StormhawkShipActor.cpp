// Structural defaults from Stormhawk.def; simulation remains data-driven.
#include "StormhawkShipActor.h"
#include "Components/SceneComponent.h"

AStormhawkShipActor::AStormhawkShipActor()
{

    // Native default only: preserve BP and placed-instance scale during construction.
    SetActorScale3D(FVector(0.250000f));
    // Stormhawk's drive and RCS rows are already in unscaled hull units.
    bPropulsionPortsInModelUnits = true;
    ThrusterPortHullName = TEXT("Storm");
    // Hull surface positions in actor axes, ordered as the DEF's RCS ports.
    HullThrusterPortPositions = {
        FVector(-16.08, -112.18, 12.46),
        FVector(-19.69, -73.44, 0.90),
        FVector(-19.45, 73.80, 0.74),
        FVector(-14.96, 111.76, 12.46),
        FVector(-136.80, -81.21, -0.71),
        FVector(-146.24, -64.97, 0.23),
        FVector(-145.72, 64.96, 0.33),
        FVector(-136.91, 81.44, -0.64),
        FVector(119.99, -11.70, 11.57),
        FVector(104.00, -11.91, 11.87),
        FVector(103.97, 11.40, 11.12),
        FVector(119.96, 11.14, 10.74),
        FVector(119.90, -10.78, -13.37),
        FVector(103.92, -11.00, -13.86),
        FVector(103.89, 10.72, -13.25),
        FVector(119.86, 10.46, -12.70),
        FVector(-70.80, -52.25, 9.26),
        FVector(-95.84, -35.10, 28.35),
        FVector(-70.96, 52.37, 9.24),
        FVector(-96.16, 35.49, 28.35),
        FVector(-81.19, -45.01, -21.49),
        FVector(-95.88, -44.93, -21.49),
        FVector(-80.74, 45.24, -21.49),
        FVector(-96.11, 45.32, -21.49),
        FVector(-16.09, -113.74, 12.67),
        FVector(-16.46, -113.22, 16.62),
        FVector(-14.95, 113.32, 12.67),
        FVector(-15.32, 112.81, 16.62),
    };
    ApplyStormhawkDefaults();
    ApplyAuthoredAttachmentDefaults();
    InitializeNativeAuthoredAttachments(TEXT("Stormhawk"));
    InitializeNativeFighterAttachments(TEXT("StormhawkPit"), 0.020000f, BridgePointOffset, {
        FFighterLandingGearDefinition(TEXT("GearNose1"), FVector(100.000000f, 0.000000f, -18.000000f), FVector(130.000000f, 0.000000f, -38.000000f)),
        FFighterLandingGearDefinition(TEXT("GearLeft1"), FVector(-74.000000f, -20.000000f, -20.000000f), FVector(-70.000000f, -40.000000f, -50.000000f)),
        FFighterLandingGearDefinition(TEXT("GearRight1"), FVector(-74.000000f, 20.000000f, -20.000000f), FVector(-70.000000f, 40.000000f, -50.000000f)),
    });

}
void AStormhawkShipActor::OnConstruction(const FTransform& Transform)
{
    ApplyStormhawkDefaults();
    Super::OnConstruction(Transform);
    FighterCockpitRoot->SetRelativeLocation(BridgePointOffset);
    ApplyStormhawkFixedPoints();
}
void AStormhawkShipActor::ApplyStormhawkDefaults()
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
    ChasePointOffset = FVector(-790.000000f, 0.000000f, 90.000000f);
    FocusPointOffset = FVector(102.000000f, 0.000000f, 23.000000f);
    DriveCenterPointOffset = FVector(-100.000000f, 0.000000f, 0.000000f);
    QuantumPointOffset = FVector(0.000000f, 0.000000f, 0.000000f);
    ShieldPointOffset = FVector(0.000000f, 0.000000f, 0.000000f);
    SensorPointOffset = FVector(100.000000f, 0.000000f, 0.000000f);
    NavPointOffset = FVector(0.000000f, 0.000000f, 0.000000f);
    ReactorPointOffset = FVector(-40.000000f, 0.000000f, 0.000000f);
    ComputerPointAOffset = FVector(80.000000f, 20.000000f, 0.000000f);
    ComputerPointBOffset = FVector(80.000000f, -20.000000f, 0.000000f);

    WeaponMountPointDefs.Empty();
    NumWeaponMountPoints = 7;
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
        Point.LocalOffset = FVector(-120.000000f, -78.000000f, -10.000000f);
        Point.LocalRotation = FRotator(0.0f,0.000000f,0.0f);
        WeaponMountPointDefs.Add(Point);
    }
    {
        FShipPointDef Point;
        Point.PointName = TEXT("STA-2_3");
        Point.LocalOffset = FVector(-110.000000f, -50.000000f, -5.000000f);
        Point.LocalRotation = FRotator(0.0f,0.000000f,0.0f);
        WeaponMountPointDefs.Add(Point);
    }
    {
        FShipPointDef Point;
        Point.PointName = TEXT("STA-3_4");
        Point.LocalOffset = FVector(-110.000000f, -16.000000f, -18.000000f);
        Point.LocalRotation = FRotator(0.0f,0.000000f,0.0f);
        WeaponMountPointDefs.Add(Point);
    }
    {
        FShipPointDef Point;
        Point.PointName = TEXT("STA-4_5");
        Point.LocalOffset = FVector(-110.000000f, 16.000000f, -18.000000f);
        Point.LocalRotation = FRotator(0.0f,0.000000f,0.0f);
        WeaponMountPointDefs.Add(Point);
    }
    {
        FShipPointDef Point;
        Point.PointName = TEXT("STA-5_6");
        Point.LocalOffset = FVector(-110.000000f, 50.000000f, -5.000000f);
        Point.LocalRotation = FRotator(0.0f,0.000000f,0.0f);
        WeaponMountPointDefs.Add(Point);
    }
    {
        FShipPointDef Point;
        Point.PointName = TEXT("STA-6_7");
        Point.LocalOffset = FVector(-120.000000f, 78.000000f, -10.000000f);
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
void AStormhawkShipActor::ApplyStormhawkFixedPoints()
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


void AStormhawkShipActor::ApplyAuthoredAttachmentDefaults()
{
    WeaponMountPointDefs.Reset();
    TurretBasePointDefs.Reset();
    { FShipPointDef Point; Point.PointName = TEXT("Cannon");
      Point.LocalOffset = FVector(32.000000f, 0.000000f, 0.000000f);
      Point.LocalRotation = FRotator(0.000000f, 0.000000f, 0.0f);
      WeaponMountPointDefs.Add(Point); }
    { FShipPointDef Point; Point.PointName = TEXT("STA-1");
      Point.LocalOffset = FVector(-120.000000f, -78.000000f, -10.000000f);
      Point.LocalRotation = FRotator(0.000000f, 0.000000f, 0.0f);
      WeaponMountPointDefs.Add(Point); }
    { FShipPointDef Point; Point.PointName = TEXT("STA-2");
      Point.LocalOffset = FVector(-110.000000f, -50.000000f, -5.000000f);
      Point.LocalRotation = FRotator(0.000000f, 0.000000f, 0.0f);
      WeaponMountPointDefs.Add(Point); }
    { FShipPointDef Point; Point.PointName = TEXT("STA-3");
      Point.LocalOffset = FVector(-110.000000f, -16.000000f, -18.000000f);
      Point.LocalRotation = FRotator(0.000000f, 0.000000f, 0.0f);
      WeaponMountPointDefs.Add(Point); }
    { FShipPointDef Point; Point.PointName = TEXT("STA-4");
      Point.LocalOffset = FVector(-110.000000f, 16.000000f, -18.000000f);
      Point.LocalRotation = FRotator(0.000000f, 0.000000f, 0.0f);
      WeaponMountPointDefs.Add(Point); }
    { FShipPointDef Point; Point.PointName = TEXT("STA-5");
      Point.LocalOffset = FVector(-110.000000f, 50.000000f, -5.000000f);
      Point.LocalRotation = FRotator(0.000000f, 0.000000f, 0.0f);
      WeaponMountPointDefs.Add(Point); }
    { FShipPointDef Point; Point.PointName = TEXT("STA-6");
      Point.LocalOffset = FVector(-120.000000f, 78.000000f, -10.000000f);
      Point.LocalRotation = FRotator(0.000000f, 0.000000f, 0.0f);
      WeaponMountPointDefs.Add(Point); }
    SyncShipAttachmentCounts();
}
