#include "BaikalShipActor.h"

ABaikalShipActor::ABaikalShipActor()
{
    ApplyBaikalDefaults();
}

void ABaikalShipActor::OnConstruction(const FTransform& Transform)
{
    ApplyBaikalDefaults();

    Super::OnConstruction(Transform);

    ApplyBaikalFixedPoints();
}

void ABaikalShipActor::ApplyBaikalDefaults()
{
    bAutoRebuildGeneratedComponents = true;
    bRebuildOnConstruction = true;

    bEnableMainEngineEmitters = true;
    bEnableThrusterEmitters = true;

    SetActorScale3D(FVector(1.6f));

    FocusPointOffset = FVector(50.0f, 0.0f, 0.0f);
    BridgePointOffset = FVector(92.0f, 0.0f, 0.0f);
    ChasePointOffset = FVector(200.0f, -1000.0f, 0.0f);

    DriveCenterPointOffset = FVector(-220.0f, 0.0f, 0.0f);
    QuantumPointOffset = FVector(-120.0f, 0.0f, 0.0f);
    ShieldPointOffset = FVector(-80.0f, 20.0f, 0.0f);
    SensorPointOffset = FVector(180.0f, 0.0f, 0.0f);
    NavPointOffset = FVector(60.0f, 16.0f, 0.0f);
    ComputerPointAOffset = FVector(80.0f, 16.0f, 20.0f);
    ComputerPointBOffset = FVector(80.0f, -16.0f, -20.0f);
    ReactorPointOffset = FVector(-60.0f, 0.0f, 0.0f);

    NumMainEnginePoints = 4;
    NumThrusterPoints = 16;
    NumWeaponMountPoints = 4;

    // --------------------------------------------------
    // MAIN ENGINES (drive ports)
    // --------------------------------------------------

    MainEnginePointDefs.Empty();
    {
        FShipPointDef P;

        P.LocalOffset = FVector(-435.0f, 43.0f, 0.0f);
        P.LocalRotation = FRotator(0.0f, 180.0f, 0.0f);
        P.PointName = TEXT("Drive_0");
        MainEnginePointDefs.Add(P);

        P.LocalOffset = FVector(-435.0f, 0.0f, 42.0f);
        P.PointName = TEXT("Drive_1");
        MainEnginePointDefs.Add(P);

        P.LocalOffset = FVector(-435.0f, 0.0f, -42.0f);
        P.PointName = TEXT("Drive_2");
        MainEnginePointDefs.Add(P);

        P.LocalOffset = FVector(-435.0f, -43.0f, 0.0f);
        P.PointName = TEXT("Drive_3");
        MainEnginePointDefs.Add(P);
    }

    // --------------------------------------------------
    // THRUSTERS
    // --------------------------------------------------

    ThrusterPointDefs.Empty();
    {
        FShipPointDef T;

        T.LocalOffset = FVector(-356.0f, 0.0f, -88.0f);
        T.LocalRotation = FRotator(0.0f, -90.0f, 0.0f);
        T.PointName = TEXT("Thruster_Left_Aft");
        ThrusterPointDefs.Add(T);

        T.LocalOffset = FVector(453.0f, -19.0f, -72.0f);
        T.PointName = TEXT("Thruster_Left_Fore_0");
        ThrusterPointDefs.Add(T);

        T.LocalOffset = FVector(453.0f, 19.0f, -72.0f);
        T.PointName = TEXT("Thruster_Left_Fore_1");
        ThrusterPointDefs.Add(T);

        T.LocalOffset = FVector(-356.0f, 0.0f, 88.0f);
        T.LocalRotation = FRotator(0.0f, 90.0f, 0.0f);
        T.PointName = TEXT("Thruster_Right_Aft");
        ThrusterPointDefs.Add(T);

        T.LocalOffset = FVector(453.0f, -19.0f, 72.0f);
        T.PointName = TEXT("Thruster_Right_Fore_0");
        ThrusterPointDefs.Add(T);

        T.LocalOffset = FVector(453.0f, 19.0f, 72.0f);
        T.PointName = TEXT("Thruster_Right_Fore_1");
        ThrusterPointDefs.Add(T);

        T.LocalOffset = FVector(498.0f, -19.0f, -54.0f);
        T.PointName = TEXT("Thruster_Fore_0");
        ThrusterPointDefs.Add(T);

        T.LocalOffset = FVector(498.0f, -41.0f, -21.0f);
        T.PointName = TEXT("Thruster_Fore_1");
        ThrusterPointDefs.Add(T);

        T.LocalOffset = FVector(498.0f, -41.0f, 21.0f);
        T.PointName = TEXT("Thruster_Fore_2");
        ThrusterPointDefs.Add(T);

        T.LocalOffset = FVector(498.0f, -19.0f, 54.0f);
        T.PointName = TEXT("Thruster_Fore_3");
        ThrusterPointDefs.Add(T);

        T.LocalOffset = FVector(-319.0f, 29.0f, -64.0f);
        T.LocalRotation = FRotator(90.0f, 0.0f, 0.0f);
        T.PointName = TEXT("Thruster_Top_Aft_0");
        ThrusterPointDefs.Add(T);

        T.LocalOffset = FVector(-319.0f, 29.0f, 64.0f);
        T.PointName = TEXT("Thruster_Top_Aft_1");
        ThrusterPointDefs.Add(T);

        T.LocalOffset = FVector(-319.0f, -29.0f, -64.0f);
        T.LocalRotation = FRotator(-90.0f, 0.0f, 0.0f);
        T.PointName = TEXT("Thruster_Bottom_Aft_0");
        ThrusterPointDefs.Add(T);

        T.LocalOffset = FVector(-319.0f, -29.0f, 64.0f);
        T.PointName = TEXT("Thruster_Bottom_Aft_1");
        ThrusterPointDefs.Add(T);

        T.LocalOffset = FVector(452.0f, 60.0f, -19.0f);
        T.LocalRotation = FRotator(90.0f, 0.0f, 0.0f);
        T.PointName = TEXT("Thruster_Top_Fore_0");
        ThrusterPointDefs.Add(T);

        T.LocalOffset = FVector(452.0f, 60.0f, 19.0f);
        T.PointName = TEXT("Thruster_Top_Fore_1");
        ThrusterPointDefs.Add(T);

        T.LocalOffset = FVector(452.0f, -60.0f, -19.0f);
        T.LocalRotation = FRotator(-90.0f, 0.0f, 0.0f);
        T.PointName = TEXT("Thruster_Bottom_Fore_0");
        ThrusterPointDefs.Add(T);

        T.LocalOffset = FVector(452.0f, -60.0f, 19.0f);
        T.PointName = TEXT("Thruster_Bottom_Fore_1");
        ThrusterPointDefs.Add(T);
    }

    // --------------------------------------------------
    // WEAPONS
    // --------------------------------------------------

    WeaponMountPointDefs.Empty();
    {
        FShipPointDef W;

        W.LocalOffset = FVector(500.0f, 25.0f, 0.0f);
        W.PointName = TEXT("Fwd_Cannon");
        WeaponMountPointDefs.Add(W);

        W.LocalOffset = FVector(450.0f, 0.0f, -20.0f);
        W.PointName = TEXT("Missile_Left");
        WeaponMountPointDefs.Add(W);

        W.LocalOffset = FVector(450.0f, 0.0f, 20.0f);
        W.PointName = TEXT("Missile_Right");
        WeaponMountPointDefs.Add(W);

        W.LocalOffset = FVector(-30.0f, 0.0f, 85.0f);
        W.LocalRotation = FRotator(0.0f, 90.0f, 0.0f);
        W.PointName = TEXT("Starboard_Cannon");
        WeaponMountPointDefs.Add(W);

        W.LocalOffset = FVector(-30.0f, 0.0f, -85.0f);
        W.LocalRotation = FRotator(0.0f, -90.0f, 0.0f);
        W.PointName = TEXT("Port_Cannon");
        WeaponMountPointDefs.Add(W);
    }

    NavLightDefs.Empty();
}

