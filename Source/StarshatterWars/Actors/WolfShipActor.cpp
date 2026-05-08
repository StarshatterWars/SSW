#include "WolfShipActor.h"

AWolfShipActor::AWolfShipActor()
{
    ApplyWolfDefaults();
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

    bEnableMainEngineEmitters = true;
    bEnableThrusterEmitters = true;

    MainEngineEmitterRelativeScale = FVector(1.35f, 0.60f, 0.60f);
    MainEngineEmitterRelativeRotation = FRotator(0.0f, 180.0f, 0.0f);

    ThrusterEmitterRelativeScale = FVector(0.55f, 0.30f, 0.30f);
    ThrusterEmitterRelativeRotation = FRotator::ZeroRotator;

    FocusPointOffset = FVector(50.0f, 0.0f, 0.0f);
    BridgePointOffset = FVector(60.0f, 320.0f, 0.0f);
    ChasePointOffset = FVector(170.0f, -1800.0f, 0.0f);

    DriveCenterPointOffset = FVector(-400.0f, 0.0f, 0.0f);
    QuantumPointOffset = FVector(0.0f, 0.0f, 0.0f);
    ShieldPointOffset = FVector(-80.0f, 20.0f, 0.0f);
    SensorPointOffset = FVector(380.0f, -16.0f, 0.0f);
    NavPointOffset = FVector(60.0f, 16.0f, 0.0f);
    ComputerPointAOffset = FVector(80.0f, 16.0f, 20.0f);
    ComputerPointBOffset = FVector(80.0f, -16.0f, -20.0f);
    ReactorPointOffset = FVector(-220.0f, -40.0f, 0.0f);

    NumMainEnginePoints = 3;
    NumThrusterPoints = 24;
    NumWeaponMountPoints = 7;
    NumTurretBasePoints = 0;
    NumDockPoints = 0;
    NumLandingPoints = 0;

    WeaponMountPointDefs.Empty();
    {
        FShipPointDef W;

        W.LocalOffset = FVector(800.0f, 23.0f, -25.0f);
        W.LocalRotation = FRotator(0.0f, 0.0f, 0.0f);
        W.PointName = TEXT("XRay_Laser_1");
        WeaponMountPointDefs.Add(W);

        W.LocalOffset = FVector(800.0f, 23.0f, 25.0f);
        W.LocalRotation = FRotator(0.0f, 0.0f, 0.0f);
        W.PointName = TEXT("XRay_Laser_2");
        WeaponMountPointDefs.Add(W);

        W.LocalOffset = FVector(500.0f, 82.0f, 0.0f);
        W.LocalRotation = FRotator(0.0f, 0.0f, 0.0f);
        W.PointName = TEXT("Fwd_Cannon");
        WeaponMountPointDefs.Add(W);

        W.LocalOffset = FVector(500.0f, -102.0f, 0.0f);
        W.LocalRotation = FRotator(180.0f, 0.0f, 0.0f);
        W.PointName = TEXT("Chin_Cannon");
        WeaponMountPointDefs.Add(W);

        W.LocalOffset = FVector(38.0f, 50.0f, 60.0f);
        W.LocalRotation = FRotator(0.0f, 90.0f, 0.0f);
        W.PointName = TEXT("Stbd_Cannon");
        WeaponMountPointDefs.Add(W);

        W.LocalOffset = FVector(38.0f, 50.0f, -60.0f);
        W.LocalRotation = FRotator(0.0f, -90.0f, 0.0f);
        W.PointName = TEXT("Port_Cannon");
        WeaponMountPointDefs.Add(W);

        W.LocalOffset = FVector(-256.0f, 70.0f, 0.0f);
        W.LocalRotation = FRotator(0.0f, 180.0f, 0.0f);
        W.PointName = TEXT("Aft_Cannon");
        WeaponMountPointDefs.Add(W);
    }

    TurretBasePointDefs.Empty();
    DockPointDefs.Empty();
    LandingPointDefs.Empty();

    BuildNavLightsFromRuntime();
    SetActorScale3D(FVector(1.7f));
}

void AWolfShipActor::ApplyWolfFixedPoints()
{
    if (BridgePoint)
    {
        BridgePoint->SetRelativeLocation(FVector(60.0f, 320.0f, 0.0f));
    }

    if (ChasePoint)
    {
        ChasePoint->SetRelativeLocation(FVector(170.0f, -1800.0f, 0.0f));
    }

    if (DriveCenterPoint)
    {
        DriveCenterPoint->SetRelativeLocation(FVector(-400.0f, 0.0f, 0.0f));
    }

    if (QuantumPoint)
    {
        QuantumPoint->SetRelativeLocation(FVector(0.0f, 0.0f, 0.0f));
    }

    if (ShieldPoint)
    {
        ShieldPoint->SetRelativeLocation(FVector(-80.0f, 20.0f, 0.0f));
    }

    if (SensorPoint)
    {
        SensorPoint->SetRelativeLocation(FVector(380.0f, -16.0f, 0.0f));
    }

    if (NavPoint)
    {
        NavPoint->SetRelativeLocation(FVector(60.0f, 16.0f, 0.0f));
    }

    if (ComputerPointA)
    {
        ComputerPointA->SetRelativeLocation(FVector(80.0f, 16.0f, 20.0f));
    }

    if (ComputerPointB)
    {
        ComputerPointB->SetRelativeLocation(FVector(80.0f, -16.0f, -20.0f));
    }

    if (ReactorPoint)
    {
        ReactorPoint->SetRelativeLocation(FVector(-220.0f, -40.0f, 0.0f));
    }
}