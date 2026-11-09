// Structural defaults from Thunderbolt.def; simulation remains data-driven.
#include "ThunderboltShipActor.h"
#include "Components/SceneComponent.h"

AThunderboltShipActor::AThunderboltShipActor()
{


    // Native default only: preserve BP and placed-instance scale during construction.
    SetActorScale3D(FVector(0.270000f));
    bPropulsionPortsInModelUnits = true;
    ThrusterPortHullName = TEXT("Thunderbolt");
    // Hull surface positions in actor axes, ordered as the DEF's RCS ports.
    HullThrusterPortPositions = {
        FVector(65.29, -85.19, 2.13),
        FVector(58.46, -65.74, -3.44),
        FVector(65.25, 76.33, 3.51),
        FVector(65.29, 85.19, 2.13),
        FVector(-93.58, -85.19, 2.13),
        FVector(-93.54, -76.45, 3.54),
        FVector(-93.54, 76.35, 3.48),
        FVector(-93.58, 85.19, 2.13),
        FVector(118.40, -24.88, -1.35),
        FVector(-155.56, -38.43, -0.02),
        FVector(118.40, 24.76, 1.29),
        FVector(-155.56, 38.30, -0.02),
        FVector(40.66, -85.08, 9.77),
        FVector(40.66, 85.08, 9.77),
        FVector(-68.95, -85.08, 9.77),
        FVector(-68.95, 85.08, 9.77),
        FVector(-140.74, -13.99, 20.85),
        FVector(-173.95, -13.99, 20.85),
        FVector(-140.74, 13.99, 20.82),
        FVector(-173.95, 13.99, 20.82),
        FVector(44.44, -85.19, -23.99),
        FVector(44.44, 85.19, -23.99),
        FVector(-74.07, -85.19, -23.99),
        FVector(-74.07, 85.19, -23.99),
        FVector(-140.74, -14.58, -20.79),
        FVector(-173.95, -14.58, -20.79),
        FVector(-140.74, 14.58, -20.77),
        FVector(-173.95, 14.58, -20.77),
        FVector(-93.52, -93.11, -0.28),
        FVector(65.14, -93.14, -0.27),
        FVector(-93.52, 93.01, -0.34),
        FVector(65.11, 93.04, -0.32),
    };
    ApplyThunderboltDefaults();
    ApplyAuthoredAttachmentDefaults();
    InitializeNativeAuthoredAttachments(TEXT("Thunderbolt"));
    InitializeNativeFighterAttachments(TEXT("ThunderboltPit"), 0.020000f, FVector(20.000000f, 0.000000f, 180.000000f), {
        FFighterLandingGearDefinition(TEXT("GearNose1"), FVector(144.000000f, 0.000000f, -15.000000f), FVector(158.000000f, 0.000000f, -35.000000f)),
        FFighterLandingGearDefinition(TEXT("GearLeft1"), FVector(-64.000000f, -20.000000f, -22.000000f), FVector(-64.000000f, -42.000000f, -48.000000f)),
        FFighterLandingGearDefinition(TEXT("GearRight1"), FVector(-64.000000f, 20.000000f, -22.000000f), FVector(-64.000000f, 42.000000f, -48.000000f)),
    });

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


void AThunderboltShipActor::ApplyAuthoredAttachmentDefaults()
{
    WeaponMountPointDefs.Reset();
    TurretBasePointDefs.Reset();
    { FShipPointDef Point; Point.PointName = TEXT("Cannon");
      Point.LocalOffset = FVector(160.000000f, 0.000000f, 0.000000f);
      Point.LocalRotation = FRotator(0.000000f, 0.000000f, 0.0f);
      WeaponMountPointDefs.Add(Point); }
    { FShipPointDef Point; Point.PointName = TEXT("Turret");
      Point.LocalOffset = FVector(-195.000000f, 0.000000f, 0.000000f);
      Point.LocalRotation = FRotator(0.000000000, 180.000420918, 0.000000000);
      WeaponMountPointDefs.Add(Point); }
    { FShipPointDef Point; Point.PointName = TEXT("STA-1");
      Point.LocalOffset = FVector(-70.000000f, -116.000000f, -25.000000f);
      Point.LocalRotation = FRotator(0.000000f, 0.000000f, 0.0f);
      WeaponMountPointDefs.Add(Point); }
    { FShipPointDef Point; Point.PointName = TEXT("STA-2");
      Point.LocalOffset = FVector(-60.000000f, -60.000000f, -16.000000f);
      Point.LocalRotation = FRotator(0.000000f, 0.000000f, 0.0f);
      WeaponMountPointDefs.Add(Point); }
    { FShipPointDef Point; Point.PointName = TEXT("STA-3");
      Point.LocalOffset = FVector(-60.000000f, -40.000000f, -16.000000f);
      Point.LocalRotation = FRotator(0.000000f, 0.000000f, 0.0f);
      WeaponMountPointDefs.Add(Point); }
    { FShipPointDef Point; Point.PointName = TEXT("STA-4");
      Point.LocalOffset = FVector(-60.000000f, 40.000000f, -16.000000f);
      Point.LocalRotation = FRotator(0.000000f, 0.000000f, 0.0f);
      WeaponMountPointDefs.Add(Point); }
    { FShipPointDef Point; Point.PointName = TEXT("STA-5");
      Point.LocalOffset = FVector(-60.000000f, 60.000000f, -16.000000f);
      Point.LocalRotation = FRotator(0.000000f, 0.000000f, 0.0f);
      WeaponMountPointDefs.Add(Point); }
    { FShipPointDef Point; Point.PointName = TEXT("STA-6");
      Point.LocalOffset = FVector(-70.000000f, 116.000000f, -25.000000f);
      Point.LocalRotation = FRotator(0.000000f, 0.000000f, 0.0f);
      WeaponMountPointDefs.Add(Point); }
    { FShipPointDef Point; Point.PointName = TEXT("Turret_Base");
      Point.LocalOffset = FVector(-195.000000f, 0.000000f, 0.000000f);
      Point.LocalRotation = FRotator(0.000000000, 180.000420918, 0.000000000);
      TurretBasePointDefs.Add(Point); }
    SyncShipAttachmentCounts();
}