void ABaikalShipActor::ApplyBaikalFixedPoints()
{
    if (FocusPoint)
    {
        FocusPoint->SetRelativeLocation(FVector(0.0f, 0.0f, 92.0f));
    }

    if (BridgePoint)
    {
        BridgePoint->SetRelativeLocation(FVector(0.0f, 0.0f, 92.0f));
    }

    if (ChasePoint)
    {
        ChasePoint->SetRelativeLocation(FVector(0.0f, -1000.0f, 200.0f));
    }

    if (DriveCenterPoint)
    {
        DriveCenterPoint->SetRelativeLocation(FVector(0.0f, 0.0f, -220.0f));
    }

    if (QuantumPoint)
    {
        QuantumPoint->SetRelativeLocation(FVector(0.0f, 0.0f, -120.0f));
    }

    if (ShieldPoint)
    {
        ShieldPoint->SetRelativeLocation(FVector(0.0f, 20.0f, -80.0f));
    }

    if (SensorPoint)
    {
        SensorPoint->SetRelativeLocation(FVector(0.0f, 0.0f, 180.0f));
    }

    if (ComputerPointA)
    {
        ComputerPointA->SetRelativeLocation(FVector(20.0f, 16.0f, 80.0f));
    }

    if (ComputerPointB)
    {
        ComputerPointB->SetRelativeLocation(FVector(-20.0f, -16.0f, 80.0f));
    }

    if (NavPoint)
    {
        NavPoint->SetRelativeLocation(FVector(0.0f, 16.0f, 60.0f));
    }
}

