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

    MainEngineEmitterRelativeScale = FVector(1.15f, 0.50f, 0.50f);
    MainEngineEmitterRelativeRotation = FRotator(0.0f, 180.0f, 0.0f);

    ThrusterEmitterRelativeScale = FVector(0.45f, 0.25f, 0.25f);
    ThrusterEmitterRelativeRotation = FRotator::ZeroRotator;

    FocusPointOffset = FVector(92.0f, 0.0f, 0.0f);
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
    NumThrusterPoints = 18;
    NumWeaponMountPoints = 5;
    NumTurretBasePoints = 0;
    NumDockPoints = 0;
    NumLandingPoints = 0;

    MainEnginePointDefs.Empty();
    {
        FShipPointDef P;

        P.LocalOffset = FVector(-435.0f, 0.0f, 43.0f);
        P.LocalRotation = FRotator(0.0f, 0.0f, 0.0f);
        P.PointName = TEXT("Drive_Top");
        MainEnginePointDefs.Add(P);

        P.LocalOffset = FVector(-435.0f, 42.0f, 0.0f);
        P.LocalRotation = FRotator(0.0f, 0.0f, 0.0f);
        P.PointName = TEXT("Drive_Starboard");
        MainEnginePointDefs.Add(P);

        P.LocalOffset = FVector(-435.0f, -42.0f, 0.0f);
        P.LocalRotation = FRotator(0.0f, 0.0f, 0.0f);
        P.PointName = TEXT("Drive_Port");
        MainEnginePointDefs.Add(P);

        P.LocalOffset = FVector(-435.0f, 0.0f, -43.0f);
        P.LocalRotation = FRotator(0.0f, 0.0f, 0.0f);
        P.PointName = TEXT("Drive_Bottom");
        MainEnginePointDefs.Add(P);
    }

    ThrusterPointDefs.Empty();
    {
        FShipPointDef T;

        T.LocalOffset = FVector(-356.0f, 0.0f, -88.0f);
        T.LocalRotation = FRotator(0.0f, -90.0f, 0.0f);
        T.PointName = TEXT("Thruster_Left_Aft");
        ThrusterPointDefs.Add(T);

        T.LocalOffset = FVector(453.0f, -19.0f, -72.0f);
        T.LocalRotation = FRotator(0.0f, -90.0f, 0.0f);
        T.PointName = TEXT("Thruster_Left_Fore_0");
        ThrusterPointDefs.Add(T);

        T.LocalOffset = FVector(453.0f, 19.0f, -72.0f);
        T.LocalRotation = FRotator(0.0f, -90.0f, 0.0f);
        T.PointName = TEXT("Thruster_Left_Fore_1");
        ThrusterPointDefs.Add(T);

        T.LocalOffset = FVector(-356.0f, 0.0f, 88.0f);
        T.LocalRotation = FRotator(0.0f, 90.0f, 0.0f);
        T.PointName = TEXT("Thruster_Right_Aft");
        ThrusterPointDefs.Add(T);

        T.LocalOffset = FVector(453.0f, -19.0f, 72.0f);
        T.LocalRotation = FRotator(0.0f, 90.0f, 0.0f);
        T.PointName = TEXT("Thruster_Right_Fore_0");
        ThrusterPointDefs.Add(T);

        T.LocalOffset = FVector(453.0f, 19.0f, 72.0f);
        T.LocalRotation = FRotator(0.0f, 90.0f, 0.0f);
        T.PointName = TEXT("Thruster_Right_Fore_1");
        ThrusterPointDefs.Add(T);

        T.LocalOffset = FVector(498.0f, -19.0f, -54.0f);
        T.LocalRotation = FRotator(0.0f, 0.0f, 0.0f);
        T.PointName = TEXT("Thruster_Fore_0");
        ThrusterPointDefs.Add(T);

        T.LocalOffset = FVector(498.0f, -41.0f, -21.0f);
        T.LocalRotation = FRotator(0.0f, 0.0f, 0.0f);
        T.PointName = TEXT("Thruster_Fore_1");
        ThrusterPointDefs.Add(T);

        T.LocalOffset = FVector(498.0f, -41.0f, 21.0f);
        T.LocalRotation = FRotator(0.0f, 0.0f, 0.0f);
        T.PointName = TEXT("Thruster_Fore_2");
        ThrusterPointDefs.Add(T);

        T.LocalOffset = FVector(498.0f, -19.0f, 54.0f);
        T.LocalRotation = FRotator(0.0f, 0.0f, 0.0f);
        T.PointName = TEXT("Thruster_Fore_3");
        ThrusterPointDefs.Add(T);

        T.LocalOffset = FVector(-319.0f, 29.0f, -64.0f);
        T.LocalRotation = FRotator(90.0f, 0.0f, 0.0f);
        T.PointName = TEXT("Thruster_Top_Aft_0");
        ThrusterPointDefs.Add(T);

        T.LocalOffset = FVector(-319.0f, 29.0f, 64.0f);
        T.LocalRotation = FRotator(90.0f, 0.0f, 0.0f);
        T.PointName = TEXT("Thruster_Top_Aft_1");
        ThrusterPointDefs.Add(T);

        T.LocalOffset = FVector(-319.0f, -29.0f, -64.0f);
        T.LocalRotation = FRotator(-90.0f, 180.0f, 0.0f);
        T.PointName = TEXT("Thruster_Bottom_Aft_0");
        ThrusterPointDefs.Add(T);

        T.LocalOffset = FVector(-319.0f, -29.0f, 64.0f);
        T.LocalRotation = FRotator(-90.0f, 180.0f, 0.0f);
        T.PointName = TEXT("Thruster_Bottom_Aft_1");
        ThrusterPointDefs.Add(T);

        T.LocalOffset = FVector(452.0f, 60.0f, -19.0f);
        T.LocalRotation = FRotator(90.0f, 0.0f, 0.0f);
        T.PointName = TEXT("Thruster_Top_Fore_0");
        ThrusterPointDefs.Add(T);

        T.LocalOffset = FVector(452.0f, 60.0f, 19.0f);
        T.LocalRotation = FRotator(90.0f, 0.0f, 0.0f);
        T.PointName = TEXT("Thruster_Top_Fore_1");
        ThrusterPointDefs.Add(T);

        T.LocalOffset = FVector(452.0f, -60.0f, -19.0f);
        T.LocalRotation = FRotator(-90.0f, 180.0f, 0.0f);
        T.PointName = TEXT("Thruster_Bottom_Fore_0");
        ThrusterPointDefs.Add(T);

        T.LocalOffset = FVector(452.0f, -60.0f, 19.0f);
        T.LocalRotation = FRotator(-90.0f, 180.0f, 0.0f);
        T.PointName = TEXT("Thruster_Bottom_Fore_1");
        ThrusterPointDefs.Add(T);
    }

    WeaponMountPointDefs.Empty();
    {
        FShipPointDef W;

        W.LocalOffset = FVector(500.0f, 25.0f, 0.0f);
        W.LocalRotation = FRotator(0.0f, 0.0f, 0.0f);
        W.PointName = TEXT("Fwd_Cannon");
        WeaponMountPointDefs.Add(W);

        W.LocalOffset = FVector(450.0f, 0.0f, -20.0f);
        W.LocalRotation = FRotator(0.0f, 0.0f, 0.0f);
        W.PointName = TEXT("Interceptor_Left");
        WeaponMountPointDefs.Add(W);

        W.LocalOffset = FVector(450.0f, 0.0f, 20.0f);
        W.LocalRotation = FRotator(0.0f, 0.0f, 0.0f);
        W.PointName = TEXT("Interceptor_Right");
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

    TurretBasePointDefs.Empty();
    DockPointDefs.Empty();
    LandingPointDefs.Empty();

    NavLightDefs.Empty();
    {
        FShipNavLightDef L;

        L.LocalOffset = FVector(80.0f, -95.0f, 0.0f);
        L.LocalRotation = FRotator::ZeroRotator;
        L.Color = FLinearColor::Red;
        L.Intensity = 3500.0f;
        L.Radius = 350.0f;
        L.Mode = EShipNavLightMode::Blink;
        L.BlinkInterval = 0.50f;
        L.PhaseOffset = 0.00f;
        NavLightDefs.Add(L);

        L.LocalOffset = FVector(80.0f, 95.0f, 0.0f);
        L.LocalRotation = FRotator::ZeroRotator;
        L.Color = FLinearColor::Green;
        L.Intensity = 3500.0f;
        L.Radius = 350.0f;
        L.Mode = EShipNavLightMode::Blink;
        L.BlinkInterval = 0.50f;
        L.PhaseOffset = 0.25f;
        NavLightDefs.Add(L);

        L.LocalOffset = FVector(180.0f, 0.0f, 70.0f);
        L.LocalRotation = FRotator::ZeroRotator;
        L.Color = FLinearColor::White;
        L.Intensity = 3000.0f;
        L.Radius = 300.0f;
        L.Mode = EShipNavLightMode::Steady;
        L.BlinkInterval = 1.00f;
        L.PhaseOffset = 0.00f;
        NavLightDefs.Add(L);

        L.LocalOffset = FVector(-120.0f, 0.0f, -70.0f);
        L.LocalRotation = FRotator::ZeroRotator;
        L.Color = FLinearColor(0.6f, 0.6f, 1.0f);
        L.Intensity = 3000.0f;
        L.Radius = 300.0f;
        L.Mode = EShipNavLightMode::Steady;
        L.BlinkInterval = 1.00f;
        L.PhaseOffset = 0.00f;
        NavLightDefs.Add(L);
    }

    SetActorScale3D(FVector(1.6f));
}

