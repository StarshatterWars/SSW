// Structural defaults from Intruder.def; simulation remains data-driven.
#include "IntruderShipActor.h"
#include "Components/SceneComponent.h"

AIntruderShipActor::AIntruderShipActor()
{

    SetActorScale3D(FVector(0.250000f));
    ApplyIntruderDefaults();
    ApplyAuthoredAttachmentDefaults();
    InitializeNativeAuthoredAttachments(TEXT("Intruder"));
    InitializeNativeFighterAttachments(TEXT("IntruderPit"), 0.020000f, FVector(0.000000f, 0.000000f, 240.000000f), {
        FFighterLandingGearDefinition(TEXT("GearNose0.mag"), FVector(134.000000f, 0.000000f, -25.000000f), FVector(148.000000f, 0.000000f, -50.000000f)),
        FFighterLandingGearDefinition(TEXT("GearLeft0.mag"), FVector(-64.000000f, -20.000000f, -30.000000f), FVector(-64.000000f, -45.000000f, -60.000000f)),
        FFighterLandingGearDefinition(TEXT("GearRight0.mag"), FVector(-64.000000f, 20.000000f, -30.000000f), FVector(-64.000000f, 45.000000f, -60.000000f)),
    });

}
void AIntruderShipActor::OnConstruction(const FTransform& Transform)
{
    ApplyIntruderDefaults();
    Super::OnConstruction(Transform);
    ApplyIntruderFixedPoints();
}
void AIntruderShipActor::ApplyIntruderDefaults()
{
    bAutoRebuildGeneratedComponents = true;
    bRebuildOnConstruction = true;
    bEnableMainEngineEmitters = true;
    bEnableThrusterEmitters = true;
    MainEngineEmitterRelativeScale = FVector(1.15f, 0.5f, 0.5f);
    MainEngineEmitterRelativeRotation = FRotator(0.0f, 180.0f, 0.0f);
    NumMainEnginePoints = NumThrusterPoints = 0;
    MainEnginePointDefs.Empty(); ThrusterPointDefs.Empty(); NavLightDefs.Empty();
    BridgePointOffset = FVector(0.000000f, 0.000000f, 240.000000f);
    ChasePointOffset = FVector(250.000000f, 0.000000f, -1200.000000f);
    FocusPointOffset = FVector(0.000000f, 0.000000f, 240.000000f);
    DriveCenterPointOffset = FVector(0.000000f, 0.000000f, 0.000000f);
    QuantumPointOffset = FVector(0.0f, 0.0f, 0.0f);
    ShieldPointOffset = FVector(0.0f, 0.0f, 0.0f);
    SensorPointOffset = FVector(30.000000f, 0.000000f, 0.000000f);
    NavPointOffset = FVector(-10.000000f, 0.000000f, 0.000000f);
    ReactorPointOffset = FVector(0.000000f, 0.000000f, 0.000000f);
    ComputerPointAOffset = FVector(40.000000f, 0.000000f, 0.000000f);
    ComputerPointBOffset = FVector(90.000000f, 0.000000f, 0.000000f);

    WeaponMountPointDefs.Empty();
    NumWeaponMountPoints = 5;
    {
        FShipPointDef Point;
        Point.PointName = TEXT("Cannon");
        Point.LocalOffset = FVector(145.000000f, -24.000000f, -6.000000f);
        Point.LocalRotation = FRotator(0.0f, 0.0f, 0.0f);
        WeaponMountPointDefs.Add(Point);
    }
    {
        FShipPointDef Point;
        Point.PointName = TEXT("STA-1");
        Point.LocalOffset = FVector(-125.000000f, -93.000000f, -27.000000f);
        Point.LocalRotation = FRotator(0.0f, 0.0f, 0.0f);
        WeaponMountPointDefs.Add(Point);
    }
    {
        FShipPointDef Point;
        Point.PointName = TEXT("STA-2");
        Point.LocalOffset = FVector(-120.000000f, -62.000000f, -25.000000f);
        Point.LocalRotation = FRotator(0.0f, 0.0f, 0.0f);
        WeaponMountPointDefs.Add(Point);
    }
    {
        FShipPointDef Point;
        Point.PointName = TEXT("STA-3");
        Point.LocalOffset = FVector(-120.000000f, 62.000000f, -25.000000f);
        Point.LocalRotation = FRotator(0.0f, 0.0f, 0.0f);
        WeaponMountPointDefs.Add(Point);
    }
    {
        FShipPointDef Point;
        Point.PointName = TEXT("STA-4");
        Point.LocalOffset = FVector(-125.000000f, 93.000000f, -27.000000f);
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
void AIntruderShipActor::ApplyIntruderFixedPoints()
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

void AIntruderShipActor::ApplyAuthoredAttachmentDefaults()
{
    WeaponMountPointDefs.Reset();
    TurretBasePointDefs.Reset();
    { FShipPointDef Point; Point.PointName = TEXT("Cannon");
      Point.LocalOffset = FVector(120.000000f, 0.000000f, 0.000000f);
      Point.LocalRotation = FRotator(0.000000f, 0.000000f, 0.0f);
      WeaponMountPointDefs.Add(Point); }
    { FShipPointDef Point; Point.PointName = TEXT("STA-1");
      Point.LocalOffset = FVector(-100.000000f, -93.000000f, -27.000000f);
      Point.LocalRotation = FRotator(0.000000f, 0.000000f, 0.0f);
      WeaponMountPointDefs.Add(Point); }
    { FShipPointDef Point; Point.PointName = TEXT("STA-2");
      Point.LocalOffset = FVector(-100.000000f, -62.000000f, -25.000000f);
      Point.LocalRotation = FRotator(0.000000f, 0.000000f, 0.0f);
      WeaponMountPointDefs.Add(Point); }
    { FShipPointDef Point; Point.PointName = TEXT("STA-3");
      Point.LocalOffset = FVector(-100.000000f, 62.000000f, -25.000000f);
      Point.LocalRotation = FRotator(0.000000f, 0.000000f, 0.0f);
      WeaponMountPointDefs.Add(Point); }
    { FShipPointDef Point; Point.PointName = TEXT("STA-4");
      Point.LocalOffset = FVector(-100.000000f, 93.000000f, -27.000000f);
      Point.LocalRotation = FRotator(0.000000f, 0.000000f, 0.0f);
      WeaponMountPointDefs.Add(Point); }
    SyncShipAttachmentCounts();
}
