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

    ThrusterPointDefs.Empty();
    {
        FShipPointDef T;

        T.LocalOffset = FVector(-601.0f, -170.0f, 12.0f);
        T.LocalRotation = FRotator(0.0f, -90.0f, 0.0f);
        T.PointName = TEXT("Thruster_Left_Aft_0");
        ThrusterPointDefs.Add(T);

        T.LocalOffset = FVector(-579.0f, -170.0f, 12.0f);
        T.LocalRotation = FRotator(0.0f, -90.0f, 0.0f);
        T.PointName = TEXT("Thruster_Left_Aft_1");
        ThrusterPointDefs.Add(T);

        T.LocalOffset = FVector(784.0f, -100.0f, 14.0f);
        T.LocalRotation = FRotator(0.0f, -90.0f, 0.0f);
        T.PointName = TEXT("Thruster_Left_Fore_0");
        ThrusterPointDefs.Add(T);

        T.LocalOffset = FVector(784.0f, -100.0f, -32.0f);
        T.LocalRotation = FRotator(0.0f, -90.0f, 0.0f);
        T.PointName = TEXT("Thruster_Left_Fore_1");
        ThrusterPointDefs.Add(T);

        T.LocalOffset = FVector(-601.0f, 170.0f, 12.0f);
        T.LocalRotation = FRotator(0.0f, 90.0f, 0.0f);
        T.PointName = TEXT("Thruster_Right_Aft_0");
        ThrusterPointDefs.Add(T);

        T.LocalOffset = FVector(-579.0f, 170.0f, 12.0f);
        T.LocalRotation = FRotator(0.0f, 90.0f, 0.0f);
        T.PointName = TEXT("Thruster_Right_Aft_1");
        ThrusterPointDefs.Add(T);

        T.LocalOffset = FVector(784.0f, 100.0f, 14.0f);
        T.LocalRotation = FRotator(0.0f, 90.0f, 0.0f);
        T.PointName = TEXT("Thruster_Right_Fore_0");
        ThrusterPointDefs.Add(T);

        T.LocalOffset = FVector(784.0f, 100.0f, -32.0f);
        T.LocalRotation = FRotator(0.0f, 90.0f, 0.0f);
        T.PointName = TEXT("Thruster_Right_Fore_1");
        ThrusterPointDefs.Add(T);

        T.LocalOffset = FVector(845.0f, -73.0f, -32.0f);
        T.LocalRotation = FRotator(0.0f, 0.0f, 0.0f);
        T.PointName = TEXT("Thruster_Fore_0");
        ThrusterPointDefs.Add(T);

        T.LocalOffset = FVector(845.0f, -30.0f, -61.0f);
        T.LocalRotation = FRotator(0.0f, 0.0f, 0.0f);
        T.PointName = TEXT("Thruster_Fore_1");
        ThrusterPointDefs.Add(T);

        T.LocalOffset = FVector(845.0f, 30.0f, -61.0f);
        T.LocalRotation = FRotator(0.0f, 0.0f, 0.0f);
        T.PointName = TEXT("Thruster_Fore_2");
        ThrusterPointDefs.Add(T);

        T.LocalOffset = FVector(845.0f, 73.0f, -36.0f);
        T.LocalRotation = FRotator(0.0f, 0.0f, 0.0f);
        T.PointName = TEXT("Thruster_Fore_3");
        ThrusterPointDefs.Add(T);

        T.LocalOffset = FVector(-770.0f, -44.0f, 48.0f);
        T.LocalRotation = FRotator(0.0f, 180.0f, 0.0f); 
        T.PointName = TEXT("Thruster_Aft_0");
        ThrusterPointDefs.Add(T);

        T.LocalOffset = FVector(-770.0f, -44.0f, -30.0f);
        T.LocalRotation = FRotator(0.0f, 180.0f, 0.0f); 
        T.PointName = TEXT("Thruster_Aft_1");
        ThrusterPointDefs.Add(T);

        T.LocalOffset = FVector(-770.0f, 44.0f, 48.0f);
        T.LocalRotation = FRotator(0.0f, 180.0, 0.0f); 
        T.PointName = TEXT("Thruster_Aft_2");
        ThrusterPointDefs.Add(T);

        T.LocalOffset = FVector(-770.0f, 44.0f, -30.0f);
        T.LocalRotation = FRotator(0.0f, 180.0f, 0.0f); 
        T.PointName = TEXT("Thruster_Aft_3");
        ThrusterPointDefs.Add(T);

        T.LocalOffset = FVector(-601.0f, -77.0f, 90.0f);
        T.LocalRotation = FRotator(90.0f,0.0f, 0.0f);
        T.PointName = TEXT("Thruster_Top_Aft_0");
        ThrusterPointDefs.Add(T);

        T.LocalOffset = FVector(-601.0f, 77.0f, 90.0f);
        T.LocalRotation = FRotator(90.0f, 0.0f, 0.0f);
        T.PointName = TEXT("Thruster_Top_Aft_1");
        ThrusterPointDefs.Add(T); 

        T.LocalOffset = FVector(-601.0f, -77.0f, -80.0f);
        T.LocalRotation = FRotator(-90.0f, 180.0f, 0.0f);
        T.PointName = TEXT("Thruster_Bottom_Aft_0");
        ThrusterPointDefs.Add(T);

        T.LocalOffset = FVector(-601.0f, 77.0f, -80.0f);
        T.LocalRotation = FRotator(-90.0f, 180.0f, 0.0f);
        T.PointName = TEXT("Thruster_Bottom_Aft_1");
        ThrusterPointDefs.Add(T);  

        // GOOD ^^

        T.LocalOffset = FVector(783.0f, -26.0f, 91.0f);
        T.LocalRotation = FRotator(90.0f, 0.0f, 0.0f);
        T.PointName = TEXT("Thruster_Top_Fore_0");
        ThrusterPointDefs.Add(T);

        T.LocalOffset = FVector(783.0f, 26.0f, 91.0f);
        T.LocalRotation = FRotator(90.0f, 0.0f, 0.0f);
        T.PointName = TEXT("Thruster_Top_Fore_1");
        ThrusterPointDefs.Add(T);

        T.LocalOffset = FVector(783.0f, -26.0f, -105.0f);
        T.LocalRotation = FRotator(-90.0f, 180.0f, 0.0f);
        T.PointName = TEXT("Thruster_Bottom_Fore_0");
        ThrusterPointDefs.Add(T);

        T.LocalOffset = FVector(783.0f, 26.0f, -105.0f);
        T.LocalRotation = FRotator(-90.0f, 180.0f, 0.0f);
        T.PointName = TEXT("Thruster_Bottom_Fore_1");
        ThrusterPointDefs.Add(T);
    }
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