void ABaikalShipActor::ApplyBaikalFixedPoints()
{
    if (FocusPoint)
    {
        FocusPoint->SetRelativeLocation(FVector(92.0f, 0.0f, 0.0f));
    }

    if (BridgePoint)
    {
        BridgePoint->SetRelativeLocation(FVector(92.0f, 0.0f, 0.0f));
    }

    if (ChasePoint)
    {
        ChasePoint->SetRelativeLocation(FVector(200.0f, -1000.0f, 0.0f));
    }

    if (DriveCenterPoint)
    {
        DriveCenterPoint->SetRelativeLocation(FVector(-220.0f, 0.0f, 0.0f));
    }

    if (QuantumPoint)
    {
        QuantumPoint->SetRelativeLocation(FVector(-120.0f, 0.0f, 0.0f));
    }

    if (ShieldPoint)
    {
        ShieldPoint->SetRelativeLocation(FVector(-80.0f, 20.0f, 0.0f));
    }

    if (SensorPoint)
    {
        SensorPoint->SetRelativeLocation(FVector(180.0f, 0.0f, 0.0f));
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
        ReactorPoint->SetRelativeLocation(FVector(-60.0f, 0.0f, 0.0f));
    }
}

void ABaikalShipActor::BuildBaikalMainEnginePoints()
{
    MainEnginePointDefs.Empty();

    FShipPointDef P;

    P.LocalOffset = FVector(-435.0f, 0.0f, 43.0f);
    P.LocalRotation = FRotator(0.0f, 0.0f, 0.0f);
    P.PointName = TEXT("Drive_Top");
    MainEnginePointDefs.Add(P);

    P.LocalOffset = FVector(-435.0f, 42.0f, 0.0f);
    P.LocalRotation = FRotator(0.0f, 0.0f, 0.0f);
    P.PointName = TEXT("Drive_Starboard");
    MainEnginePointDefs.Add(P);

    P.LocalOffset = FVector(-435.0f, -42.0f, 0.0f);
    P.LocalRotation = FRotator(0.0f, 0.0f, 0.0f);
    P.PointName = TEXT("Drive_Port");
    MainEnginePointDefs.Add(P);

    P.LocalOffset = FVector(-435.0f, 0.0f, -43.0f);
    P.LocalRotation = FRotator(0.0f, 0.0f, 0.0f);
    P.PointName = TEXT("Drive_Bottom");
    MainEnginePointDefs.Add(P);
}

