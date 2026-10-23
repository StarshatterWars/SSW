// Structural defaults from Jackal.def; simulation remains data-driven.
#include "JackalShipActor.h"
#include "Components/SceneComponent.h"

AJackalShipActor::AJackalShipActor()
{

    SetActorScale3D(FVector(0.275000f));
    ApplyJackalDefaults();
    ApplyAuthoredAttachmentDefaults();
    InitializeNativeAuthoredAttachments();
}
void AJackalShipActor::OnConstruction(const FTransform& Transform)
{
    ApplyJackalDefaults();
    Super::OnConstruction(Transform);
    ApplyJackalFixedPoints();
}
void AJackalShipActor::ApplyJackalDefaults()
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
    DriveCenterPointOffset = FVector(-80.000000f, 0.000000f, 0.000000f);
    QuantumPointOffset = FVector(0.0f, 0.0f, 0.0f);
    ShieldPointOffset = FVector(0.0f, 0.0f, 0.0f);
    SensorPointOffset = FVector(100.000000f, 0.000000f, 0.000000f);
    NavPointOffset = FVector(60.000000f, 0.000000f, 0.000000f);
    ReactorPointOffset = FVector(0.000000f, 0.000000f, 0.000000f);
    ComputerPointAOffset = FVector(80.000000f, 20.000000f, 0.000000f);
    ComputerPointBOffset = FVector(80.000000f, -20.000000f, 0.000000f);

    WeaponMountPointDefs.Empty();
    NumWeaponMountPoints = 2;
    {
        FShipPointDef Point;
        Point.PointName = TEXT("Gun-1");
        Point.LocalOffset = FVector(182.000000f, -8.000000f, -26.000000f);
        Point.LocalRotation = FRotator(0.0f, 0.0f, 0.0f);
        WeaponMountPointDefs.Add(Point);
    }
    {
        FShipPointDef Point;
        Point.PointName = TEXT("Gun-2");
        Point.LocalOffset = FVector(182.000000f, 8.000000f, -26.000000f);
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
void AJackalShipActor::ApplyJackalFixedPoints()
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

void AJackalShipActor::ApplyAuthoredAttachmentDefaults()
{
    WeaponMountPointDefs.Reset();
    TurretBasePointDefs.Reset();
    { FShipPointDef Point; Point.PointName = TEXT("Port Cannon");
      Point.LocalOffset = FVector(160.000000f, -8.000000f, -26.000000f);
      Point.LocalRotation = FRotator(0.000000f, 0.000000f, 0.0f);
      WeaponMountPointDefs.Add(Point); }
    { FShipPointDef Point; Point.PointName = TEXT("Starboard Cannon");
      Point.LocalOffset = FVector(160.000000f, 8.000000f, -26.000000f);
      Point.LocalRotation = FRotator(0.000000f, 0.000000f, 0.0f);
      WeaponMountPointDefs.Add(Point); }
    SyncShipAttachmentCounts();
}
