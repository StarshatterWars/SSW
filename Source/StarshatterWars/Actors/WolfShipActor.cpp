#include "WolfShipActor.h"

AWolfShipActor::AWolfShipActor()
{


    // Native default only: preserve BP and placed-instance scale during construction.
    SetActorScale3D(FVector(1.7f));
    ApplyWolfDefaults();
    ApplyAuthoredAttachmentDefaults();
    InitializeNativeAuthoredAttachments(TEXT("Wolf"));
}

void AWolfShipActor::OnConstruction(const FTransform& Transform)
{
    ApplyWolfDefaults();

    Super::OnConstruction(Transform);

    ApplyWolfFixedPoints();
}

void AWolfShipActor::ApplyWolfDefaults()
{
    bAutoRebuildGeneratedComponents = true;
    bRebuildOnConstruction = true;

    /*
     * Runtime visual systems
     */
    bEnableMainEngineEmitters = true;
    bEnableThrusterEmitters = true;

    /*
     * Main engine Niagara defaults
     */
    MainEngineEmitterRelativeScale =
        FVector(0.65f, 0.25f, 0.25f);

    MainEngineEmitterRelativeRotation =
        FRotator(0.0f, 180.0f, 0.0f);

    /*
     * Camera and framing
     */
    FocusPointOffset =
        FVector(50.0f, 0.0f, 0.0f);

    BridgePointOffset =
        FVector(60.0f, 320.0f, 0.0f);

    ChasePointOffset =
        FVector(170.0f, -1800.0f, 0.0f);

    /*
     * Ship subsystem reference points
     */
    DriveCenterPointOffset =
        FVector(-400.0f, 0.0f, 0.0f);

    QuantumPointOffset =
        FVector(0.0f, 0.0f, 0.0f);

    ShieldPointOffset =
        FVector(-80.0f, 20.0f, 0.0f);

    SensorPointOffset =
        FVector(380.0f, -16.0f, 0.0f);

    NavPointOffset =
        FVector(60.0f, 16.0f, 0.0f);

    ComputerPointAOffset =
        FVector(80.0f, 16.0f, 20.0f);

    ComputerPointBOffset =
        FVector(80.0f, -16.0f, -20.0f);

    ReactorPointOffset =
        FVector(-220.0f, -40.0f, 0.0f);

    /*
     * Runtime thrusters own all thruster ports.
     * Disable generated fallback thruster system.
     */
    // Runtime drive ports from Wolf.def are authoritative.
    NumMainEnginePoints = 0;
    MainEnginePointDefs.Empty();

    NumThrusterPoints = 0;
    ThrusterPointDefs.Empty();

    NumWeaponMountPoints = 7;
    NumTurretBasePoints = 0;
    NumDockPoints = 0;
    NumLandingPoints = 0;

    /*
     * Weapon mounts
     */
    WeaponMountPointDefs.Empty();

    {
        FShipPointDef W;

        W.LocalOffset =
            FVector(800.0f, 23.0f, -25.0f);

        W.LocalRotation =
            FRotator(0.0f, 0.0f, 0.0f);

        W.PointName =
            TEXT("XRay_Laser_1");

        WeaponMountPointDefs.Add(W);

        W.LocalOffset =
            FVector(800.0f, 23.0f, 25.0f);

        W.LocalRotation =
            FRotator(0.0f, 0.0f, 0.0f);

        W.PointName =
            TEXT("XRay_Laser_2");

        WeaponMountPointDefs.Add(W);

        W.LocalOffset =
            FVector(500.0f, 82.0f, 0.0f);

        W.LocalRotation =
            FRotator(0.0f, 0.0f, 0.0f);

        W.PointName =
            TEXT("Fwd_Cannon");

        WeaponMountPointDefs.Add(W);

        W.LocalOffset =
            FVector(500.0f, -102.0f, 0.0f);

        W.LocalRotation =
            FRotator(180.0f, 0.0f, 0.0f);

        W.PointName =
            TEXT("Chin_Cannon");

        WeaponMountPointDefs.Add(W);

        W.LocalOffset =
            FVector(38.0f, 50.0f, 60.0f);

        W.LocalRotation =
            FRotator(0.0f, 90.0f, 0.0f);

        W.PointName =
            TEXT("Stbd_Cannon");

        WeaponMountPointDefs.Add(W);

        W.LocalOffset =
            FVector(38.0f, 50.0f, -60.0f);

        W.LocalRotation =
            FRotator(0.0f, -90.0f, 0.0f);

        W.PointName =
            TEXT("Port_Cannon");

        WeaponMountPointDefs.Add(W);

        W.LocalOffset =
            FVector(-256.0f, 70.0f, 0.0f);

        W.LocalRotation =
            FRotator(0.0f, 180.0f, 0.0f);

        W.PointName =
            TEXT("Aft_Cannon");

        WeaponMountPointDefs.Add(W);
    }

    TurretBasePointDefs.Empty();
    DockPointDefs.Empty();
    LandingPointDefs.Empty();


}