void ABaikalShipActor::BuildBaikalThrusterPoints()
{
    ThrusterPointDefs.Empty();

    FShipPointDef T;

    T.LocalOffset = FVector(-356.0f, 0.0f, -88.0f);
    T.LocalRotation = FRotator(0.0f, -90.0f, 0.0f);
    T.PointName = TEXT("Thruster_Left_Aft");
    ThrusterPointDefs.Add(T);

    T.LocalOffset = FVector(453.0f, -19.0f, -72.0f);
    T.LocalRotation = FRotator(0.0f, -90.0f, 0.0f);
    T.PointName = TEXT("Thruster_Left_Fore_0");
    ThrusterPointDefs.Add(T);

    T.LocalOffset = FVector(453.0f, 19.0f, -72.0f);
    T.LocalRotation = FRotator(0.0f, -90.0f, 0.0f);
    T.PointName = TEXT("Thruster_Left_Fore_1");
    ThrusterPointDefs.Add(T);

    T.LocalOffset = FVector(-356.0f, 0.0f, 88.0f);
    T.LocalRotation = FRotator(0.0f, 90.0f, 0.0f);
    T.PointName = TEXT("Thruster_Right_Aft");
    ThrusterPointDefs.Add(T);

    T.LocalOffset = FVector(453.0f, -19.0f, 72.0f);
    T.LocalRotation = FRotator(0.0f, 90.0f, 0.0f);
    T.PointName = TEXT("Thruster_Right_Fore_0");
    ThrusterPointDefs.Add(T);

    T.LocalOffset = FVector(453.0f, 19.0f, 72.0f);
    T.LocalRotation = FRotator(0.0f, 90.0f, 0.0f);
    T.PointName = TEXT("Thruster_Right_Fore_1");
    ThrusterPointDefs.Add(T);

    T.LocalOffset = FVector(498.0f, -19.0f, -54.0f);
    T.LocalRotation = FRotator(0.0f, 0.0f, 0.0f);
    T.PointName = TEXT("Thruster_Fore_0");
    ThrusterPointDefs.Add(T);

    T.LocalOffset = FVector(498.0f, -41.0f, -21.0f);
    T.LocalRotation = FRotator(0.0f, 0.0f, 0.0f);
    T.PointName = TEXT("Thruster_Fore_1");
    ThrusterPointDefs.Add(T);

    T.LocalOffset = FVector(498.0f, -41.0f, 21.0f);
    T.LocalRotation = FRotator(0.0f, 0.0f, 0.0f);
    T.PointName = TEXT("Thruster_Fore_2");
    ThrusterPointDefs.Add(T);

    T.LocalOffset = FVector(498.0f, -19.0f, 54.0f);
    T.LocalRotation = FRotator(0.0f, 0.0f, 0.0f);
    T.PointName = TEXT("Thruster_Fore_3");
    ThrusterPointDefs.Add(T);

    T.LocalOffset = FVector(-319.0f, 29.0f, -64.0f);
    T.LocalRotation = FRotator(90.0f, 0.0f, 0.0f);
    T.PointName = TEXT("Thruster_Top_Aft_0");
    ThrusterPointDefs.Add(T);

    T.LocalOffset = FVector(-319.0f, 29.0f, 64.0f);
    T.LocalRotation = FRotator(90.0f, 0.0f, 0.0f);
    T.PointName = TEXT("Thruster_Top_Aft_1");
    ThrusterPointDefs.Add(T);

    T.LocalOffset = FVector(-319.0f, -29.0f, -64.0f);
    T.LocalRotation = FRotator(-90.0f, 180.0f, 0.0f);
    T.PointName = TEXT("Thruster_Bottom_Aft_0");
    ThrusterPointDefs.Add(T);

    T.LocalOffset = FVector(-319.0f, -29.0f, 64.0f);
    T.LocalRotation = FRotator(-90.0f, 180.0f, 0.0f);
    T.PointName = TEXT("Thruster_Bottom_Aft_1");
    ThrusterPointDefs.Add(T);

    T.LocalOffset = FVector(452.0f, 60.0f, -19.0f);
    T.LocalRotation = FRotator(90.0f, 0.0f, 0.0f);
    T.PointName = TEXT("Thruster_Top_Fore_0");
    ThrusterPointDefs.Add(T);

    T.LocalOffset = FVector(452.0f, 60.0f, 19.0f);
    T.LocalRotation = FRotator(90.0f, 0.0f, 0.0f);
    T.PointName = TEXT("Thruster_Top_Fore_1");
    ThrusterPointDefs.Add(T);

    T.LocalOffset = FVector(452.0f, -60.0f, -19.0f);
    T.LocalRotation = FRotator(-90.0f, 180.0f, 0.0f);
    T.PointName = TEXT("Thruster_Bottom_Fore_0");
    ThrusterPointDefs.Add(T);

    T.LocalOffset = FVector(452.0f, -60.0f, 19.0f);
    T.LocalRotation = FRotator(-90.0f, 180.0f, 0.0f);
    T.PointName = TEXT("Thruster_Bottom_Fore_1");
    ThrusterPointDefs.Add(T);
}