void ABaikalShipActor::BuildBaikalMainEnginePoints()
{
    FShipPointDef P;

    P.LocalOffset = FVector(-435.0f, 43.0f, 0.0f);
    P.LocalRotation = FRotator(0.0f, 180.0f, 0.0f);
    MainEnginePointDefs.Add(P);

    P.LocalOffset = FVector(-435.0f, 0.0f, 42.0f);
    P.LocalRotation = FRotator(0.0f, 180.0f, 0.0f);
    MainEnginePointDefs.Add(P);

    P.LocalOffset = FVector(-435.0f, 0.0f, -42.0f);
    P.LocalRotation = FRotator(0.0f, 180.0f, 0.0f);
    MainEnginePointDefs.Add(P);

    P.LocalOffset = FVector(-435.0f, -43.0f, 0.0f);
    P.LocalRotation = FRotator(0.0f, 180.0f, 0.0f);
    MainEnginePointDefs.Add(P);
}

void ABaikalShipActor::BuildBaikalThrusterPoints()
{
    FShipPointDef T;

    // Port / left
    T.LocalOffset = FVector(-356.0f, 0.0f, -88.0f);
    T.LocalRotation = FRotator(0.0f, -90.0f, 0.0f);
    ThrusterPointDefs.Add(T);

    T.LocalOffset = FVector(453.0f, -19.0f, -72.0f);
    T.LocalRotation = FRotator(0.0f, -90.0f, 0.0f);
    ThrusterPointDefs.Add(T);

    T.LocalOffset = FVector(453.0f, 19.0f, -72.0f);
    T.LocalRotation = FRotator(0.0f, -90.0f, 0.0f);
    ThrusterPointDefs.Add(T);

    // Starboard / right
    T.LocalOffset = FVector(-356.0f, 0.0f, 88.0f);
    T.LocalRotation = FRotator(0.0f, 90.0f, 0.0f);
    ThrusterPointDefs.Add(T);

    T.LocalOffset = FVector(453.0f, -19.0f, 72.0f);
    T.LocalRotation = FRotator(0.0f, 90.0f, 0.0f);
    ThrusterPointDefs.Add(T);

    T.LocalOffset = FVector(453.0f, 19.0f, 72.0f);
    T.LocalRotation = FRotator(0.0f, 90.0f, 0.0f);
    ThrusterPointDefs.Add(T);

    // Fore
    T.LocalOffset = FVector(498.0f, -19.0f, -54.0f);
    T.LocalRotation = FRotator(0.0f, 0.0f, 0.0f);
    ThrusterPointDefs.Add(T);

    T.LocalOffset = FVector(498.0f, -41.0f, -21.0f);
    T.LocalRotation = FRotator(0.0f, 0.0f, 0.0f);
    ThrusterPointDefs.Add(T);

    T.LocalOffset = FVector(498.0f, -41.0f, 21.0f);
    T.LocalRotation = FRotator(0.0f, 0.0f, 0.0f);
    ThrusterPointDefs.Add(T);

    T.LocalOffset = FVector(498.0f, -19.0f, 54.0f);
    T.LocalRotation = FRotator(0.0f, 0.0f, 0.0f);
    ThrusterPointDefs.Add(T);

    // Dorsal / top
    T.LocalOffset = FVector(-319.0f, 29.0f, -64.0f);
    T.LocalRotation = FRotator(-90.0f, 0.0f, 0.0f);
    ThrusterPointDefs.Add(T);

    T.LocalOffset = FVector(-319.0f, 29.0f, 64.0f);
    T.LocalRotation = FRotator(-90.0f, 0.0f, 0.0f);
    ThrusterPointDefs.Add(T);

    T.LocalOffset = FVector(452.0f, 60.0f, -19.0f);
    T.LocalRotation = FRotator(-90.0f, 0.0f, 0.0f);
    ThrusterPointDefs.Add(T);

    T.LocalOffset = FVector(452.0f, 60.0f, 19.0f);
    T.LocalRotation = FRotator(-90.0f, 0.0f, 0.0f);
    ThrusterPointDefs.Add(T);

    // Ventral / bottom
    T.LocalOffset = FVector(-319.0f, -29.0f, -64.0f);
    T.LocalRotation = FRotator(90.0f, 0.0f, 0.0f);
    ThrusterPointDefs.Add(T);

    T.LocalOffset = FVector(-319.0f, -29.0f, 64.0f);
    T.LocalRotation = FRotator(90.0f, 0.0f, 0.0f);
    ThrusterPointDefs.Add(T);

    T.LocalOffset = FVector(452.0f, -60.0f, -19.0f);
    T.LocalRotation = FRotator(90.0f, 0.0f, 0.0f);
    ThrusterPointDefs.Add(T);

    T.LocalOffset = FVector(452.0f, -60.0f, 19.0f);
    T.LocalRotation = FRotator(90.0f, 0.0f, 0.0f);
    ThrusterPointDefs.Add(T);
}