void AWolfShipActor::ApplyWolfFixedPoints()
{
    if (BridgePoint)
    {
        BridgePoint->SetRelativeLocation(
            FVector(60.0f, 320.0f, 0.0f));
    }

    if (ChasePoint)
    {
        ChasePoint->SetRelativeLocation(
            FVector(170.0f, -1800.0f, 0.0f));
    }

    if (DriveCenterPoint)
    {
        DriveCenterPoint->SetRelativeLocation(
            FVector(-400.0f, 0.0f, 0.0f));
    }

    if (QuantumPoint)
    {
        QuantumPoint->SetRelativeLocation(
            FVector(0.0f, 0.0f, 0.0f));
    }

    if (ShieldPoint)
    {
        ShieldPoint->SetRelativeLocation(
            FVector(-80.0f, 20.0f, 0.0f));
    }

    if (SensorPoint)
    {
        SensorPoint->SetRelativeLocation(
            FVector(380.0f, -16.0f, 0.0f));
    }

    if (NavPoint)
    {
        NavPoint->SetRelativeLocation(
            FVector(60.0f, 16.0f, 0.0f));
    }

    if (ComputerPointA)
    {
        ComputerPointA->SetRelativeLocation(
            FVector(80.0f, 16.0f, 20.0f));
    }

    if (ComputerPointB)
    {
        ComputerPointB->SetRelativeLocation(
            FVector(80.0f, -16.0f, -20.0f));
    }

    if (ReactorPoint)
    {
        ReactorPoint->SetRelativeLocation(
            FVector(-220.0f, -40.0f, 0.0f));
    }
}
void AWolfShipActor::ApplyAuthoredAttachmentDefaults()
{
    WeaponMountPointDefs.Reset();
    TurretBasePointDefs.Reset();
    { FShipPointDef Point; Point.PointName = TEXT("XRay Laser 1");
      Point.LocalOffset = FVector(800.000000f, -25.000000f, 23.000000f);
      Point.LocalRotation = FRotator(0.000000f, 0.000000f, 0.0f);
      WeaponMountPointDefs.Add(Point); }
    { FShipPointDef Point; Point.PointName = TEXT("XRay Laser 2");
      Point.LocalOffset = FVector(800.000000f, 25.000000f, 23.000000f);
      Point.LocalRotation = FRotator(0.000000f, 0.000000f, 0.0f);
      WeaponMountPointDefs.Add(Point); }
    { FShipPointDef Point; Point.PointName = TEXT("Fwd Cannon");
      Point.LocalOffset = FVector(500.000000f, 0.000000f, 82.000000f);
      Point.LocalRotation = FRotator(0.000000f, 0.000000f, 0.0f);
      WeaponMountPointDefs.Add(Point); }
    { FShipPointDef Point; Point.PointName = TEXT("Chin Cannon");
      Point.LocalOffset = FVector(500.000000f, 0.000000f, -102.000000f);
      Point.LocalRotation = FRotator(0.000000f, 0.000000f, 0.0f);
      WeaponMountPointDefs.Add(Point); }
    { FShipPointDef Point; Point.PointName = TEXT("Stbd Cannon");
      Point.LocalOffset = FVector(38.000000f, 60.000000f, 50.000000f);
      Point.LocalRotation = FRotator(0.000000f, 90.000000f, 0.0f);
      WeaponMountPointDefs.Add(Point); }
    { FShipPointDef Point; Point.PointName = TEXT("Port Cannon");
      Point.LocalOffset = FVector(38.000000f, -60.000000f, 50.000000f);
      Point.LocalRotation = FRotator(0.000000f, -90.000000f, 0.0f);
      WeaponMountPointDefs.Add(Point); }
    { FShipPointDef Point; Point.PointName = TEXT("Aft Cannon");
      Point.LocalOffset = FVector(-256.000000f, 0.000000f, 70.000000f);
      Point.LocalRotation = FRotator(0.000000f, 180.000000f, 0.0f);
      WeaponMountPointDefs.Add(Point); }
    { FShipPointDef Point; Point.PointName = TEXT("Fwd Cannon_Base");
      Point.LocalOffset = FVector(500.000000f, 0.000000f, 82.000000f);
      Point.LocalRotation = FRotator(0.000000f, 0.000000f, 0.0f);
      TurretBasePointDefs.Add(Point); }
    { FShipPointDef Point; Point.PointName = TEXT("Chin Cannon_Base");
      Point.LocalOffset = FVector(500.000000f, 0.000000f, -102.000000f);
      Point.LocalRotation = FRotator(0.000000f, 0.000000f, 0.0f);
      TurretBasePointDefs.Add(Point); }
    { FShipPointDef Point; Point.PointName = TEXT("Stbd Cannon_Base");
      Point.LocalOffset = FVector(38.000000f, 60.000000f, 50.000000f);
      Point.LocalRotation = FRotator(0.000000f, 90.000000f, 0.0f);
      TurretBasePointDefs.Add(Point); }
    { FShipPointDef Point; Point.PointName = TEXT("Port Cannon_Base");
      Point.LocalOffset = FVector(38.000000f, -60.000000f, 50.000000f);
      Point.LocalRotation = FRotator(0.000000f, -90.000000f, 0.0f);
      TurretBasePointDefs.Add(Point); }
    { FShipPointDef Point; Point.PointName = TEXT("Aft Cannon_Base");
      Point.LocalOffset = FVector(-256.000000f, 0.000000f, 70.000000f);
      Point.LocalRotation = FRotator(0.000000f, 180.000000f, 0.0f);
      TurretBasePointDefs.Add(Point); }
    SyncShipAttachmentCounts();
}