void ABaikalShipActor::BuildBaikalWeaponMountPoints()
{
    WeaponMountPointDefs.Empty();

    FShipPointDef W;

    W.LocalOffset = FVector(500.0f, 25.0f, 0.0f);
    W.LocalRotation = FRotator(0.0f, 0.0f, 0.0f);
    W.PointName = TEXT("Fwd_Cannon");
    WeaponMountPointDefs.Add(W);

    W.LocalOffset = FVector(450.0f, 0.0f, -20.0f);
    W.LocalRotation = FRotator(0.0f, 0.0f, 0.0f);
    W.PointName = TEXT("Interceptor_Left");
    WeaponMountPointDefs.Add(W);

    W.LocalOffset = FVector(450.0f, 0.0f, 20.0f);
    W.LocalRotation = FRotator(0.0f, 0.0f, 0.0f);
    W.PointName = TEXT("Interceptor_Right");
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

void ABaikalShipActor::BuildBaikalTurretBasePoints()
{
    TurretBasePointDefs.Empty();
}

void ABaikalShipActor::BuildBaikalDockPoints()
{
    DockPointDefs.Empty();
}

void ABaikalShipActor::BuildBaikalLandingPoints()
{
    LandingPointDefs.Empty();
}

void ABaikalShipActor::BuildBaikalNavLights()
{
    NavLightDefs.Empty();

    FShipNavLightDef L;

    L.LocalOffset = FVector(80.0f, -95.0f, 0.0f);
    L.LocalRotation = FRotator::ZeroRotator;
    L.Color = FLinearColor::Red;
    L.Intensity = 3500.0f;
    L.Radius = 350.0f;
    L.Mode = EShipNavLightMode::Blink;
    L.BlinkInterval = 0.50f;
    L.PhaseOffset = 0.00f;
    NavLightDefs.Add(L);

    L.LocalOffset = FVector(80.0f, 95.0f, 0.0f);
    L.LocalRotation = FRotator::ZeroRotator;
    L.Color = FLinearColor::Green;
    L.Intensity = 3500.0f;
    L.Radius = 350.0f;
    L.Mode = EShipNavLightMode::Blink;
    L.BlinkInterval = 0.50f;
    L.PhaseOffset = 0.25f;
    NavLightDefs.Add(L);

    L.LocalOffset = FVector(180.0f, 0.0f, 70.0f);
    L.LocalRotation = FRotator::ZeroRotator;
    L.Color = FLinearColor::White;
    L.Intensity = 3000.0f;
    L.Radius = 300.0f;
    L.Mode = EShipNavLightMode::Steady;
    L.BlinkInterval = 1.00f;
    L.PhaseOffset = 0.00f;
    NavLightDefs.Add(L);

    L.LocalOffset = FVector(-120.0f, 0.0f, -70.0f);
    L.LocalRotation = FRotator::ZeroRotator;
    L.Color = FLinearColor(0.6f, 0.6f, 1.0f);
    L.Intensity = 3000.0f;
    L.Radius = 300.0f;
    L.Mode = EShipNavLightMode::Steady;
    L.BlinkInterval = 1.00f;
    L.PhaseOffset = 0.00f;
    NavLightDefs.Add(L);
}