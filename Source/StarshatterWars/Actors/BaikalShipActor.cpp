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

    MainEngineEmitterRelativeScale = FVector(1.20f, 0.50f, 0.50f);
    MainEngineEmitterRelativeRotation = FRotator(0.0f, 180.0f, 0.0f);

    ThrusterEmitterRelativeScale = FVector(0.60f, 0.25f, 0.25f);
    ThrusterEmitterRelativeRotation = FRotator::ZeroRotator;

    FocusPointOffset = FVector(0.0f, 0.0f, 92.0f);
    BridgePointOffset = FVector(0.0f, 0.0f, 92.0f);
    ChasePointOffset = FVector(0.0f, -1000.0f, 200.0f);

    NumMainEnginePoints = 4;
    NumThrusterPoints = 18;

    // Adjust these counts if your Baikal.def has more exact weapon/turret entries:
    NumWeaponMountPoints = 4;
    NumTurretBasePoints = 2;
    NumDockPoints = 1;
    NumLandingPoints = 0;

    MainEnginePointDefs.Empty();
    ThrusterPointDefs.Empty();
    WeaponMountPointDefs.Empty();
    TurretBasePointDefs.Empty();
    DockPointDefs.Empty();
    LandingPointDefs.Empty();
    NavLightDefs.Empty();

    BuildBaikalMainEnginePoints();
    BuildBaikalThrusterPoints();
    BuildBaikalWeaponMountPoints();
    BuildBaikalTurretBasePoints();
    BuildBaikalDockPoints();
    BuildBaikalLandingPoints();
    BuildBaikalNavLights();

    SetActorScale3D(FVector(1.6f));
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