void ABaikalShipActor::BuildBaikalWeaponMountPoints()
{
    FShipPointDef W;

    // Forward plasma cannon
    W.LocalOffset = FVector(470.0f, 0.0f, 0.0f);
    W.LocalRotation = FRotator(0.0f, 0.0f, 0.0f);
    WeaponMountPointDefs.Add(W);

    // Interceptor launcher
    W.LocalOffset = FVector(420.0f, 18.0f, 0.0f);
    W.LocalRotation = FRotator(0.0f, 0.0f, 0.0f);
    WeaponMountPointDefs.Add(W);

    // Port Ursa cannon
    W.LocalOffset = FVector(120.0f, 0.0f, -110.0f);
    W.LocalRotation = FRotator(0.0f, -90.0f, 0.0f);
    WeaponMountPointDefs.Add(W);

    // Starboard Ursa cannon
    W.LocalOffset = FVector(120.0f, 0.0f, 110.0f);
    W.LocalRotation = FRotator(0.0f, 90.0f, 0.0f);
    WeaponMountPointDefs.Add(W);
}

void ABaikalShipActor::BuildBaikalTurretBasePoints()
{
    FShipPointDef T;

    // Dorsal turret base
    T.LocalOffset = FVector(40.0f, 58.0f, 0.0f);
    T.LocalRotation = FRotator(0.0f, 0.0f, 0.0f);
    TurretBasePointDefs.Add(T);

    // Ventral turret base
    T.LocalOffset = FVector(40.0f, -58.0f, 0.0f);
    T.LocalRotation = FRotator(180.0f, 0.0f, 0.0f);
    TurretBasePointDefs.Add(T);
}

void ABaikalShipActor::BuildBaikalDockPoints()
{
    FShipPointDef D;

    D.LocalOffset = FVector(-120.0f, 0.0f, 0.0f);
    D.LocalRotation = FRotator(0.0f, 180.0f, 0.0f);
    DockPointDefs.Add(D);
}

void ABaikalShipActor::BuildBaikalLandingPoints()
{
    // Baikal does not appear to need explicit external landing points in this first pass.
    // Leave empty unless your .def has explicit bay / pad coordinates you want exposed.
}

void ABaikalShipActor::BuildBaikalNavLights()
{
    FShipNavLightDef L;

    // Port red
    L.LocalOffset = FVector(300.0f, 0.0f, -95.0f);
    L.LocalRotation = FRotator::ZeroRotator;
    L.Color = FLinearColor::Red;
    L.Intensity = 12000.0f;
    L.Radius = 400.0f;
    L.Mode = EShipNavLightMode::Blink;
    L.BlinkInterval = 0.50f;
    L.PhaseOffset = 0.00f;
    NavLightDefs.Add(L);

    // Starboard green
    L.LocalOffset = FVector(300.0f, 0.0f, 95.0f);
    L.LocalRotation = FRotator::ZeroRotator;
    L.Color = FLinearColor::Green;
    L.Intensity = 12000.0f;
    L.Radius = 400.0f;
    L.Mode = EShipNavLightMode::Blink;
    L.BlinkInterval = 0.50f;
    L.PhaseOffset = 0.25f;
    NavLightDefs.Add(L);

    // Dorsal white
    L.LocalOffset = FVector(100.0f, 70.0f, 0.0f);
    L.LocalRotation = FRotator::ZeroRotator;
    L.Color = FLinearColor::White;
    L.Intensity = 6000.0f;
    L.Radius = 300.0f;
    L.Mode = EShipNavLightMode::Steady;
    L.BlinkInterval = 1.0f;
    L.PhaseOffset = 0.0f;
    NavLightDefs.Add(L);

    // Ventral pale white/blue
    L.LocalOffset = FVector(100.0f, -70.0f, 0.0f);
    L.LocalRotation = FRotator::ZeroRotator;
    L.Color = FLinearColor(0.6f, 0.6f, 1.0f);
    L.Intensity = 6000.0f;
    L.Radius = 300.0f;
    L.Mode = EShipNavLightMode::Steady;
    L.BlinkInterval = 1.0f;
    L.PhaseOffset = 0.0f;
    NavLightDefs.Add(L);
}